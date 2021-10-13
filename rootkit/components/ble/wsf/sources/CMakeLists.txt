# Zephyr Port


zephyr_include_directories(port/zephyr)

zephyr_library_include_directories(
  util
  .
)

zephyr_library_sources(
  port/zephyr/wsf_assert.c
  port/zephyr/wsf_buf.c
  port/zephyr/wsf_efs.c
  port/zephyr/wsf_math.c
  port/zephyr/wsf_msg.c
  port/zephyr/wsf_os.c
  port/zephyr/wsf_queue.c
  port/zephyr/wsf_timer.c
  port/zephyr/wsf_trace.c
  port/zephyr/wsf_thread.c
  
  util/bda.c
  util/bstream.c
  util/calc128.c
  util/crc32.c
  util/print.c
  util/terminal.c
  util/wstr.c
)


