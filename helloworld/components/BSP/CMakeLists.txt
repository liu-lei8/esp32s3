set(scr_dirs
            DS18B20
            TEMPERATURE_SENSOR
            REMOTE
            AP3216C
            ADC
            RTC
            SPI
            LCD
            OLED
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
            DS18B20
            TEMPERATURE_SENSOR
            REMOTE
            AP3216C
            ADC
            RTC
            SPI
            LCD
            OLED
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
            esp_adc
            newlib
            esp_lcd
            esp_timer
            driver)
#[[
1.newlib依赖库用于BSP中的RTC(是一个专为嵌入式系统设计的、开源的、轻量级C标准库实现)
]]

idf_component_register(SRC_DIRS ${scr_dirs} INCLUDE_DIRS ${include_dirs} 
                        REQUIRES ${requires})

component_compile_options(-ffast-math -O3 -Wno-error=format=-Wno-format)