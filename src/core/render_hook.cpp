#include "core/render_hook.h"

#include <d3d11.h>
#include <dxgi.h>
#include <windows.h>

#include <MinHook.h>
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>

#include "core/input.h"
#include "core/log.h"
#include "core/menu.h"

namespace core::render {
namespace {

using PresentFn = HRESULT(STDMETHODCALLTYPE*)(IDXGISwapChain*, UINT, UINT);
using ResizeBuffersFn = HRESULT(STDMETHODCALLTYPE*)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT);

constexpr int kPresentIndex = 8;
constexpr int kResizeBuffersIndex = 13;

PresentFn g_origPresent = nullptr;
ResizeBuffersFn g_origResizeBuffers = nullptr;
void* g_presentTarget = nullptr;
void* g_resizeTarget = nullptr;

bool g_imguiReady = false;
IDXGISwapChain* g_swapChain = nullptr;  // the swap chain ImGui was set up on
ID3D11Device* g_device = nullptr;
ID3D11DeviceContext* g_context = nullptr;
ID3D11RenderTargetView* g_rtv = nullptr;

void CreateRenderTarget(IDXGISwapChain* swapChain) {
    ID3D11Texture2D* backBuffer = nullptr;
    if (SUCCEEDED(swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer))) && backBuffer) {
        g_device->CreateRenderTargetView(backBuffer, nullptr, &g_rtv);
        backBuffer->Release();
    }
}

void ReleaseRenderTarget() {
    if (g_rtv) {
        g_rtv->Release();
        g_rtv = nullptr;
    }
}

bool InitImGui(IDXGISwapChain* swapChain) {
    if (FAILED(swapChain->GetDevice(IID_PPV_ARGS(&g_device)))) return false;
    g_device->GetImmediateContext(&g_context);

    DXGI_SWAP_CHAIN_DESC desc{};
    swapChain->GetDesc(&desc);

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;  // window layout is not persisted
    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
    ImGui::StyleColorsDark();

    ImGui_ImplWin32_Init(desc.OutputWindow);
    ImGui_ImplDX11_Init(g_device, g_context);
    input::Install(desc.OutputWindow);
    CreateRenderTarget(swapChain);

    g_swapChain = swapChain;
    log::Info("ImGui initialised on window %p (%ux%u)", desc.OutputWindow,
              desc.BufferDesc.Width, desc.BufferDesc.Height);
    return true;
}

HRESULT STDMETHODCALLTYPE HookPresent(IDXGISwapChain* swapChain, UINT syncInterval, UINT flags) {
    if (!g_imguiReady) g_imguiReady = InitImGui(swapChain);

    // Only draw on the swap chain we initialised on (ignores any secondary chains).
    if (g_imguiReady && swapChain == g_swapChain && !(flags & DXGI_PRESENT_TEST)) {
        if (!g_rtv) CreateRenderTarget(swapChain);

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::GetIO().MouseDrawCursor = menu::IsOpen();
        ImGui::NewFrame();
        menu::Draw();
        ImGui::Render();

        if (g_rtv) {
            g_context->OMSetRenderTargets(1, &g_rtv, nullptr);
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        }
    }
    return g_origPresent(swapChain, syncInterval, flags);
}

HRESULT STDMETHODCALLTYPE HookResizeBuffers(IDXGISwapChain* swapChain, UINT count, UINT width,
                                            UINT height, DXGI_FORMAT format, UINT flags) {
    if (swapChain == g_swapChain) ReleaseRenderTarget();  // recreated lazily on next Present
    return g_origResizeBuffers(swapChain, count, width, height, format, flags);
}

// Reads Present/ResizeBuffers addresses from a throwaway swap chain's vtable.
bool FindSwapChainVTable(void** present, void** resize) {
    WNDCLASSEXW wc{sizeof(wc)};
    wc.lpfnWndProc = DefWindowProcW;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"SDTrainerDummy";
    RegisterClassExW(&wc);
    HWND hwnd = CreateWindowExW(0, wc.lpszClassName, L"", WS_OVERLAPPEDWINDOW, 0, 0, 100, 100,
                                nullptr, nullptr, wc.hInstance, nullptr);
    if (!hwnd) return false;

    DXGI_SWAP_CHAIN_DESC desc{};
    desc.BufferCount = 1;
    desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.OutputWindow = hwnd;
    desc.SampleDesc.Count = 1;
    desc.Windowed = TRUE;
    desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    IDXGISwapChain* swapChain = nullptr;
    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* context = nullptr;
    const D3D_FEATURE_LEVEL level = D3D_FEATURE_LEVEL_11_0;
    HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, &level, 1,
                                               D3D11_SDK_VERSION, &desc, &swapChain, &device, nullptr,
                                               &context);
    bool ok = SUCCEEDED(hr) && swapChain;
    if (ok) {
        void** vtable = *reinterpret_cast<void***>(swapChain);
        *present = vtable[kPresentIndex];
        *resize = vtable[kResizeBuffersIndex];
    } else {
        log::Error("Dummy D3D11 swap chain creation failed (hr=0x%08lX)", hr);
    }

    if (swapChain) swapChain->Release();
    if (context) context->Release();
    if (device) device->Release();
    DestroyWindow(hwnd);
    UnregisterClassW(wc.lpszClassName, wc.hInstance);
    return ok;
}

}  // namespace

bool Install() {
    // The game creates its device itself; wait until d3d11/dxgi are actually loaded.
    for (int i = 0; i < 600 && !(GetModuleHandleW(L"d3d11.dll") && GetModuleHandleW(L"dxgi.dll")); ++i) {
        Sleep(100);
    }

    if (!FindSwapChainVTable(&g_presentTarget, &g_resizeTarget)) return false;
    log::Info("Present=%p ResizeBuffers=%p", g_presentTarget, g_resizeTarget);

    MH_STATUS st = MH_Initialize();
    if (st != MH_OK && st != MH_ERROR_ALREADY_INITIALIZED) {
        log::Error("MH_Initialize failed: %s", MH_StatusToString(st));
        return false;
    }
    if ((st = MH_CreateHook(g_presentTarget, &HookPresent, reinterpret_cast<void**>(&g_origPresent))) != MH_OK ||
        (st = MH_CreateHook(g_resizeTarget, &HookResizeBuffers, reinterpret_cast<void**>(&g_origResizeBuffers))) != MH_OK ||
        (st = MH_EnableHook(MH_ALL_HOOKS)) != MH_OK) {
        log::Error("Hooking swap chain failed: %s", MH_StatusToString(st));
        return false;
    }
    return true;
}

void Uninstall() {
    MH_DisableHook(MH_ALL_HOOKS);
    input::Uninstall();
    if (g_imguiReady) {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        g_imguiReady = false;
    }
    ReleaseRenderTarget();
}

}  // namespace core::render
