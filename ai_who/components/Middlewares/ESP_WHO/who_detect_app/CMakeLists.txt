set(src_dirs        .
                    ../who_app/who_app_common
                    ../who_app/who_app_common/who_detect_result_handle)

set(include_dirs    .
                    ../who_app/who_app_common
                    ../who_app/who_app_common/who_detect_result_handle)

idf_component_register(
    SRC_DIRS ${src_dirs}
    INCLUDE_DIRS ${include_dirs}
    REQUIRES bsp_atk_dnesp32s3m
)

# 强制使用无 LVGL 模式，避免引用 lv_obj_t 等 LVGL 类型
target_compile_definitions(${COMPONENT_LIB} PRIVATE BSP_CONFIG_NO_GRAPHIC_LIB=1)