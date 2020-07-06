set(COMPONENT_SRCS main.cpp )
set(COMPONENT_ADD_INCLUDEDIRS ".")
register_component(
    REQUIRES SSD1306
)
spiffs_create_partition_image(storage ../font FLASH_IN_PROJECT)