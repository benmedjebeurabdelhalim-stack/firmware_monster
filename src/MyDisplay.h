#ifndef MY_DISPLAY_H
#define MY_DISPLAY_H

#define LGFX_USE_V1
#include <LovyanGFX.hpp>

class MyCustomDisplay : public lgfx::LGFX_Device {
    // العودة لـ ST7796 لأنها الأكثر انتشاراً في مقاس 4.0 SPI
    lgfx::Panel_ST7796     _panel_instance; 
    lgfx::Bus_SPI          _bus_instance;
    // سنعطل اللمس برمجياً مؤقتاً لنرى الشاشة تقلع أولاً
    // lgfx::Touch_XPT2046    _touch_instance; 

public:
    MyCustomDisplay() {
        { 
            auto cfg = _bus_instance.config();
            cfg.spi_host = SPI2_HOST;
            cfg.spi_mode = 0;
            
            // --- تعديل ذهبي: سرعة بطيئة جداً للتغلب على مشاكل اللحام والأسلاك ---
            cfg.freq_write = 5000000;  // 5MHz فقط (للـ Debug)
            cfg.freq_read  = 5000000;
            
            cfg.pin_sclk = 12; 
            cfg.pin_mosi = 11; 
            cfg.pin_miso = 13; 
            cfg.pin_dc   = 14; 
            
            _bus_instance.config(cfg);
            _panel_instance.setBus(&_bus_instance);
        }
        { 
            auto cfg = _panel_instance.config();
            cfg.pin_cs   = 10; 
            cfg.pin_rst  = 5;  
            cfg.panel_width  = 320;
            cfg.panel_height = 480;
            cfg.bus_shared   = true; 
            
            // إعدادات إضافية لضمان التوافق
            cfg.dummy_read_pixel = 8;
            cfg.offset_rotation = 0;
            
            _panel_instance.config(cfg);
        }
        
        // تعطيل اللمس حالياً للتأكد من الشاشة
        setPanel(&_panel_instance);
    }

    void begin() {
        // نبضة ريست يدوية قبل البدء
        pinMode(5, OUTPUT);
        digitalWrite(5, LOW);
        delay(100);
        digitalWrite(5, HIGH);
        delay(100);
        
        init(); 
    }

    bool getTouch(int32_t* x, int32_t* y) {
        // دالة وهمية حتى نصلح الشاشة
        return false; 
    }
};

extern MyCustomDisplay my_sovereign_display;

#endif 
