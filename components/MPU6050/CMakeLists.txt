set(srcs "src/i2c.cpp" "src/kalmanfilter.cpp" "src/mpu6050.cpp")
set(COMPONENT_ADD_INCLUDEDIRS "include")
idf_component_register(SRCS "${srcs}"
    INCLUDE_DIRS include)