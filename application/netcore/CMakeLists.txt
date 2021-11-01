# Application
cmake_minimum_required(VERSION 3.13.1)

include(../cmake/modules.cmake)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(netcore)

#=======================
# User extension
#=======================
include(../cmake/extensions.cmake)
include(../cmake/debugger.cmake)

#=======================
# Global include path
#=======================
zephyr_include_directories(.)


#===========================
# Subdirectories and target
#===========================
target_sources(app PRIVATE
  src/main.c
)
collect_link_libraries(applibs app)
target_link_libraries(app PUBLIC ${applibs})


