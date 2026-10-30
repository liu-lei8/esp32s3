idf_component_register( SRC_DIRS "."
                                 "APP/HUMAN_RECOGNITION"
                                 "APP/HUMAN_DETECT"
                                 "APP/CAT_DETECT"
                        INCLUDE_DIRS "."
                                     "APP/HUMAN_RECOGNITION"
                                     "APP/HUMAN_DETECT"
                                     "APP/CAT_DETECT"
                        REQUIRES
                            BSP
                            fatfs
                            bsp_atk_dnesp32s3m
                            espressif__human_face_recognition
                            espressif__human_face_detect
                            espressif__cat_detect
                            esp32-camera)
