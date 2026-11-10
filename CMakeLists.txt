
cmake_minimum_required(VERSION 3.21)

project(QrApp VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

option(QR_BUILD_APP "Build the Qt GUI app" ON)
option(QR_BUILD_TESTS "Build the tests" ON)

function(qr_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive-)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

# -------- CORE LIBRARY --------

file(GLOB_RECURSE CORE_SRC CONFIGURE_DEPENDS
    src/core/*.cpp
    src/encoder/*.cpp
)

if(CORE_SRC)
    add_library(qrcore STATIC ${CORE_SRC})
    target_include_directories(qrcore PUBLIC 
        src/core
        src/encoder
    )
    qr_warnings(qrcore)
else()
    message(STATUS "No core source files found. Skipping qrcore.")
endif()

# -------- TESTS --------

if(QR_BUILD_TESTS)

    file(GLOB TEST_SRC CONFIGURE_DEPENDS
        tests/*.cpp
    )

    if(TEST_SRC AND TARGET qrcore)

        enable_testing()

        add_executable(qr_tests ${TEST_SRC})
        target_link_libraries(qr_tests PRIVATE qrcore)
        target_include_directories(qr_tests PRIVATE tests)
        qr_warnings(qr_tests)

        add_test(NAME qr_tests COMMAND qr_tests)

    else()
        message(STATUS "No test sources or core library found. Skipping tests.")
    endif()

endif()

# -------- QT GUI APPLICATION --------

if(QR_BUILD_APP)

    set(CMAKE_AUTOMOC ON)

    find_package(Qt6 REQUIRED COMPONENTS Widgets Multimedia)

    file(GLOB_RECURSE APP_SRC CONFIGURE_DEPENDS
        src/ui/*.cpp
        src/camera/*.cpp
        src/renderer/*.cpp
        src/main.cpp
    )

    if(APP_SRC)

        add_executable(qrapp ${APP_SRC})

        target_include_directories(qrapp PRIVATE 
        src
        src/renderer
        )

        target_link_libraries(qrapp PRIVATE
            Qt6::Widgets
            Qt6::Multimedia
        )

        if(TARGET qrcore)
            target_link_libraries(qrapp PRIVATE qrcore)
        endif()

        qr_warnings(qrapp)

    endif()

endif()
