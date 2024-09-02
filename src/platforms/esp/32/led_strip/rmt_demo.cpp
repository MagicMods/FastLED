/// @file    Blink.ino
/// @brief   Blink the first LED of an LED strip
/// @example Blink.ino

#ifdef ESP32

#include "rmt_demo.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/rmt_tx.h"
#include "led_strip.h"
#include "enabled.h"
#include "esp_log.h"
#include "esp_err.h"
#include "led_strip_rmt.h"



#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
#include "esp_private/esp_clk.h"

#endif

// GPIO assignment
#define LED_STRIP_BLINK_GPIO  2
// Numbers of the LED in the strip
#define LED_STRIP_LED_NUMBERS 24
// 10MHz resolution, 1 tick = 0.1us (led strip needs a high resolution)
#define LED_STRIP_RMT_RES_HZ  (10 * 1000 * 1000)

static const char *TAG = "example";



led_strip_config_t make_config(
      int strip_gpio_num,
      uint32_t max_leds,
      led_pixel_format_t led_pixel_format,
      led_model_t led_model,
      uint32_t invert_out) {
    // Note this is the idf 5.1 code with the idf 4.4 code stripped out.
    led_strip_config_t config = {};
    config.strip_gpio_num = strip_gpio_num;
    config.max_leds = max_leds;
    config.led_pixel_format = led_pixel_format;
    config.led_model = led_model;
    config.flags.invert_out = invert_out;
    return config;
}

led_strip_rmt_config_t make_rmt_config(rmt_clock_source_t clk_src, uint32_t resolution_hz, size_t mem_block_symbols, bool with_dma) {
    // assume #if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    led_strip_rmt_config_t config = {};
    #if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)
    config.rmt_channel = rmt_channel;
    #else
    config.clk_src = clk_src;
    config.resolution_hz = resolution_hz;
    #endif
    config.mem_block_symbols = mem_block_symbols;
    config.flags.with_dma = with_dma;
    return config;
}

led_strip_handle_t configure_led(void)
{
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0) || FASTLED_ESP32_COMPONENT_LED_STRIP_FORCE_IDF4
    led_strip_config_t strip_config = make_config(LED_STRIP_BLINK_GPIO, LED_STRIP_LED_NUMBERS, LED_PIXEL_FORMAT_GRB, LED_MODEL_WS2812, 0);
    led_strip_rmt_config_t rmt_config = make_rmt_config(RMT_CLK_SRC_DEFAULT, LED_STRIP_RMT_RES_HZ, 0, false);

    // LED Strip object handle
    led_strip_handle_t led_strip;
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    ESP_LOGI(TAG, "Created LED strip object with RMT backend");
    return led_strip;
#else
    // For older ESP-IDF versions, use the appropriate LED strip initialization
    led_strip_config_t strip_config = {
        .strip_gpio_num = LED_STRIP_BLINK_GPIO,
        .max_leds = LED_STRIP_LED_NUMBERS,
    };
    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = LED_STRIP_RMT_RES_HZ,
    };
    led_strip_handle_t led_strip;
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    ESP_LOGI(TAG, "Created LED strip object with RMT backend");
    return led_strip;
#endif
}

void __demo() {
    led_strip_handle_t led_strip = configure_led();
    bool led_on_off = false;

    ESP_LOGI(TAG, "Start blinking LED strip");
    while (1) {
        if (led_on_off) {
            /* Set the LED pixel using RGB from 0 (0%) to 255 (100%) for each color */
            for (int i = 0; i < LED_STRIP_LED_NUMBERS; i++) {
                ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, i, 5, 5, 5));
            }
            /* Refresh the strip to send data */
            ESP_ERROR_CHECK(led_strip_refresh(led_strip));
            ESP_LOGI(TAG, "LED ON!");
        } else {
            /* Set all LED off to clear all pixels */
            ESP_ERROR_CHECK(led_strip_clear(led_strip));
            ESP_LOGI(TAG, "LED OFF!");
        }

        led_on_off = !led_on_off;
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}


void rmt_demo() {
    __demo();
}

#endif  // ESP32
