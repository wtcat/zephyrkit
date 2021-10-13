# Components

zephyr_include_directories(
  include
  .
)

###############profile select########################

zephyr_library_sources_ifdef(CONFIG_BLE_PROFILES_ANPC
	  anpc/anpc_main.c)


zephyr_library_sources_ifdef(CONFIG_BLE_PROFILES_ATPC
	  atpc/atpc_main.c)
zephyr_library_sources_ifdef(CONFIG_BLE_PROFILES_ATPS
	  atps/atps_main.c)

zephyr_library_sources_ifdef(CONFIG_BLE_PROFILES_BAS
	  bas/bas_main.c)


zephyr_library_sources_ifdef(CONFIG_BLE_PROFILES_BLPC
	  blpc/blpc_main.c)
zephyr_library_sources_ifdef(CONFIG_BLE_PROFILES_BLPS
	  blps/blps_main.c)

zephyr_library_sources_ifdef(CONFIG_BLE_PROFILES_CPP
	  cpp/cpps_main.c)

zephyr_library_sources_ifdef(CONFIG_BLE_PROFILES_CSCP
	  cscp/cscps_main.c)

zephyr_library_sources_ifdef(CONFIG_BLE_PROFILES_DIS
	  dis/dis_main.c)

zephyr_library_sources_ifdef(CONFIG_BLE_PROFILES_FMPL
	  fmpl/fmpl_main.c)

zephyr_library_sources_ifdef(CONFIG_BLE_PROFILES_GAP
	  gap/gap_main.c)


zephyr_include_directories_ifdef(CONFIG_BLE_PROFILES_GATT
  gatt/
)
zephyr_library_sources_ifdef(CONFIG_BLE_PROFILES_GATT
	gatt/gatt_main.c)

zephyr_library_sources_ifdef(CONFIG_BLE_PROFILES_GLPC
	glpc/glpc_main.c)
zephyr_library_sources_ifdef(CONFIG_BLE_PROFILES_GLPS
	glps/glps_main.c
	glps/glps_db.c)

zephyr_library_sources_ifdef(CONFIG_BLE_PROFILES_HID
	hid/hid_main.c)

zephyr_library_sources_ifdef(CONFIG_BLE_PROFILES_HRPC
	hrpc/hrpc_main.c)
zephyr_library_sources_ifdef(CONFIG_BLE_PROFILES_HRPS
	hrps/hrps_main.c)

zephyr_library_sources_ifdef(CONFIG_BLE_PROFILES_HTPC
	htpc/htpc_main.c)
zephyr_library_sources_ifdef(CONFIG_BLE_PROFILES_HTPS
	htps/htps_main.c)

zephyr_library_sources_ifdef(CONFIG_BLE_PROFILES_PASPC
	paspc/paspc_main.c)

zephyr_library_sources_ifdef(CONFIG_BLE_PROFILES_PLXPC
	plxpc/plxpc_main.c)
zephyr_library_sources_ifdef(CONFIG_BLE_PROFILES_PLXPS
	plxps/plxps_main.c
	plxps/plxps_db.c)

zephyr_library_sources_ifdef(CONFIG_BLE_PROFILES_RSCP
	rscp/rscps_main.c)

zephyr_library_sources_ifdef(CONFIG_BLE_PROFILES_SCPPS
	scpps/scpps_main.c)

zephyr_library_sources_ifdef(CONFIG_BLE_PROFILES_SENSOR
	sensor/gyro_main.c
	sensor/temp_main.c)

zephyr_library_sources_ifdef(CONFIG_BLE_PROFILES_TIPC
	tipc/tipc_main.c)

zephyr_library_sources_ifdef(CONFIG_BLE_PROFILES_UDSC
	udsc/udsc_main.c)

zephyr_library_sources_ifdef(CONFIG_BLE_PROFILES_URIBEACON
	uribeacon/uricfg_main.c)

zephyr_library_sources_ifdef(CONFIG_BLE_PROFILES_WDXC
	wdxc/wdxc_main.c
	wdxc/wdxc_ft.c
	wdxc/wdxc_stream.c)

zephyr_library_sources_ifdef(CONFIG_BLE_PROFILES_WDXS
	wdxs/wdxs_main.c
	wdxs/wdxs_ft.c
	wdxs/wdxs_au.c
	wdxs/wdxs_dc.c
	wdxs/wdxs_phy.c
	wdxs/wdxs_stream.c)

zephyr_library_sources_ifdef(CONFIG_BLE_PROFILES_WPC
	wpc/wpc_main.c)

zephyr_library_sources_ifdef(CONFIG_BLE_PROFILES_WSPC
	wspc/wspc_main.c)

zephyr_library_sources_ifdef(CONFIG_BLE_PROFILES_WSPS
	wsps/wsps_main.c)

