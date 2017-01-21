cmake_minimum_required (VERSION 3.1)
project (JB370737-PW-ZADANIE3 C)

#set(CMAKE_C_STANDARD ...)
set(CMAKE_C_FLAGS "-g -Wall")

# http://stackoverflow.com/questions/10555706/
macro (add_executable _name)
  # invoke built-in add_executable
  _add_executable(${ARGV})
  if (TARGET ${_name})
    target_link_libraries(${_name} err)
  endif()
endmacro()

add_library(err err.c)
add_library(node node.c)
add_library(parser parser.c)
add_library(vector vector.c)
add_library(int_vector int_vector.c)

add_library(main_process main_process.c)
target_link_libraries(main_process err int_vector)

add_executable(circuit circuit.c)
target_link_libraries(circuit node parser vector int_vector main_process pthread)

add_executable(node_process node_process.c)
target_link_libraries(node_process int_vector pthread)

install(TARGETS DESTINATION .)
