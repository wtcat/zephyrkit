cmake_minimum_required(VERSION 3.8)

zephyr_include_directories(.)

# GUI custom widget 
zephyr_library_sources(
    guix_screenbase.c
    guix_checkbox.c
)




