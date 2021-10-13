
zephyr_include_directories(common)

zephyr_compile_definitions(
  -DSEC_ECC_CFG=SEC_ECC_CFG_UECC
)

zephyr_library_sources(
  common/sec_aes.c
  common/sec_aes_rev.c
  common/sec_ccm_hci.c
  common/sec_cmac_hci.c
  common/sec_ecc_debug.c
  common/sec_ecc_hci.c
  common/sec_main.c

#  pal/sec_ccm.c
  
  uecc/sec_ecc.c
)

