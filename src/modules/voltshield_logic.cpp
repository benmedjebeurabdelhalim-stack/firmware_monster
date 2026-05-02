#include <Arduino.h>
#include <globals.h>
#include "core/display.h"
#include "core/mykeyboard.h"

// دبابيسك المحددة
#define RADAR_PIN 1
#define MODEM_TX 17
#define MODEM_RX 18
#define MODEM_PWR 16

bool radarActive = false;

// وظيفة فحص الموديم
String checkModemSignal() {
    Serial1.println("AT+CSQ"); // أمر طلب قوة الإشارة
    delay(100);
    String res = "";
    while(Serial1.available()) {
        res += (char)Serial1.read();
    }
    if (res.indexOf("+CSQ:") != -1) return res.substring(res.indexOf(":")+2, res.indexOf(","));
    return "No Signal";
}

// واجهة تطبيق VoltShield X
void voltshield_app() {
    bool exitApp = false;
    tft.fillScreen(TFT_BLACK);
    
    while (!exitApp) {
        tft.setCursor(0, 0);
        tft.setTextColor(TFT_GREEN, TFT_BLACK);
        tft.setTextSize(2);
        tft.println("--- VoltShield X ---");
        
        // 1. عرض حالة الرادار
        int motion = digitalRead(RADAR_PIN);
        tft.setCursor(0, 40);
        tft.setTextSize(1);
        if (motion) {
            tft.setTextColor(TFT_RED, TFT_BLACK);
            tft.println("RADAR: MOTION DETECTED!");
        } else {
            tft.setTextColor(TFT_WHITE, TFT_BLACK);
            tft.println("RADAR: Scanning...");
        }

        // 2. عرض حالة الموديم
        tft.setCursor(0, 70);
        tft.setTextColor(0x6DFC); // لون أزرق فاتح
        tft.print("4G Signal: ");
        tft.println(checkModemSignal());

        tft.setCursor(0, 100);
        tft.setTextColor(TFT_YELLOW);
        tft.println("Press BACK to Exit");

        // الخروج والتحكم
        if (check(EscPress)) exitApp = true;
        
        delay(200); // تحديث كل 200 ملي ثانية
    }
}
