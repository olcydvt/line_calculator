cmake_minimum_required (VERSION 3.8)
project (task_line_calculation)
set(CMAKE_CXX_STANDARD 20)

add_executable (task_line_calculation task_line_calculation.cpp)

target_link_libraries(task_line_calculation PUBLIC pthread)