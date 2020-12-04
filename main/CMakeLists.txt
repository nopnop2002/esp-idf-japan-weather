set(COMPONENT_SRCS main.c ili9340.c fontx.c m5stack.c)
set(COMPONENT_ADD_INCLUDEDIRS ".")

register_component()

# Create a SPIFFS image from the contents of the 'fonts' directory
# that fits the partition named 'storage'. FLASH_IN_PROJECT indicates that
# the generated image should be flashed when the entire project is flashed to
# the target with 'idf.py -p PORT flash
spiffs_create_partition_image(storage ../fonts FLASH_IN_PROJECT)

