#ifndef MY_DISPLAY_H
#define MY_DISPLAY_H

#define LGFX_USE_V1
#include <LovyanGFX.hpp>

class MyCustomDisplay : public lgfx::LGFX_Device {
    // الشاشة ST7796 كما أثبتت الصور
    lgfx::Panel_ST7796     _panel_instance; 
    lgfx::Bus_SPI          _bus_instance;
    // أعدنا تفعيل اللمس لأن الناقل أصبح محمياً
    lgfx::Touch_XPT2046    _touch_instance; 

public:
    MyCustomDisplay() {
        { // 1. إعداد ناقل SPI
            auto cfg = _bus_instance.config();
            cfg.spi_host = SPI2_HOST; // FSPI
            cfg.spi_mode = 0;
            
            // سرعة ممتازة ومستقرة (40MHz) للرسم السريع
            cfg.freq_write = 40000000; 
            cfg.freq_read  = 16000000; 
            
            // الدبابيس كما في VoltShield X
            cfg.pin_sclk = 12; 
            cfg.pin_mosi = 11; 
            cfg.pin_miso = 13; 
            cfg.pin_dc   = 14; 
            
            _bus_instance.config(cfg);
            _panel_instance.setBus(&_bus_instance);
        }
        { // 2. إعداد الشاشة (ST7796)
            auto cfg = _panel_instance.config();
            cfg.pin_cs   = 10; 
            cfg.pin_rst  = 5;  
            cfg.panel_width  = 320;
            cfg.panel_height = 480;
            
            // --- إعدادات الحماية والتوافق ---
            cfg.bus_shared   = true; // يمنع احتكار الناقل ويدعم وجود أجهزة أخرى (SD, LoRa)
            cfg.invert       = true; // تصحيح ألوان الشاشات الحمراء (برتقالي بدل أزرق)
            cfg.dummy_read_pixel = 8;
            cfg.offset_rotation = 0;
            
            _panel_instance.config(cfg);
        }
        { // 3. إعداد اللمس (XPT2046)
            auto cfg = _touch_instance.config();
            cfg.x_min      = 0;
            cfg.x_max      = 319;
            cfg.y_min      = 0;
            cfg.y_max      = 479;
            cfg.pin_int    = 2;  // TOUCH_IRQ
            cfg.pin_cs     = 4;  // TOUCH_CS
            
            cfg.bus_shared = true; // مشاركة الناقل بدون تصادم
            cfg.offset_rotation = 0;
            cfg.spi_host   = SPI2_HOST;
            cfg.freq       = 1000000; // سرعة اللمس بطيئة (1MHz) لضمان الدقة
            
            _touch_instance.config(cfg);
            _panel_instance.setTouch(&_touch_instance);
        }
        
        setPanel(&_panel_instance);
    }

    void begin() {
        // نبضة ريست يدوية قبل البدء (إسعافات أولية ممتازة لضمان الإقلاع)
        pinMode(5, OUTPUT);
        digitalWrite(5, LOW);
        delay(100);
        digitalWrite(5, HIGH);
        delay(100);
        
        init(); 
    }

    bool getTouch(int32_t* x, int32_t* y) {
        // أعدنا تفعيل قراءة اللمس الحقيقية
        return LGFX_Device::getTouch(x, y); 
    }
};

extern MyCustomDisplay my_sovereign_display;

#endif
