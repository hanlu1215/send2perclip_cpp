cmake_minimum_required(VERSION 3.16)

project(send2perclip LANGUAGES CXX)

if(NOT WIN32)
    message(FATAL_ERROR "send2perclip 依赖 Windows 的键盘钩子与剪贴板 API，只能在 Windows 上构建。")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---------------------------------------------------------------- 静态链接
# MSVC：用静态 CRT（/MT、/MTd），不再依赖 MSVCP140.dll / VCRUNTIME140.dll
# 必须在 add_executable 之前设置：目标创建时会把该变量写进 MSVC_RUNTIME_LIBRARY 属性
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_executable(send2perclip
    src/main.cpp
    src/clipboard.cpp
    src/csv.cpp
    src/paste_watcher.cpp
    src/text.cpp
)

target_include_directories(send2perclip PRIVATE src)

# user32: 键盘钩子 / 剪贴板；kernel32（默认已包含）: Beep / Sleep 等
target_link_libraries(send2perclip PRIVATE user32)

# GCC / Clang（含 MinGW）：把 libstdc++ / libgcc / winpthread 一起静态链进 exe，
# 生成的程序不依赖 libstdc++-6.dll、libgcc_s_seh-1.dll，可直接拷到别的机器上跑。
# 注意：Windows 系统自带的 KERNEL32.dll / USER32.dll / ucrtbase.dll（或 msvcrt.dll）
#       仍然必须由系统提供，这部分无法（也不应该）静态链接。
if(NOT MSVC)
    target_link_options(send2perclip PRIVATE
        -static
        -static-libgcc
        -static-libstdc++
    )
endif()

target_compile_definitions(send2perclip PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    UNICODE
    _UNICODE
)

if(MSVC)
    # /utf-8：源码与执行字符集都用 UTF-8，保证中文字面量与日志输出正常
    target_compile_options(send2perclip PRIVATE /utf-8 /W4 /permissive-)
else()
    target_compile_options(send2perclip PRIVATE -Wall -Wextra)
endif()

# 若把源码目录加入 IDE 工程，运行时工作目录设为构建目录
set_target_properties(send2perclip PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)

# ---------------------------------------------------------------- 清理
# clean     : CMake 自带，只删可执行文件与目标文件，保留 CMake 配置（清完可直接再构建）
# distclean : 连 CMakeCache.txt 一起删（配置 + 产物全部清空），
#             清完必须重新执行 configure，例如：
#                 cmake --build build --target distclean
#                 cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
# 想彻底推倒重来（推荐）：直接删掉整个构建目录
#     Remove-Item -Recurse -Force build
add_custom_target(distclean
    COMMAND "${CMAKE_COMMAND}" -E rm -rf
            "${CMAKE_BINARY_DIR}/CMakeCache.txt"
            "${CMAKE_BINARY_DIR}/CMakeFiles"
            "${CMAKE_BINARY_DIR}/cmake_install.cmake"
            "${CMAKE_BINARY_DIR}/send2perclip.dir"
            "${CMAKE_BINARY_DIR}/send2perclip.exe"
    COMMENT "Removed CMakeCache.txt + CMakeFiles + binaries. Re-run configure before building again."
    VERBATIM
)
