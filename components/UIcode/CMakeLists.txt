# set(srcs "src/example-3d-hello.cpp" "src/delay.cpp")
# set(srcs "src/example-star-hello.cpp" "src/delay.cpp")
# set(srcs "src/example-particle.cpp" "src/delay.cpp" "src/resources/Microsoft_YaHei_28.cpp")
# set(srcs "src/example-3d-wave.cpp" "src/delay.cpp")
# set(srcs "src/example-circle.cpp" "src/delay.cpp")
set(srcs "src/example-molecular.cpp" "src/delay.cpp")

set(COMPONENT_ADD_INCLUDEDIRS "include")
idf_component_register(SRCS "${srcs}"
    INCLUDE_DIRS include)