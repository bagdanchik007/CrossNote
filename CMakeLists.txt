cmake_minimum_required(VERSION 3.21)

project(CrossNote VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 6.5 REQUIRED COMPONENTS Quick)
qt_standard_project_setup(REQUIRES 6.5)

qt_add_executable(crossnote
    src/main.cpp
)

qt_add_qml_module(crossnote
    URI CrossNote
    VERSION 1.0
    QML_FILES
        qml/Main.qml
)

target_compile_options(crossnote PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
    $<$<CXX_COMPILER_ID:MSVC>:/permissive->
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wextra>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wpedantic>
)

if (WIN32)
    set_property(TARGET crossnote PROPERTY WIN32_EXECUTABLE TRUE)
endif()
