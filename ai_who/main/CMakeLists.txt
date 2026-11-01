idf_component_register( SRC_DIRS "."
                                 "APP/HUMAN_RECOGNITION"
                                 "APP/HUMAN_DETECT"
                                 "APP/CAT_DETECT"
                                 "APP/QR_CODE"
                        INCLUDE_DIRS "."
                                     "APP/HUMAN_RECOGNITION"
                                     "APP/HUMAN_DETECT"
                                     "APP/CAT_DETECT"
                                     "APP/QR_CODE"
                        REQUIRES
                            BSP
                            fatfs
                            bsp_atk_dnesp32s3m
                            espressif__human_face_recognition
                            espressif__human_face_detect
                            espressif__cat_detect
                            espressif__quirc
                            esp32-camera)
