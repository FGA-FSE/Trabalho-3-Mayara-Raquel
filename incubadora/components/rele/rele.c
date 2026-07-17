#include "rele.h"
#include "driver/gpio.h"

static int  s_gpio = -1;
static bool s_active_low = true;

void rele_init(int gpio, bool active_low)
{
    s_gpio       = gpio;
    s_active_low = active_low;

    gpio_config_t io = {
        .pin_bit_mask = (1ULL << gpio),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&io);
    rele_set(false);   /* comeca desligado (senao fecha durante o boot) */
}

void rele_set(bool on)
{
    if (s_gpio < 0) return;
    gpio_set_level(s_gpio, on ? (s_active_low ? 0 : 1)
                              : (s_active_low ? 1 : 0));
}
