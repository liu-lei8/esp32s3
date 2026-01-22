set(scr_dirs
            GPTIM_NEW
            GPTIM_OLD
            ESPTIM
            UART
            EXIT
            KEY
            LED)

set(include_dirs
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