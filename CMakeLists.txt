cmake_minimum_required(VERSION 3.21)
project(WanShiTong VERSION 0.1.0
        DESCRIPTION "WYSIWYG-style note editor that stores Markdown"
        LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.2 REQUIRED COMPONENTS Widgets)

option(WANSHITONG_BUILD_TESTS "Build unit tests" ON)

add_subdirectory(src)

if(WANSHITONG_BUILD_TESTS)
    find_package(Qt6 6.2 REQUIRED COMPONENTS Test)
    enable_testing()
    add_subdirectory(tests)
endif()

include(GNUInstallDirs)
install(TARGETS WanShiTong RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES packaging/WanShiTong.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
