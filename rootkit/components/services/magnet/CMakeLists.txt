# Components
zephyr_library_named(magnet)

zephyr_library_include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/libmag
)

zephyr_library_sources(
  magnet_svr.c
)

zephyr_library_link_libraries(
  ${CMAKE_CURRENT_SOURCE_DIR}/libmag/MagCali.a
)
 

