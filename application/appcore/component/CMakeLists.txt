# Application Components
static_library(component)

target_sources(component 
    PRIVATE
    ${ZEPHYR_BASE}/misc/empty_file.c
)
add_subdirectory_ifdef(CONFIG_BT bt)