list(APPEND srcs    fonts.c text.c unicode_gbk.c)

idf_component_register(SRCS ${srcs}
                       INCLUDE_DIRS "."
                       REQUIRES BSP
                                spi_flash
                                esp_partition
                                fatfs
                    )