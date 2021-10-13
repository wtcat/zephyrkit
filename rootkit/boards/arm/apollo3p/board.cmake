# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=AMA3B2KK-KBR" "--speed=2000")

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
