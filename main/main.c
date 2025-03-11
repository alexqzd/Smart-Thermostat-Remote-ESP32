#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "rom/gpio.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "lvgl.h"
#include "board.h"
#include "esp_timer.h"
#include "screen.h" // Asegúrate de incluir screen.h

#define TAG "MAIN"

static void increase_lvgl_tick(void* arg) {
    lv_tick_inc(portTICK_PERIOD_MS);
}

extern lv_display_t* screen_init(void);
extern lv_indev_t* get_encoder_indev(void);

static lv_indev_t* encoder_indev;
static lv_display_t* display;
static lv_obj_t *label; // Declarar la etiqueta globalmente

static void arc_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *arc = lv_event_get_target(e);

    if (code == LV_EVENT_VALUE_CHANGED) {
        int value = lv_arc_get_value(arc);
        printf("Arc value: %d\n", value);
        char buf[8];
        snprintf(buf, sizeof(buf), "%d°C", value); // Usar el símbolo de grados en UTF-8
        lv_label_set_text(label, buf); // Actualizar el texto de la etiqueta
    }
}

static void encoder_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    // lv_obj_t *arc = lv_event_get_target(e);

    if (code == LV_EVENT_PRESSED) {
        printf("Returning to menu...\n");
        //load_previous_screen(); // Implement this to go back
    }
}

void lvgl_task(void* arg) {
    display = screen_init();
    encoder_indev = get_encoder_indev();

    // Tick interface for LVGL
    const esp_timer_create_args_t periodic_timer_args = {
        .callback = increase_lvgl_tick,
        .name = "periodic_gui"
    };
    esp_timer_handle_t periodic_timer;
    esp_timer_create(&periodic_timer_args, &periodic_timer);
    esp_timer_start_periodic(periodic_timer, portTICK_PERIOD_MS * 1000);

    lv_obj_t *arc = lv_arc_create(lv_scr_act());
    lv_arc_set_range(arc, 16, 30);
    lv_arc_set_value(arc, 24);

    // Crear la etiqueta y configurarla
    label = lv_label_create(lv_scr_act());

    // use big font size
    lv_label_set_text(label, "24°C"); // Usar el símbolo de grados en UTF-8
    lv_obj_center(label);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_48, LV_PART_MAIN);
    lv_obj_add_event_cb(arc, arc_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(arc, encoder_event_handler, LV_EVENT_PRESSED, NULL);

    lv_obj_set_size(arc, 400, 400);
    lv_obj_center(arc);

    lv_group_t *g = lv_group_create();
    lv_group_add_obj(g, arc);

    lv_indev_set_group(encoder_indev, g);
    
    lv_group_focus_obj(arc);
    lv_group_set_editing(g, true);

    for (;;) {
        lv_task_handler();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void app_main(void) {
    xTaskCreatePinnedToCore(lvgl_task, NULL, 8 * 1024, NULL, 5, NULL, 1);
}
