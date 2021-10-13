# Install script for directory: /home/wt/zephyr-kit/zephyr_pkg/zephyr

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "TRUE")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/home/wt/zephyr-kit/project_wkpace/build/zephyr/arch/cmake_install.cmake")
  include("/home/wt/zephyr-kit/project_wkpace/build/zephyr/lib/cmake_install.cmake")
  include("/home/wt/zephyr-kit/project_wkpace/build/zephyr/soc/arm/cmake_install.cmake")
  include("/home/wt/zephyr-kit/project_wkpace/build/zephyr/boards/cmake_install.cmake")
  include("/home/wt/zephyr-kit/project_wkpace/build/zephyr/subsys/cmake_install.cmake")
  include("/home/wt/zephyr-kit/project_wkpace/build/zephyr/drivers/cmake_install.cmake")
  include("/home/wt/zephyr-kit/project_wkpace/build/modules/gui/cmake_install.cmake")
  include("/home/wt/zephyr-kit/project_wkpace/build/modules/subsys/cmake_install.cmake")
  include("/home/wt/zephyr-kit/project_wkpace/build/modules/testsuite/cmake_install.cmake")
  include("/home/wt/zephyr-kit/project_wkpace/build/modules/drivers/cmake_install.cmake")
  include("/home/wt/zephyr-kit/project_wkpace/build/modules/drivers_ext/cmake_install.cmake")
  include("/home/wt/zephyr-kit/project_wkpace/build/modules/components/cmake_install.cmake")
  include("/home/wt/zephyr-kit/project_wkpace/build/modules/fatfs/cmake_install.cmake")
  include("/home/wt/zephyr-kit/project_wkpace/build/modules/littlefs/cmake_install.cmake")
  include("/home/wt/zephyr-kit/project_wkpace/build/modules/nffs/cmake_install.cmake")
  include("/home/wt/zephyr-kit/project_wkpace/build/modules/segger/cmake_install.cmake")
  include("/home/wt/zephyr-kit/project_wkpace/build/modules/tinycrypt/cmake_install.cmake")
  include("/home/wt/zephyr-kit/project_wkpace/build/zephyr/kernel/cmake_install.cmake")
  include("/home/wt/zephyr-kit/project_wkpace/build/zephyr/cmake/flash/cmake_install.cmake")
  include("/home/wt/zephyr-kit/project_wkpace/build/zephyr/cmake/usage/cmake_install.cmake")
  include("/home/wt/zephyr-kit/project_wkpace/build/zephyr/cmake/reports/cmake_install.cmake")

endif()

