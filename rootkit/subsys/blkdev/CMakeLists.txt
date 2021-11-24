
zephyr_library_named(bdbuf)

zephyr_library_sources(
    bdbuf.c
    blkdev.c
    blkdev_flash.c
    blkdev_partition.c
    diskdevs.c
    blkdev_shell.c
)