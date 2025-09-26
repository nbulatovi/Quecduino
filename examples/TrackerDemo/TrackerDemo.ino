#include <LPWA.h>

void setup() {
    Serial.begin(115200);    
    Serial2.begin(115200, SERIAL_8N1, 26, 25);

    LPWA.begin(&Serial2, /*dtr_pin*/27, /*wakeup_pin*/33);
    LPWA.wakeup();
    LPWA.reset();
    LPWA.enable_drx();
    LPWA.enable_psm();
}

void loop() {
    // Download XTRA file
    static uint8_t xtra_dl_cnt = 0;
    if (LPWA.registered && !xtra_dl_cnt++)  {    
        LPWA.at_send("AT+QGPSCFG=\"xtra_download\",1");
    } else if (LPWA.urc_map["QGPSURC"].find("XTRA_DL")==1 && LPWA.urc_map["QGPSXTRADATA"].empty()) {
        LPWA.at_send("AT+QGPSXTRADATA?");
    }
    
    // Print URC list periodically
    LPWA.update();
    
    // Sleep
    delay(1000);
    if (LPWA.urc_map["QGPSURC"].find("XTRA_DL")==1) {
        LPWA.sleep();
        delay(10000);
        LPWA.wakeup();
        WAIT_FOR(LPWA.ready, NETWORK_TIMEOUT);
    }
}
