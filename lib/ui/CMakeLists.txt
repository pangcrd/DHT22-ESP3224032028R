SET(SOURCES ui_Screen1.c
    ui.c
    ui_comp_hook.c
    ui_helpers.c
    ui_events.c
    ui_img_temperature_png.c
    ui_img_humidity_png.c
    fonts/ui_font_Zendots22.c)

add_library(ui ${SOURCES})
