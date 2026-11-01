include(FetchContent)
# Immutable commits plus archive hashes: no floating dependency branches.
FetchContent_Declare(imgui
    URL https://codeload.github.com/ocornut/imgui/tar.gz/97428e8ac99e339ce05eee531cf55b77b29ea709
    URL_HASH SHA256=e7881abd9544100f2aa66fcee80e92f18a15b74455c33334613d8c965922c53d
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_Declare(json
    URL https://codeload.github.com/nlohmann/json/tar.gz/9cca280a4d0ccf0c08f47a99aa71d1b0e52f8d03
    URL_HASH SHA256=0dbc5e40a01ff142e7e68c03e85247a4dcede2f592d12d3677dee3664d17975a
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_Declare(googletest
    URL https://codeload.github.com/google/googletest/tar.gz/b514bdc898e2951020cbdca1304b75f5950d1f59
    URL_HASH SHA256=9257316d65d6259f6596bc4fa5464092305ab3f3d9dc6d7e5780ab462d8baa64
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_Declare(harfbuzz
    URL https://codeload.github.com/harfbuzz/harfbuzz/tar.gz/7b27c8edd46c674e01dd226fa9e1aa7549f5c436
    URL_HASH SHA256=818ac15af42769d5806d5f53077664976790264a8fd9161e135904b92ca9a923
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(gtest_force_shared_crt OFF CACHE BOOL "" FORCE)
set(HB_BUILD_SUBSET OFF CACHE BOOL "" FORCE)
set(HB_BUILD_UTILS OFF CACHE BOOL "" FORCE)
set(HB_HAVE_UNISCRIBE OFF CACHE BOOL "" FORCE)
set(HB_HAVE_DIRECTWRITE OFF CACHE BOOL "" FORCE)
set(HB_HAVE_GDI OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(imgui json googletest harfbuzz)
add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp)
target_include_directories(imgui SYSTEM PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
target_link_libraries(imgui PUBLIC d3d11)
target_compile_definitions(imgui PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
