zephyr_library()

zephyr_library_include_directories(  
    ./
    ./Large/
    ./gh3011_code/
    ./zephyr_porting/
    ./HeartRate_srv/
    #./PAH_MAIN/

)

zephyr_library_sources_ifdef(CONFIG_GH3011
    ./gh3011_entry.c
    ./gh3011_code/gh3011_example_comm_pkg.c
    ./gh3011_code/gh3011_example_ctrl.c
    ./gh3011_code/gh3011_example_port.c
    ./gh3011_code/gh3011_example_process.c
    ./gh3011_code/gh3011_example_reg_array.c
    #./zephyr_porting/gh3011_i2c.c
    ./HeartRate_srv/hr_srv.c
)

if(CONFIG_GH3011)
zephyr_library_link_libraries(
    ${CMAKE_CURRENT_SOURCE_DIR}/Large/band_lib/gcc_hard_fpv4-sp-d16/lib_app_cmd_send_GCC_M4_hardfp.a
    ${CMAKE_CURRENT_SOURCE_DIR}/Large/band_lib/gcc_hard_fpv4-sp-d16/libhbd_communicate_cortexM4l.a
    ${CMAKE_CURRENT_SOURCE_DIR}/Large/band_lib/gcc_hard_fpv4-sp-d16/libhbd_ctrl_cortexM4l.a
    ${CMAKE_CURRENT_SOURCE_DIR}/Large/band_lib/gcc_hard_fpv4-sp-d16/TestLib_GCC_M4_hardfp.a
)
endif()
