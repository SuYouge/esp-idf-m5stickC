set(srcs "src/decode.c" "src/identify.c" "src/quirc.c" "src/version_db.c")
set(COMPONENT_ADD_INCLUDEDIRS "include")
idf_component_register(SRCS "${srcs}"
                    INCLUDE_DIRS include)