set(srcs "src/st7735s.c" "src/fontx.c")
set(COMPONENT_ADD_INCLUDEDIRS "include")
idf_component_register(SRCS "${srcs}"
                    INCLUDE_DIRS include
                    REQUIRES log spiffs)
