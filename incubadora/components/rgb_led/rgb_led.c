#include "rgb_led.h"
#include "driver/ledc.h"

#define LEDC_MODE   LEDC_LOW_SPEED_MODE
#define LEDC_TIMER  LEDC_TIMER_0
#define LEDC_RES    LEDC_TIMER_8_BIT   /* duty 0..255 */
#define LEDC_FREQ   5000               /* 5 kHz */

static const ledc_channel_t CH[3] = { LEDC_CHANNEL_0, LEDC_CHANNEL_1, LEDC_CHANNEL_2 };
static bool s_common_anode;

void rgb_led_init(int gpio_r, int gpio_g, int gpio_b, bool common_anode)
{
    s_common_anode = common_anode;

    ledc_timer_config_t tcfg = {
        .speed_mode      = LEDC_MODE,
        .duty_resolution = LEDC_RES,
        .timer_num       = LEDC_TIMER,
        .freq_hz         = LEDC_FREQ,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&tcfg);

    const int gpios[3] = { gpio_r, gpio_g, gpio_b };
    for (int i = 0; i < 3; i++) {
        ledc_channel_config_t ccfg = {
            .gpio_num   = gpios[i],
            .speed_mode = LEDC_MODE,
            .channel    = CH[i],
            .timer_sel  = LEDC_TIMER,
            .duty       = 0,
            .hpoint     = 0,
        };
        ledc_channel_config(&ccfg);
    }
    rgb_led_set(0, 0, 0);
}

static void set_channel(int i, uint8_t value)
{
    uint32_t duty = s_common_anode ? (255u - value) : value;   /* anodo comum: invertido */
    ledc_set_duty(LEDC_MODE, CH[i], duty);
    ledc_update_duty(LEDC_MODE, CH[i]);
}

void rgb_led_set(uint8_t r, uint8_t g, uint8_t b)
{
    set_channel(0, r);
    set_channel(1, g);
    set_channel(2, b);
}
