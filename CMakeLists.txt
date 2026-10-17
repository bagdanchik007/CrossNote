cmake_minimum_required(VERSION 3.21)

project(CrossNote VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 6.5 REQUIRED COMPONENTS Quick QuickControls2 Sql)
qt_standard_project_setup(REQUIRES 6.5)

qt_add_executable(crossnote
    src/main.cpp
    src/application/NoteController.cpp
    src/application/NoteController.h
    src/data/NoteRepository.cpp
    src/data/NoteRepository.h
    src/domain/Note.h
    src/presentation/NoteListModel.cpp
    src/presentation/NoteListModel.h
)

qt_add_qml_module(crossnote
    URI CrossNote
    VERSION 1.0
    QML_FILES
        qml/Main.qml
)

target_include_directories(crossnote PRIVATE src)
target_link_libraries(crossnote PRIVATE Qt6::Quick Qt6::QuickControls2 Qt6::Sql)

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
