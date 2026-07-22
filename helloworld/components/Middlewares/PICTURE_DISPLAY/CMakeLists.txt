list(APPEND srcs    piclib.c
                    bmp.c
                    gif.c
                    jpeg.c
                    tjpgd.c
                    png.c
                    pngle.c)

idf_component_register(SRCS ${srcs}
                    INCLUDE_DIRS "."
                    REQUIRES BSP
                             MYFATFS
                             fatfs
                    )