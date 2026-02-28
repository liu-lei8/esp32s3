set(scr_dirs
            24CXX
            XL9555
            IIC
            WDT
            SW_HW_PWM
            GPTIM_NEW
            GPTIM_OLD
            ESPTIM
            UART
            EXIT
            KEY
            LED)

set(include_dirs
            24CXX
            XL9555
            IIC
            WDT
            SW_HW_PWM
            GPTIM_NEW
            GPTIM_OLD
            ESPTIM
            UART
            EXIT
            KEY
            LED)

set(requires
            esp_timer
            driver)

idf_component_register(SRC_DIRS ${scr_dirs} INCLUDE_DIRS ${include_dirs} 
                        REQUIRES ${requires})

component_compile_options(-ffast-math -O3 -Wno-error=format=-Wno-format)