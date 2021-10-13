
#zephyr_library()

zephyr_compile_definitions(
	-DAM_HAL_DISABLE_API_VALIDATION
	-DAM_PACKAGE_BGA
)

set(ambiq_socs
	apollo
	apollo2
	apollo3
	apollo3p
)

if(CONFIG_CPU_CORTEX_M4)
	zephyr_compile_definitions( -DCORE_CM4 )
endif()

foreach(ambiq_soc ${ambiq_socs})
	string(TOUPPER ${ambiq_soc} soc_to_upper)
	if(CONFIG_SOC_${soc_to_upper})
		zephyr_include_directories(
		    ${ambiq_soc}
		    ${ambiq_soc}/regs
		    ${ambiq_soc}/hal
		)
		
		zephyr_compile_definitions(-DAM_PART_${soc_to_upper})

		add_subdirectory(${ambiq_soc})
	endif()
endforeach()
