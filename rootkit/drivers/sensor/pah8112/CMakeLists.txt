zephyr_library()

zephyr_library_include_directories(
    ./
    ./PAH_MAIN
    ./PAH8112
    ./PAH8112/verify/
    ./PxiAlg/HR/v5551/
    ./PxiAlg/HR/v114009_hr/
    ./PxiAlg/SPO2/v571/
    ./zephyr_porting/

)
zephyr_library_sources_ifdef(CONFIG_PAH8112
    ./pah8112_entry.c
    ./zephyr_porting/i2c.c
    ./zephyr_porting/accelerometer.c
    ./PAH8112/pah_8112_internal.c
    ./PAH8112/pah_comm_i2c.c
    ./PAH8112/pah_driver_8112_reg_array.c
    ./PAH8112/pah_driver_8112.c
    #./PAH8112/verify/pah_verify_8112.c
    #./PAH8112/pah_comm_others/pah_comm_i2c_length_limit.c
    #./PAH_MAIN/pah_factory_test.c
    ./PAH_MAIN/pah_hrd_function.c
    ./PAH_MAIN/pah_spo2_function_v2.c
)

if(CONFIG_PAH8112)
zephyr_library_link_libraries(
    ${CMAKE_CURRENT_SOURCE_DIR}/PxiAlg/HR/v5551/libpah8series_motion5551_9_m4f_gcc_hard_os.a
    ${CMAKE_CURRENT_SOURCE_DIR}/PxiAlg/HR/v114009_hr/libpaw8001_ofn114009_12_m4f_hard_gcc.a
    ${CMAKE_CURRENT_SOURCE_DIR}/PxiAlg/SPO2/v571/libpxialg_spo2_v571_25hz_2p_10_m4f_hard_gcc.a
)
endif()
