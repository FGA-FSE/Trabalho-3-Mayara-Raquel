#include "buzzer.h"
#include "driver/gpio.h"

static int  s_gpio = -1;
static bool s_active_high = true;

void buzzer_init(int gpio, bool active_high)
{
    s_gpio        = gpio;
    s_active_high = active_high;

    gpio_config_t io = {
        .pin_bit_mask = (1ULL << gpio),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&io);
    buzzer_set(false);   /* comeca em silencio */
}

void buzzer_set(bool on)
{
    if (s_gpio < 0) return;
    gpio_set_level(s_gpio, on ? (s_active_high ? 1 : 0)
                              : (s_active_high ? 0 : 1));
}
