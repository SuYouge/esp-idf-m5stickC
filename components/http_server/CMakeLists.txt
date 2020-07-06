set(srcs "src/my_http_server.c")
set(COMPONENT_ADD_INCLUDEDIRS "include")
idf_component_register(SRCS "${srcs}"
                    INCLUDE_DIRS include
                    REQUIRES nghttp)