#include <LPWA.h>

void setup() {
    Serial.begin(115200);
    
    Serial2.begin(115200, SERIAL_8N1, 26, 25);
    LPWA.begin(&Serial2, /*dtr_pin*/27, /*wakeup_pin*/14);
    LPWA.wakeup();
    // LPWA.factory_reset();
    LPWA.configure();
}

void loop() {
    // Download XTRA file
    static uint8_t xtra_dl_cnt = 0;
    if (LPWA.registered && !xtra_dl_cnt++)  {    
        LPWA.at_send("AT+QGPSCFG=\"xtra_download\",1");
    } else if (LPWA.urc_map["QGPSURC"].find("XTRA_DL")==1 && LPWA.urc_map["QGPSXTRADATA"].empty()) {
        LPWA.at_send("AT+QGPSXTRADATA?");
    }

    // Run periodic task
    LPWA.update();
    
    // Sleep
    LPWA.sleep();
    delay(1000);
    LPWA.wakeup();
}
