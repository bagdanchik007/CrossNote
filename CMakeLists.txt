cmake_minimum_required(VERSION 3.21)

project(CrossNote VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Quick QuickControls2 Sql)
qt_standard_project_setup(REQUIRES 6.5)
if (QT_KNOWN_POLICY_QTP0004)
    qt_policy(SET QTP0004 NEW)
endif()

add_library(crossnote_data STATIC
    src/data/NoteRepository.cpp
    src/data/NoteRepository.h
    src/domain/Note.h
)
target_include_directories(crossnote_data PUBLIC src)
target_link_libraries(crossnote_data PUBLIC Qt6::Core Qt6::Sql)

qt_add_executable(crossnote
    src/main.cpp
    src/application/NoteController.cpp
    src/application/NoteController.h
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
target_link_libraries(crossnote PRIVATE crossnote_data Qt6::Quick Qt6::QuickControls2)

target_compile_options(crossnote PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
    $<$<CXX_COMPILER_ID:MSVC>:/permissive->
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wextra>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wpedantic>
)

if (WIN32 AND CMAKE_BUILD_TYPE STREQUAL "Release")
    set_property(TARGET crossnote PROPERTY WIN32_EXECUTABLE TRUE)
endif()

include(CTest)
if (BUILD_TESTING)
    find_package(Qt6 6.5 REQUIRED COMPONENTS Test)
    qt_add_executable(crossnote_note_creation_tests
        tests/NoteCreationTest.cpp
    )
    target_link_libraries(crossnote_note_creation_tests PRIVATE crossnote_data Qt6::Test)
    add_test(NAME crossnote_note_creation COMMAND crossnote_note_creation_tests)

    qt_add_executable(crossnote_controller_tests
        tests/NoteControllerTest.cpp
        src/application/NoteController.cpp
        src/application/NoteController.h
        src/presentation/NoteListModel.cpp
        src/presentation/NoteListModel.h
    )
    target_include_directories(crossnote_controller_tests PRIVATE src)
    target_link_libraries(crossnote_controller_tests PRIVATE crossnote_data Qt6::Core Qt6::Test)
    add_test(NAME crossnote_controller COMMAND crossnote_controller_tests)

    qt_add_executable(crossnote_note_list_model_tests
        tests/NoteListModelTest.cpp
        src/presentation/NoteListModel.cpp
        src/presentation/NoteListModel.h
    )
    target_include_directories(crossnote_note_list_model_tests PRIVATE src)
    target_link_libraries(crossnote_note_list_model_tests PRIVATE Qt6::Core Qt6::Test)
    add_test(NAME crossnote_note_list_model COMMAND crossnote_note_list_model_tests)
endif()
