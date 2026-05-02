#if !defined(LITE_VERSION)
#include "LoRaRF.h"
#include "WString.h"
#include "core/config.h"
#include "core/configPins.h"
#include <Arduino.h>
#include <FS.h>
#include <LittleFS.h>
#include <RadioLib.h>
#include <core/display.h>
#include <core/mykeyboard.h>
#include <core/utils.h>
#include <globals.h>
#include <vector>

extern BruceConfigPins bruceConfigPins;

// متغيرات التحكم بالواجهة
bool update = false;
String msg;
String rcvmsg;
String displayName;
bool intlora = true;
std::vector<String> messages;
int scrollOffset = 0;
const int maxMessages = 19;

// إعدادات البث (Sovereign LoRa Settings)
#define spreadingFactor 9
#define SignalBandwidth 31.25E3
#define codingRateDenominator 8
#define preambleLength 8

int contentWidth = tftWidth - 20;
int yStart = 35;
int yPos = yStart;
int ySpacing = 10;
int rightColumnX = tftWidth / 2 + 10;

SPIClass *loraSpi = nullptr;
Module *loraModule = nullptr;
SX1276 *lora1276 = nullptr;
SX1262 *lora1262 = nullptr;

volatile bool loraPacketReceived = false;
volatile bool loraInterruptEnabled = true;
enum class LoRaRadioVariant { SX1276, SX1262 };
LoRaRadioVariant loraRadioVariant = LoRaRadioVariant::SX1276; // Ra-02 هو SX1278 ويتوافق مع تعريف 1276

// ========================================================
// VOLTSHIELD X: HARDCODED PIN DEFINITIONS (Sovereign Injection)
// ========================================================
int getLoraIrqPin()   { return 6; }  // الدبوس GPIO 6 حسب مخططك
int getLoraCsPin()    { return 7; }  // الدبوس GPIO 7 حسب مخططك
int getLoraResetPin() { return -1; } // Ra-02 غالباً مربوط بالـ 3.3V أو لا يحتاج رست خارجي
int getLoraBusyPin()  { return GPIO_NUM_NC; }

void clearLoraRadio() {
    if (lora1276) { delete lora1276; lora1276 = nullptr; }
    if (lora1262) { delete lora1262; lora1262 = nullptr; }
    if (loraModule) { delete loraModule; loraModule = nullptr; }
}

void onLoraPacket() {
    if (!loraInterruptEnabled) return;
    loraPacketReceived = true;
}

SPIClass *selectLoraSPIBus() {
    // إجبار اللورا على استخدام ناقل SPI المشترك مع الشاشة لضمان عدم حدوث Crash
    // الدبابيس هي (11, 12, 13) التي قمنا بتهيئتها في MyDisplay.h
    Serial.println("VoltShield X: Using Shared SPI for LoRa Operations");
    return &tft.getSPIinstance(); 
}

bool startLoraRadio(float bandMHz) {
    intlora = false;
    loraPacketReceived = false;
    loraInterruptEnabled = true;
    
    // سحب الدبابيس المادية المحددة
    const int irqPin = getLoraIrqPin();
    const int csPin = getLoraCsPin();

    loraSpi = selectLoraSPIBus();
    clearLoraRadio();
    
    // تهيئة موديول SX1278 (Ra-02)
    loraModule = new Module(csPin, irqPin, getLoraResetPin(), GPIO_NUM_NC, *loraSpi);

    int state = RADIOLIB_ERR_NONE;
    if (loraRadioVariant == LoRaRadioVariant::SX1276) {
        lora1276 = new SX1276(loraModule);
        state = lora1276->begin(bandMHz);
        if (state == RADIOLIB_ERR_NONE) { lora1276->setDio0Action(onLoraPacket, CHANGE); }
        if (state == RADIOLIB_ERR_NONE) state = lora1276->setSpreadingFactor(spreadingFactor);
        if (state == RADIOLIB_ERR_NONE) state = lora1276->setBandwidth(SignalBandwidth / 1000.0);
        if (state == RADIOLIB_ERR_NONE) state = lora1276->setCodingRate(codingRateDenominator);
        if (state == RADIOLIB_ERR_NONE) state = lora1276->setPreambleLength(preambleLength);
        if (state == RADIOLIB_ERR_NONE) state = lora1276->startReceive();
    } else {
        // دعم SX1262 في حال قمت بترقية الهاردوار مستقبلاً
        lora1262 = new SX1262(loraModule);
        state = lora1262->begin(bandMHz);
        if (state == RADIOLIB_ERR_NONE) { lora1262->setDio1Action(onLoraPacket); }
        if (state == RADIOLIB_ERR_NONE) state = lora1262->setSpreadingFactor(spreadingFactor);
        if (state == RADIOLIB_ERR_NONE) state = lora1262->setBandwidth(SignalBandwidth / 1000.0);
        if (state == RADIOLIB_ERR_NONE) state = lora1262->setCodingRate(codingRateDenominator);
        if (state == RADIOLIB_ERR_NONE) state = lora1262->setPreambleLength(preambleLength);
        if (state == RADIOLIB_ERR_NONE) state = lora1262->startReceive();
    }

    if (state != RADIOLIB_ERR_NONE) {
        Serial.printf("LoRa Initialization Failed! Error: %d\n", state);
        displayError("LoRa Init Failed", true);
        clearLoraRadio();
        return false;
    }
    intlora = true;
    Serial.println("LoRa Ra-02 Ready on VoltShield X");
    return true;
}

