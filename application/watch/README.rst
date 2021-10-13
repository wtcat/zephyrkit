cmsis:
    CONFIG_SOC_APOLLO1
    CONFIG_SOC_APOLLO2 
    CONFIG_SOC_APOLLO3
    CONFIG_SOC_APOLLO3P

mcu:
    CONFIG_HAS_APOLLO_UTILS
    CONFIG_HAS_APOLLO_LIB
    CONFIG_SOC_APOLLO3P
    
    
    
build steps:

mkdir build
cd build
cmake -DBOARD=board_name ..

--board_name = apollo3p, ama3b2evb
