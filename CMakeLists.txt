cmake_minimum_required(VERSION 3.24)
cmake_policy(SET CMP0091 NEW)
project(Superior VERSION 0.1.0 LANGUAGES C CXX)

if(NOT WIN32)
    message(FATAL_ERROR "Superior targets Windows 10/11. Build with the supplied GitHub Actions workflow.")
endif()
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON)
include(cmake/Dependencies.cmake)
include(CTest)

function(superior_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive- /utf-8 /EHsc /Zc:__cplusplus)
        target_compile_definitions(${target} PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX
            _WIN32_WINNT=0x0A00 WINVER=0x0A00
            _SILENCE_EXPERIMENTAL_COROUTINE_DEPRECATION_WARNINGS)
    endif()
endfunction()

add_library(superior_core STATIC
    src/core/config.cpp src/core/hotkey.cpp src/core/timing.cpp src/core/logging.cpp
    src/core/paths.cpp src/core/strings.cpp)
target_include_directories(superior_core PUBLIC include)
target_link_libraries(superior_core PUBLIC nlohmann_json::nlohmann_json)
superior_warnings(superior_core)

add_library(superior_text STATIC src/ui/text.cpp)
target_include_directories(superior_text PUBLIC include)
target_include_directories(superior_text SYSTEM PRIVATE ${imgui_SOURCE_DIR})
target_link_libraries(superior_text PUBLIC imgui harfbuzz usp10)
superior_warnings(superior_text)

add_executable(Superior WIN32
    src/app/main.cpp src/app/application.cpp src/app/window.cpp src/app/tray.cpp
    src/capture/capture.cpp src/capture/windows.cpp src/hardware/adapter.cpp
    src/present/device.cpp src/present/presenter.cpp
    src/ui/interface.cpp assets/superior.rc)
target_include_directories(Superior PRIVATE include)
target_link_libraries(Superior PRIVATE superior_core superior_text imgui
    d3d11 dxgi d3dcompiler dwmapi shell32 ole32 windowsapp runtimeobject)
superior_warnings(Superior)
set_target_properties(Superior PROPERTIES OUTPUT_NAME Superior)

# Compile HLSL in CI (or on a developer's Windows build machine); never at app startup.
file(GLOB_RECURSE SDK_FXC
    "C:/Program Files (x86)/Windows Kits/10/bin/*/x64/fxc.exe"
    "C:/Program Files/Windows Kits/10/bin/*/x64/fxc.exe")
if(SDK_FXC)
    list(SORT SDK_FXC COMPARE NATURAL ORDER DESCENDING)
    list(GET SDK_FXC 0 FXC_DEFAULT)
endif()
if(FXC_DEFAULT)
    set(FXC_EXECUTABLE "${FXC_DEFAULT}" CACHE FILEPATH "Windows SDK HLSL compiler")
else()
    find_program(FXC_EXECUTABLE NAMES fxc)
endif()
if(NOT FXC_EXECUTABLE)
    message(FATAL_ERROR "fxc.exe is required. Install the Windows 10/11 SDK (included on windows-latest).")
endif()
set(SHADER_OUTPUT "$<TARGET_FILE_DIR:Superior>/shaders/present.cso")
add_custom_command(TARGET Superior POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:Superior>/shaders"
    COMMAND "${FXC_EXECUTABLE}" /nologo /T cs_5_0 /E scale_cs /O3 /WX
        /Fo "${SHADER_OUTPUT}" "${CMAKE_CURRENT_SOURCE_DIR}/shaders/present.hlsl"
    COMMAND "${FXC_EXECUTABLE}" /nologo /T ps_5_0 /E scale_ps /O3 /WX
        /Fo "$<TARGET_FILE_DIR:Superior>/shaders/present_ps.cso" "${CMAKE_CURRENT_SOURCE_DIR}/shaders/present.hlsl"
    COMMAND "${FXC_EXECUTABLE}" /nologo /T vs_5_0 /E fullscreen_vs /O3 /WX
        /Fo "$<TARGET_FILE_DIR:Superior>/shaders/present_vs.cso" "${CMAKE_CURRENT_SOURCE_DIR}/shaders/present.hlsl"
    VERBATIM)

if(BUILD_TESTING)
    add_executable(superior_tests tests/config_test.cpp tests/timing_test.cpp tests/hotkey_test.cpp)
    target_link_libraries(superior_tests PRIVATE superior_core GTest::gtest_main)
    superior_warnings(superior_tests)
    include(GoogleTest)
    gtest_discover_tests(superior_tests DISCOVERY_TIMEOUT 30)
endif()

install(TARGETS Superior RUNTIME DESTINATION .)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/shaders/present.hlsl" DESTINATION shaders)
install(DIRECTORY "$<TARGET_FILE_DIR:Superior>/shaders/" DESTINATION shaders)
