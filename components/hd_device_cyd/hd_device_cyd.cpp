#include "hd_device_cyd.h"
#include "Room.h"
#include "LovyanGFX.hpp"

namespace esphome {
namespace hd_device {

static const char *const TAG = "HD_DEVICE";
static lv_disp_draw_buf_t draw_buf;
static lv_color_t *buf = (lv_color_t *)heap_caps_malloc(TFT_HEIGHT * 20 * sizeof(lv_color_t), MALLOC_CAP_DMA);

int x = 0;
int y = 0;

SPIClass mySpi = SPIClass(VSPI);
XPT2046_Touchscreen ts(XPT2046_CS, XPT2046_IRQ);

LGFX lcd;

lv_disp_t *indev_disp;
lv_group_t *group;

//test
const uint16_t test_image[3] = {0xF800, 0x07E0, 0x001F}; 


void displayBackground() {
    lcd.startWrite();  
    // Convert the Room.jpg image to a compatible format (e.g., RGB565) and include it as a C array
    lcd.pushImage(0, 0, Room.width, Room.height, reinterpret_cast<const uint16_t*>(Room.data)); 
    //lcd.pushImage(0, 0, 3, 1, test_image);   
    lcd.endWrite();
}

void IRAM_ATTR flush_pixels(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    uint32_t len = w * h;

    lcd.startWrite();                            /* Start new TFT transaction */
    lcd.setAddrWindow(area->x1, area->y1, w, h); /* set the working window */
    lcd.writePixels((uint16_t *)&color_p->full, len, true);
    lcd.endWrite();                              /* terminate TFT transaction */

    lv_disp_flush_ready(disp);
}

void IRAM_ATTR touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data)
{

    if (ts.touched()) {
        TS_Point p = ts.getPoint();
        x = map(p.x, 220, 3850, 320, 1);
        y = map(p.y, 310, 3773, 1, 240);
        data->point.x = x;
        data->point.y = y;
        data->state = LV_INDEV_STATE_PR;
        ESP_LOGCONFIG(TAG, "X: %d ", x);
        ESP_LOGCONFIG(TAG, "Y: %d ", y);
        ESP_LOGD("TOUCH", "Raw X: %d, Raw Y: %d", p.x, p.y);
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}

void HaDeckDevice::setup() {
    lv_init();
    lv_theme_default_init(NULL, lv_color_hex(0xFFEB3B), lv_color_hex(0xFF7043), 1, LV_FONT_DEFAULT);

    mySpi.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
    ts.begin(mySpi);
    ts.setRotation(2);

    

    lcd.init();
    lcd.setRotation(2);

    lv_disp_draw_buf_init(&draw_buf, buf, NULL, TFT_HEIGHT * 20);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = TFT_WIDTH;
    disp_drv.ver_res = TFT_HEIGHT;
    disp_drv.rotated = 0;
    disp_drv.sw_rotate = 0;
    disp_drv.flush_cb = flush_pixels;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_t *disp = lv_disp_drv_register(&disp_drv);

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.long_press_time = 1000;
    indev_drv.long_press_repeat_time = 300;
    indev_drv.read_cb = touchpad_read;
    lv_indev_drv_register(&indev_drv);

    group = lv_group_create();
    lv_group_set_default(group);

    lcd.setBrightness(brightness_);

    lv_obj_t * bg_color = lv_obj_create(lv_scr_act());
    lv_obj_set_size(bg_color, 320, 240);
    lv_obj_set_style_border_width(bg_color, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(bg_color, lv_color_hex(0x171717), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_parent(bg_color, lv_scr_act());

}

void HaDeckDevice::loop() {
    lv_timer_handler();

    unsigned long ms = millis();
    if (ms - time_ > 60000) {
        time_ = ms;
        ESP_LOGCONFIG(TAG, "Free memory: %d bytes", esp_get_free_heap_size());
    }

    // lcd.fillScreen(0xF800); // Remplit l'écran en rouge (RGB565)
    // delay(2000);
    // lcd.fillScreen(0x07E0); // Vert
    // delay(2000);
    // lcd.fillScreen(0x001F); // Bleu

    displayBackground();
}

float HaDeckDevice::get_setup_priority() const { return setup_priority::DATA; }

uint8_t HaDeckDevice::get_brightness() {
    return brightness_;
}

void HaDeckDevice::set_brightness(uint8_t value) {
    brightness_ = value;
    lcd.setBrightness(brightness_);
}

}  // namespace hd_device
}  // namespace esphome
