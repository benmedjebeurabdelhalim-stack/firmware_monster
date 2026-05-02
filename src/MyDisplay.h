#ifndef MY_DISPLAY_H
#define MY_DISPLAY_H

#define LGFX_USE_V1
#include <LovyanGFX.hpp>

class MyCustomDisplay : public lgfx::LGFX_Device {
    lgfx::Panel_ST7796     _panel_instance; 
    lgfx::Bus_SPI          _bus_instance;
    lgfx::Touch_XPT2046    _touch_instance; 

public:
    MyCustomDisplay() {
        { // إعداد ناقل SPI المشترك (11, 12, 13)
            auto cfg = _bus_instance.config();
            cfg.spi_host = SPI2_HOST;
            cfg.spi_mode = 0;
            cfg.freq_write = 40000000;
            cfg.pin_sclk = 12; 
            cfg.pin_mosi = 11; 
            cfg.pin_miso = 13; 
            cfg.pin_dc   = 14; 
            _bus_instance.config(cfg);
            _panel_instance.setBus(&_bus_instance);
        }
        { // إعداد الشاشة (CS=10, RST=5)
            auto cfg = _panel_instance.config();
            cfg.pin_cs   = 10; 
            cfg.pin_rst  = 5;  
            cfg.panel_width  = 320;
            cfg.panel_height = 480;
            cfg.bus_shared   = true; 
            _panel_instance.config(cfg);
        }
        { // إعداد اللمس (CS=4, IRQ=2)
            auto cfg = _touch_instance.config();
            cfg.pin_int    = 2;   
            cfg.pin_cs     = 4;  
            cfg.spi_host   = SPI2_HOST;
            cfg.bus_shared = true;
            _touch_instance.config(cfg);
            _panel_instance.setTouch(&_touch_instance);
        }
        setPanel(&_panel_instance);
    }

    void begin() {
        init(); // تشغيل LovyanGFX فعلياً
    }

    // دالة جلب اللمس الحقيقية
    bool getTouch(int32_t* x, int32_t* y) {
        return LGFX_Device::getTouch(x, y);
    }
};

// تعريف الكائن ليكون متاحاً في main.cpp
extern MyCustomDisplay my_sovereign_display;

#endif 
