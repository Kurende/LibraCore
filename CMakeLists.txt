cmake_minimum_required(VERSION 3.5)

project(LibraCore VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Sql PrintSupport Charts)

set(PROJECT_SOURCES
    main.cpp

    ui/MainWindow.cpp
    ui/MainWindow.h
    ui/MainWindow.ui

    database/DatabaseManager.cpp
    database/DatabaseManager.h

    auth/AuthManager.cpp
    auth/AuthManager.h

    models/User.cpp
    models/User.h
    models/Learner.cpp
    models/Learner.h
    models/Book.cpp
    models/Book.h
    models/Transaction.cpp
    models/Transaction.h
    models/Payments.cpp
    models/Payments.h
    models/PaymentItem.cpp
    models/PaymentItem.h

    utils/Encryption.cpp
    utils/Encryption.h
)

set(PROJECT_RESOURCES
    resources.qrc
)

add_executable(LibraCore
    ${PROJECT_SOURCES}
    ${PROJECT_RESOURCES}
)

target_link_libraries(LibraCore PRIVATE
    Qt6::Widgets
    Qt6::Sql
    Qt6::PrintSupport
    Qt6::Charts
)

target_include_directories(LibraCore PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/database
    ${CMAKE_CURRENT_SOURCE_DIR}/auth
    ${CMAKE_CURRENT_SOURCE_DIR}/models
    ${CMAKE_CURRENT_SOURCE_DIR}/utils
)

set_target_properties(LibraCore PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

install(TARGETS LibraCore
    RUNTIME DESTINATION bin
)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/database/library_system.db")
    file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/database/library_system.db"
         DESTINATION "${CMAKE_BINARY_DIR}/bin")
endif()

if(WIN32)
    set_target_properties(LibraCore PROPERTIES
        WIN32_EXECUTABLE TRUE
    )

    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        find_program(WINDEPLOYQT_EXECUTABLE windeployqt
            HINTS "${Qt6_DIR}/../../../bin")
        if(WINDEPLOYQT_EXECUTABLE)
            add_custom_command(TARGET LibraCore POST_BUILD
                COMMAND ${WINDEPLOYQT_EXECUTABLE}
                        $<TARGET_FILE:LibraCore>
            )
        endif()
    endif()
endif()

message(STATUS "")
message(STATUS "LibraCore Configuration:")
message(STATUS "  Version:      ${PROJECT_VERSION}")
message(STATUS "  Qt Version:   ${Qt6Widgets_VERSION}")
message(STATUS "  Build Type:   ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "")
