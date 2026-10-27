cmake_minimum_required(VERSION 3.16)
project(QtCalculator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

if (MSVC)
    add_compile_options(/utf-8)
endif()

find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_add_executable(QtCalculator
    main.cpp
    MainWindow.cpp
    MainWindow.h
    CalcEngine.h
)

target_link_libraries(QtCalculator PRIVATE Qt6::Widgets)

if (WIN32)
    set_target_properties(QtCalculator PROPERTIES WIN32_EXECUTABLE ON)
endif()
