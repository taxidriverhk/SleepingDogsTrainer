cmake_minimum_required(VERSION 3.24)
project(SleepingDogsTrainer LANGUAGES C CXX)

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "Sleeping Dogs: Definitive Edition is 64-bit; configure an x64 build.")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Static CRT: the DLL must not depend on a VC++ redistributable being installed.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

include(FetchContent)

FetchContent_Declare(minhook
    GIT_REPOSITORY https://github.com/TsudaKageyu/minhook.git
    GIT_TAG        v1.3.3
    GIT_SHALLOW    TRUE)
FetchContent_Declare(imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.91.5
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(minhook imgui)

# --- MinHook (built from source; v1.3.3 predates its own CMake support) ---
add_library(minhook_lib STATIC
    ${minhook_SOURCE_DIR}/src/buffer.c
    ${minhook_SOURCE_DIR}/src/hook.c
    ${minhook_SOURCE_DIR}/src/trampoline.c
    ${minhook_SOURCE_DIR}/src/hde/hde64.c)
target_include_directories(minhook_lib PUBLIC ${minhook_SOURCE_DIR}/include)

# --- Dear ImGui with the Win32 + DX11 backends ---
add_library(imgui_lib STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp)
target_include_directories(imgui_lib PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
target_compile_definitions(imgui_lib PUBLIC IMGUI_DISABLE_OBSOLETE_FUNCTIONS)

# --- The trainer, built as a proxy for xinput9_1_0.dll ---
add_library(sdtrainer SHARED
    src/dllmain.cpp
    src/proxy/xinput_proxy.cpp
    src/proxy/exports.def
    src/core/config.cpp
    src/core/input.cpp
    src/core/log.cpp
    src/core/menu.cpp
    src/core/nav.cpp
    src/core/pattern.cpp
    src/core/render_hook.cpp
    src/features/features.cpp
    src/game/game.cpp)
set_target_properties(sdtrainer PROPERTIES OUTPUT_NAME "xinput9_1_0")
target_include_directories(sdtrainer PRIVATE src)
target_compile_definitions(sdtrainer PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE)
target_compile_options(sdtrainer PRIVATE /W4 /permissive- /utf-8)
target_link_libraries(sdtrainer PRIVATE minhook_lib imgui_lib d3d11 dxgi)

# Optional: copy the DLL into the game folder after each build.
#   cmake -B build -DSDT_GAME_DIR="G:/SteamLibrary/steamapps/common/SleepingDogsDefinitiveEdition"
set(SDT_GAME_DIR "" CACHE PATH "Sleeping Dogs DE install folder to copy the built DLL into")
if(SDT_GAME_DIR)
    add_custom_command(TARGET sdtrainer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:sdtrainer> "${SDT_GAME_DIR}"
        COMMENT "Copying xinput9_1_0.dll to ${SDT_GAME_DIR}")
endif()