// ... بقية الوظائف البرمجية (sendLoraMessage, reciveMessage, render...)
// تم الإبقاء عليها كما هي لأنها تتعامل مع المنطق البرمجي للرسائل والقوائم.

bool sendLoraMessage(String &payload) {
    if (!intlora) return false;
    loraInterruptEnabled = false;
    int state = RADIOLIB_ERR_NONE;
    if (loraRadioVariant == LoRaRadioVariant::SX1276 && lora1276) {
        state = lora1276->transmit(payload);
        lora1276->startReceive();
    } else if (loraRadioVariant == LoRaRadioVariant::SX1262 && lora1262) {
        state = lora1262->transmit(payload);
        lora1262->startReceive();
    } else {
        loraInterruptEnabled = true;
        return false;
    }
    loraInterruptEnabled = true;
    if (state != RADIOLIB_ERR_NONE) {
        Serial.printf("LoRa transmit failed: %d\n", state);
        displayError("LoRa send failed");
        return false;
    }
    return true;
}

void reciveMessage() {
    if (!loraPacketReceived || !intlora) return;
    loraInterruptEnabled = false;
    loraPacketReceived = false;
    String incoming;
    int state = (loraRadioVariant == LoRaRadioVariant::SX1262 && lora1262)
                    ? lora1262->readData(incoming)
                    : (lora1276 ? lora1276->readData(incoming) : -1);
    if (state == RADIOLIB_ERR_NONE) {
        rcvmsg = incoming;
        Serial.println("Received: " + rcvmsg);
        File file = LittleFS.open("/chats.txt", "a");
        file.println(rcvmsg);
        file.close();
        messages.push_back(rcvmsg);
        if (messages.size() > maxMessages) { scrollOffset = messages.size() - maxMessages; }
        update = true;
    } else {
        Serial.printf("LoRa read failed: %d\n", state);
    }
    if (loraRadioVariant == LoRaRadioVariant::SX1262 && lora1262) {
        lora1262->startReceive();
    } else if (lora1276) {
        lora1276->startReceive();
    }
    loraInterruptEnabled = true;
}

void render() {
    if (!update) return;
    tft.setTextSize(1);
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(0x6DFC);
    if (!intlora) { tft.drawString("Lora Init Failed", 10, 13); }
    tft.drawString("USRN: " + String(displayName), 10, 25);

    int yPos = yStart;
    int endLine = scrollOffset + maxMessages;
    if (endLine > messages.size()) endLine = messages.size();
    for (int i = scrollOffset; i < endLine; i++) {
        tft.setTextColor(bruceConfig.priColor);
        tft.drawString(messages[i], 10, yPos);
        yPos += ySpacing;
    }
    update = false;
}

void loadMessages() {
    messages.clear();
    File file = LittleFS.open("/chats.txt", "r");
    while (file.available()) {
        String line = file.readStringUntil('\n');
        messages.push_back(line);
    }
    file.close();
    if (messages.size() > maxMessages) {
        scrollOffset = messages.size() - maxMessages;
    } else {
        scrollOffset = 0;
    }
}

void sendmsg() {
    Serial.println("Send Action Initiated");
    tft.fillScreen(TFT_BLACK);
    if (!intlora) {
        tft.setTextColor(TFT_RED);
        tft.setTextSize(2);
        tft.drawCentreString("LoRa not initialized!", tftWidth / 2, tftHeight / 2, 2);
        delay(1500);
        update = true;
        return;
    }
    msg = keyboard(msg, 256, "Message:");
    if (msg == "") { update = true; return; }
    msg = String(displayName) + ": " + msg;
    if (!sendLoraMessage(msg)) {
        update = true;
        return;
    }
    tft.fillScreen(TFT_BLACK);
    update = true;
    File file = LittleFS.open("/chats.txt", "a");
    file.println(msg);
    file.close();

    messages.push_back(msg);
    if (messages.size() > maxMessages) { scrollOffset = messages.size() - maxMessages; }
    msg = "";
}

