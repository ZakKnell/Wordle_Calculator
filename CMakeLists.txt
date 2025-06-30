cmake_minimum_required(VERSION 3.31)
project(Wordle_Calculator)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_AUTOMOC ON)

add_executable(Wordle_Calculator main.cpp WordleWindow.cpp)

find_package(Qt6 COMPONENTS Widgets REQUIRED)
target_link_libraries(Wordle_Calculator PRIVATE Qt6::Widgets)
target_compile_definitions(Wordle_Calculator PRIVATE USE_QT)