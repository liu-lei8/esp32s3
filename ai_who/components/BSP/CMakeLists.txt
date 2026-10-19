set(scr_dirs
            CAMERA
            SPI
            LED
            IIC
            LCD
            XL9555)

set(include_dirs
            CAMERA
            SPI
            LED
            IIC
            LCD
            XL9555)

set(requires
            driver
            esp32-camera)


idf_component_register(SRC_DIRS ${scr_dirs}
                       INCLUDE_DIRS ${include_dirs} 
                       REQUIRES ${requires})

component_compile_options(-ffast-math -O3 -Wno-error=format -Wno-format)