set(srcs "src/bitmap.c" "src/camera.c" "src/ov2640.c" "src/ov7725.c" "src/sccb.c" "src/twi.c" "src/wiring.c" "src/xclk.c" "src/led.c" "src/qr_recoginize.c")
set(COMPONENT_ADD_INCLUDEDIRS "include")
idf_component_register(SRCS "${srcs}"
                    INCLUDE_DIRS include)