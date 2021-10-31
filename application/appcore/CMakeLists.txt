# Application

cmake_minimum_required(VERSION 3.13.1)

include(../cmake/modules.cmake)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(firmware)

include(../cmake/debugger.cmake)

# Global include path
zephyr_include_directories(.)

add_subdirectory(component)
target_sources(app PRIVATE
  src/main.c
)
target_link_libraries(app 
  PUBLIC 
  component
)

