#include "ssd1306.h"
#include <string.h>
#include <stddef.h>

#define OLED_TIMEOUT_MS 1000
#define OLED_W 128
#define OLED_H 64

static i2c_master_dev_handle_t s_oled;
static uint8_t s_fb[OLED_W * OLED_H / 8];   /* framebuffer: 1024 bytes */

/* Fonte 5x7 (colunas; bit0 = topo). Apenas os caracteres usados no projeto. */
typedef struct { char c; uint8_t col[5]; } glyph_t;
static const glyph_t FONT[] = {
    {' ', {0x00,0x00,0x00,0x00,0x00}},
    {'0', {0x3E,0x51,0x49,0x45,0x3E}},
    {'1', {0x00,0x42,0x7F,0x40,0x00}},
    {'2', {0x42,0x61,0x51,0x49,0x46}},
    {'3', {0x21,0x41,0x45,0x4B,0x31}},
    {'4', {0x18,0x14,0x12,0x7F,0x10}},
    {'5', {0x27,0x45,0x45,0x45,0x39}},
    {'6', {0x3C,0x4A,0x49,0x49,0x30}},
    {'7', {0x01,0x71,0x09,0x05,0x03}},
    {'8', {0x36,0x49,0x49,0x49,0x36}},
    {'9', {0x06,0x49,0x49,0x29,0x1E}},
    {'.', {0x00,0x60,0x60,0x00,0x00}},
    {'-', {0x08,0x08,0x08,0x08,0x08}},
    {':', {0x00,0x36,0x36,0x00,0x00}},
    {'%', {0x23,0x13,0x08,0x64,0x62}},
    {'~', {0x07,0x05,0x07,0x00,0x00}},   /* simbolo de grau */
    {'A', {0x7E,0x11,0x11,0x11,0x7E}},
    {'B', {0x7F,0x49,0x49,0x49,0x36}},
    {'C', {0x3E,0x41,0x41,0x41,0x22}},
    {'D', {0x7F,0x41,0x41,0x22,0x1C}},
    {'E', {0x7F,0x49,0x49,0x49,0x41}},
    {'F', {0x7F,0x09,0x09,0x09,0x01}},
    {'H', {0x7F,0x08,0x08,0x08,0x7F}},
    {'I', {0x00,0x41,0x7F,0x41,0x00}},
    {'L', {0x7F,0x40,0x40,0x40,0x40}},
    {'M', {0x7F,0x02,0x0C,0x02,0x7F}},
    {'N', {0x7F,0x04,0x08,0x10,0x7F}},
    {'O', {0x3E,0x41,0x41,0x41,0x3E}},
    {'P', {0x7F,0x09,0x09,0x09,0x06}},
    {'Q', {0x3E,0x41,0x51,0x21,0x5E}},
    {'R', {0x7F,0x09,0x19,0x29,0x46}},
    {'S', {0x46,0x49,0x49,0x49,0x31}},
    {'T', {0x01,0x01,0x7F,0x01,0x01}},
    {'U', {0x3F,0x40,0x40,0x40,0x3F}},
};

static const uint8_t *glyph(char c)
{
    for (size_t i = 0; i < sizeof(FONT) / sizeof(FONT[0]); i++) {
        if (FONT[i].c == c) return FONT[i].col;
    }
    return FONT[0].col;   /* nao achou -> espaco */
}

static esp_err_t oled_cmd(uint8_t c)
{
    uint8_t b[2] = { 0x00, c };
    return i2c_master_transmit(s_oled, b, 2, OLED_TIMEOUT_MS);
}

static void set_pixel(int x, int y)
{
    if (x < 0 || x >= OLED_W || y < 0 || y >= OLED_H) return;
    s_fb[(y / 8) * OLED_W + x] |= (uint8_t)(1 << (y % 8));
}

void ssd1306_clear(void)
{
    memset(s_fb, 0, sizeof(s_fb));
}

static void draw_char(int x, int y, char c, int scale)
{
    const uint8_t *g = glyph(c);
    for (int col = 0; col < 5; col++) {
        for (int row = 0; row < 7; row++) {
            if (g[col] & (1 << row)) {
                for (int dx = 0; dx < scale; dx++)
                    for (int dy = 0; dy < scale; dy++)
                        set_pixel(x + col * scale + dx, y + row * scale + dy);
            }
        }
    }
}

void ssd1306_draw_string(int x, int y, const char *s, int scale)
{
    while (*s) {
        draw_char(x, y, *s, scale);
        x += 6 * scale;
        s++;
    }
}

void ssd1306_flush(void)
{
    for (int page = 0; page < 8; page++) {
        oled_cmd(0xB0 | page);
        oled_cmd(0x00);
        oled_cmd(0x10);
        uint8_t buf[1 + OLED_W];
        buf[0] = 0x40;
        memcpy(&buf[1], &s_fb[page * OLED_W], OLED_W);
        i2c_master_transmit(s_oled, buf, sizeof(buf), OLED_TIMEOUT_MS);
    }
}

esp_err_t ssd1306_init(i2c_master_bus_handle_t bus, uint8_t addr)
{
    i2c_device_config_t cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = addr,
        .scl_speed_hz    = 100000,
    };
    esp_err_t err = i2c_master_bus_add_device(bus, &cfg, &s_oled);
    if (err != ESP_OK) return err;

    static const uint8_t init_seq[] = {
        0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00, 0x40,
        0x8D, 0x14, 0x20, 0x02, 0xA1, 0xC8, 0xDA, 0x12,
        0x81, 0xCF, 0xD9, 0xF1, 0xDB, 0x40, 0xA4, 0xA6, 0xAF,
    };
    for (size_t i = 0; i < sizeof(init_seq); i++) {
        err = oled_cmd(init_seq[i]);
        if (err != ESP_OK) return err;
    }
    ssd1306_clear();
    ssd1306_flush();
    return ESP_OK;
}
