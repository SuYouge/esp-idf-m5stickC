set(srcs "src/UIcode.cpp" "src/delay.cpp")
set(COMPONENT_ADD_INCLUDEDIRS "include")
idf_component_register(SRCS "${srcs}"
    INCLUDE_DIRS include)
# target_link_libraries(${COMPONENT} delay.c)