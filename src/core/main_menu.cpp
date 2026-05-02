#include "main_menu.h"
#include "display.h"
#include "utils.h"
#include <globals.h>

// ==========================================
// VOLTSHIELD X: EXTERNAL APP LINKAGE
// ==========================================
extern void voltshield_app(); 
// ==========================================

MainMenu::MainMenu() {
    _menuItems = {
        &wifiMenu,
        &bleMenu,
#if !defined(LITE_VERSION)
        &ethernetMenu,
#endif
        &rfMenu,
        &rfidMenu,
        &irMenu,
#if defined(FM_SI4713) && !defined(LITE_VERSION)
        &fmMenu,
#endif
        &fileMenu,
        &gpsMenu,
        &nrf24Menu,
#if !defined(LITE_VERSION)
#if !defined(DISABLE_INTERPRETER)
        &scriptsMenu,
        &appsMenu,
#endif
        &loraMenu,
#endif
        &othersMenu,
        &clockMenu,
#if !defined(LITE_VERSION)
        &connectMenu,
#endif
        &configMenu,
    };

    _totalItems = _menuItems.size();
}

MainMenu::~MainMenu() {}

void MainMenu::begin(void) {
    returnToMenu = false;
    options = {};

    std::vector<String> l = bruceConfig.disabledMenus;
    for (int i = 0; i < _totalItems; i++) {
        String itemName = _menuItems[i]->getName();
        if (find(l.begin(), l.end(), itemName) == l.end()) { 
            options.push_back(
                {
                 _menuItems[i]->getName(),
                 [this, i]() { _menuItems[i]->optionsMenu(); },
                 false,                                  
                 [](void *menuItem, bool shouldRender) { 
                     if (!shouldRender) return false;
                     drawMainBorder(false);

                     MenuItemInterface *obj = static_cast<MenuItemInterface *>(menuItem);
                     float scale = float((float)tftWidth / (float)240);
                     if (bruceConfigPins.rotation & 0b01) scale = float((float)tftHeight / (float)135);
                     obj->draw(scale);
#if defined(HAS_TOUCH)
                     TouchFooter();
#endif
                     return true;
                 },
                 _menuItems[i]
                }
            );
        }
    }

    // ==========================================
    // VOLTSHIELD X: SOVEREIGN MENU INJECTION
    // ==========================================
    options.push_back({
        "VoltShield X",
        []() { voltshield_app(); }, // استدعاء التطبيق الأمني
        false,
        [](void *menuItem, bool shouldRender) {
            if (!shouldRender) return false;
            drawMainBorder(false);
            tft.setTextColor(TFT_GREEN);
            tft.setTextSize(2);
            // رسم أيقونة نصية بسيطة للمنظومة
            tft.drawCentreString("[ VX ]", tftWidth / 2, tftHeight / 2 - 30, 1);
            tft.setTextSize(1);
            tft.drawCentreString("Sovereign Security", tftWidth / 2, tftHeight / 2 + 10, 1);
#if defined(HAS_TOUCH)
            TouchFooter();
#endif
            return true;
        }
    });
    // ==========================================

    _currentIndex = loopOptions(options, MENU_TYPE_MAIN, "Main Menu", _currentIndex);
};

void MainMenu::hideAppsMenu() {
    auto items = this->getItems();
RESTART: 
    options.clear();
    for (auto item : items) {
        String label = item->getName();
        std::vector<String> l = bruceConfig.disabledMenus;
        bool enabled = find(l.begin(), l.end(), label) == l.end();
        options.push_back({label, [this, label]() { bruceConfig.addDisabledMenu(label); }, enabled});
    }
    options.push_back({"Show All", [=]() { bruceConfig.disabledMenus.clear(); }, true});
    addOptionToMainMenu();
    loopOptions(options);
    bruceConfig.saveFile();
    if (!returnToMenu) goto RESTART;
}
