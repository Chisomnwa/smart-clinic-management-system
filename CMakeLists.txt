cmake_minimum_required(VERSION 3.21)

project(SCAPMS VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

include(CTest)

find_package(Qt6 6.0 REQUIRED COMPONENTS Core Widgets)

add_library(scapms_domain
    controllers/appointment_controller.cpp
    controllers/clinic_manager.cpp
    controllers/clinic_reports.cpp
    controllers/patient_algorithms.cpp
    data/clinic_database.cpp
    models/appointment.cpp
    models/emergency_patient.cpp
    models/follow_up_patient.cpp
    models/general_patient.cpp
    models/patient_record.cpp
    persistence/file_handler.cpp
)
target_include_directories(scapms_domain PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(scapms_domain PUBLIC Qt6::Core)

add_library(scapms_gui
    gui/main_window.cpp
    gui/main_window.h
)
target_link_libraries(scapms_gui PUBLIC scapms_domain Qt6::Widgets)

add_executable(scapms gui/main.cpp)
target_link_libraries(scapms PRIVATE scapms_gui)

if(BUILD_TESTING)
    find_package(Qt6 6.0 REQUIRED COMPONENTS Test)

    add_executable(test_domain_models
        tests/test_domain_models.cpp
    )
    target_link_libraries(test_domain_models PRIVATE scapms_domain Qt6::Test)
    add_test(NAME domain_models COMMAND test_domain_models)

    add_executable(test_phase3
        tests/test_phase3.cpp
    )
    target_link_libraries(test_phase3 PRIVATE scapms_domain Qt6::Test)
    add_test(NAME phase3_features COMMAND test_phase3)

    add_executable(test_persistence
        tests/test_persistence.cpp
    )
    target_link_libraries(test_persistence PRIVATE scapms_domain Qt6::Test)
    add_test(NAME persistence COMMAND test_persistence)

    add_executable(test_gui_integration
        tests/test_gui_integration.cpp
    )
    target_link_libraries(test_gui_integration PRIVATE scapms_gui Qt6::Test)
    add_test(NAME gui_integration COMMAND test_gui_integration)
    set_tests_properties(gui_integration PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endif()