void upress() {
    if (scrollOffset > 0) {
        scrollOffset--;
        update = true;
    }
}

void downpress() {
    if (scrollOffset < messages.size() - maxMessages) {
        scrollOffset++;
        update = true;
    }
}

void selectRadioVariant(JsonDocument &doc) {
    String stored = doc["LoRa_Radio"] | "SX1276";
    if (stored.equalsIgnoreCase("SX1262")) { loraRadioVariant = LoRaRadioVariant::SX1262; }
    std::vector<Option> radioOptions = {
        {"SX1276", []() {}},
        {"SX1262", []() {}}
    };
    int selected = loopOptions(
        radioOptions, MENU_TYPE_SUBMENU, "LoRa Radio", (loraRadioVariant == LoRaRadioVariant::SX1262) ? 1 : 0
    );
    if (selected >= 0) {
        loraRadioVariant = (selected == 1) ? LoRaRadioVariant::SX1262 : LoRaRadioVariant::SX1276;
        doc["LoRa_Radio"] = (loraRadioVariant == LoRaRadioVariant::SX1262) ? "SX1262" : "SX1276";
        File cfg = LittleFS.open("/lora_settings.json", "w");
        serializeJson(doc, cfg);
        cfg.close();
    }
}

void mainloop() {
    bool breakloop = false;
    while (true) {
        render();
        reciveMessage();
        if (breakloop) { break; }

        if (check(NextPress)) downpress();
        if (check(EscPress)) break;
        if (check(PrevPress)) upress();
        if (check(SelPress)) sendmsg();

        delay(20);
    }
}

void lorachat() {
    if (!LittleFS.exists("/chats.txt")) {
        File file = LittleFS.open("/chats.txt", "w");
        file.close();
    }
    if (!LittleFS.exists("/lora_settings.json")) {
        JsonDocument doc;
        File file = LittleFS.open("/lora_settings.json", "w");
        doc["LoRa_Frequency"] = "434500000.00";
        doc["LoRa_Name"] = "VoltShieldX";
        doc["LoRa_Radio"] = "SX1276";
        serializeJson(doc, file);
        file.close();
    }
    File file = LittleFS.open("/lora_settings.json", "r");
    JsonDocument doc;
    deserializeJson(doc, file);
    displayName = doc["LoRa_Name"].as<String>();
    double BAND = doc["LoRa_Frequency"].as<String>().toDouble();
    file.close();
    
    selectRadioVariant(doc);
    float bandMHz = (BAND > 1000) ? BAND / 1000000.0f : (float)BAND;
    
    tft.fillScreen(TFT_BLACK);
    update = true;
    
    Serial.println("Sovereign Init: " + String(bandMHz) + "MHz");

    if (!startLoraRadio(bandMHz)) {
        update = true;
        return;
    }
    tft.setTextWrap(true, true);
    tft.setTextDatum(TL_DATUM);
    loadMessages();
    mainloop();
}

void changeusername() {
    tft.fillScreen(TFT_BLACK);
    String username = keyboard("", 64, "Set Nickname:");
    if (username == "") return;
    File file = LittleFS.open("/lora_settings.json", "r");
    JsonDocument doc;
    deserializeJson(doc, file);
    file.close();
    doc["LoRa_Name"] = username;
    file = LittleFS.open("/lora_settings.json", "w");
    serializeJson(doc, file);
    file.close();
}

void chfreq() {
    tft.fillScreen(TFT_BLACK);
    char buf[15];
    File file = LittleFS.open("/lora_settings.json", "r");
    JsonDocument doc;
    deserializeJson(doc, file);
    file.close();

    double dfreq = doc["LoRa_Frequency"].as<String>().toDouble();
    dfreq = dfreq / 1000000;
    snprintf(buf, sizeof(buf), "%.3f", dfreq);
    String freq = num_keyboard(buf, 12, "In MHz:");
    dfreq = freq.toDouble();
    if (dfreq == 0 || dfreq > 1000) {
        displayError("Invalid Frequency");
        return;
    }
    dfreq = dfreq * 1000000;
    snprintf(buf, sizeof(buf), "%.2f", dfreq);
    doc["LoRa_Frequency"] = buf;

    file = LittleFS.open("/lora_settings.json", "w");
    serializeJson(doc, file);
    file.close();
}
#endif
