idf_component_register( SRC_DIRS "."
                                 "APP/HUMAN_DETECT"
                        INCLUDE_DIRS "."
                                     "APP/HUMAN_DETECT"
                        REQUIRES
                            bsp_atk_dnesp32s3m
                            espressif__human_face_detect
                            esp32-camera)
