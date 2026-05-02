#ifndef MY_DISPLAY_H
#define MY_DISPLAY_H

#define LGFX_USE_V1
#include <LovyanGFX.hpp>

class MyCustomDisplay : public lgfx::LGFX_Device {
    // استخدمنا ILI9488 كخيار بديل لـ ST7796 لكسر اللون الأبيض
    lgfx::Panel_ILI9488    _panel_instance; 
    lgfx::Bus_SPI          _bus_instance;
    lgfx::Touch_XPT2046    _touch_instance; 

public:
    MyCustomDisplay() {
        { // إعداد ناقل SPI
            auto cfg = _bus_instance.config();
            cfg.spi_host = SPI2_HOST;
            cfg.spi_mode = 0;
            cfg.freq_write = 20000000; // سرعة 20MHz لضمان استقرار الإشارة (SCK)
            
            // ملاحظة: المتغير اسمه pin_sclk برمجياً وهو موصول بـ SCK في لوحتك
            cfg.pin_sclk = 12; // SCK -> GPIO 12
            cfg.pin_mosi = 11; // MOSI -> GPIO 11
            cfg.pin_miso = 13; // MISO -> GPIO 13
            cfg.pin_dc   = 14; // DC -> GPIO 14
            
            _bus_instance.config(cfg);
            _panel_instance.setBus(&_bus_instance);
        }
        { // إعداد الشاشة (ILI9488)
            auto cfg = _panel_instance.config();
            cfg.pin_cs   = 10; // CS -> GPIO 10
            cfg.pin_rst  = 5;  // RST -> GPIO 5
            cfg.panel_width  = 320;
            cfg.panel_height = 480;
            cfg.bus_shared   = true; 
            _panel_instance.config(cfg);
        }
        { // إعداد اللمس (XPT2046)
            auto cfg = _touch_instance.config();
            cfg.pin_int    = 2;   // IRQ -> GPIO 2
            cfg.pin_cs     = 4;   // Touch CS -> GPIO 4
            cfg.spi_host   = SPI2_HOST;
            cfg.bus_shared = true;
            _touch_instance.config(cfg);
            _panel_instance.setTouch(&_touch_instance);
        }
        setPanel(&_panel_instance);
    }

    void begin() {
        init(); // هنا يبدأ السحر وتظهر الألوان
    }

    bool getTouch(int32_t* x, int32_t* y) {
        return LGFX_Device::getTouch(x, y);
    }
};

extern MyCustomDisplay my_sovereign_display;

#endif 
