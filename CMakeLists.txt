cmake_minimum_required(VERSION 3.16)
project(GUIC_2.0 LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# windres 需要能在 src 目录中定位 resource.rc 引用的 .ico / .manifest
set(CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS} -I${CMAKE_CURRENT_SOURCE_DIR}/src")

add_executable(GUIC_2
    src/main.cpp
    src/windowproc.cpp
    src/i18n.cpp
    src/config/config.cpp
    src/utils/logger.cpp
    src/utils/network.cpp
    src/utils/renderer.cpp
    src/utils/utils.cpp
    src/utils/lunar.cpp
    src/utils/weather.cpp
    src/utils/http_server.cpp
    src/utils/ntp_server.cpp
    src/dialogs/dialog_countdown.cpp
    src/dialogs/dialog_message.cpp
    src/dialogs/dialog_settings.cpp
    src/dialogs/dialog_special.cpp
    src/resource.rc
)

target_compile_options(GUIC_2 PRIVATE -std=gnu++14 -m64)
target_link_options(GUIC_2 PRIVATE -static -mwindows)
target_link_libraries(GUIC_2
    ws2_32 ole32 gdi32 comdlg32 uuid oleaut32
)

# 构建产物统一输出到 build/ 目录（与 Code::Blocks 的 build/GUIC_2.exe 保持同一位置）
set_target_properties(GUIC_2 PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build"
)