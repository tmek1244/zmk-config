/*
 * Low battery warning: while the keyboard is in use and the battery is at or
 * below CONFIG_LOSS_BATTERY_LED_THRESHOLD percent, briefly double-blink the
 * controller's on-board LED (P0.15) every CONFIG_LOSS_BATTERY_LED_INTERVAL_MS.
 * It stays dark while the keyboard is idle, so it doesn't drain the battery
 * overnight.
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include <zmk/activity.h>
#include <zmk/battery.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#define FLASH_MS 60
#define GAP_MS 150

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_NODELABEL(blue_led), gpios);

static void flash(void) {
    gpio_pin_set_dt(&led, 1);
    k_msleep(FLASH_MS);
    gpio_pin_set_dt(&led, 0);
}

static void battery_led_thread(void) {
    if (!gpio_is_ready_dt(&led) || gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE) < 0) {
        LOG_ERR("Battery LED: GPIO not available");
        return;
    }

    while (true) {
        k_msleep(CONFIG_LOSS_BATTERY_LED_INTERVAL_MS);

        uint8_t level = zmk_battery_state_of_charge();
        if (level > CONFIG_LOSS_BATTERY_LED_THRESHOLD ||
            zmk_activity_get_state() != ZMK_ACTIVITY_ACTIVE) {
            continue;
        }

        LOG_DBG("Battery low (%d%%), blinking LED", level);
        flash();
        k_msleep(GAP_MS);
        flash();
    }
}

K_THREAD_DEFINE(loss_battery_led, 512, battery_led_thread, NULL, NULL, NULL,
                K_LOWEST_APPLICATION_THREAD_PRIO, 0, 0);
