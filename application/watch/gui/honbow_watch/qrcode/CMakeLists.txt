cmake_add_library(qrcode)

target_sources(qrcode
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/qrcodegen.c
)

