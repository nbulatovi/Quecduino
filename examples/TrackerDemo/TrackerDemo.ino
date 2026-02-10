// TrackerDemo.ino - Quecduino Arduino Library
// Copyright (c) 2025 Quectel Wireless Solutions, Co., Ltd.

#include <LPWA.h>

#define MQTT_HOST "broker.emqx.io"
#define MQTT_PORT 1883
#define MQTT_TOPIC "lpwa/tracker/location"

void setup() {
    Serial.begin(115200);
    Serial2.begin(115200, SERIAL_8N1, 26, 25);
    delay(3000);

    LPWA.config.module = BG950A;
    LPWA.begin(&Serial2, /*dtr_pin*/27, /*wakeup_pin*/33);
    LPWA.wakeup();
    LPWA.factory_reset();
    LPWA.configure();
    LPWA.disable_psm();
    LPWA.disable_drx();
}

void loop() {
    // Print modem status every second -----------------------------------------
    delay(1000);
    LPWA.update();

    // Assistance data download ------------------------------------------------
    static uint8_t agnss_dl_cnt = 0;
    if (LPWA.registered && !agnss_dl_cnt++) {
        LPWA.agnss_download();
    }
    if (!LPWA.agnss_valid && !LPWA.agnss_check()) return;

    // GPS signal acquisition --------------------------------------------------
    if (!LPWA.gps_on) {
        LPWA.start_gnss();
    }
    if (LPWA.gnss_fix.count < 5 && millis() - LPWA.gnss_start < 25000) return;
    LPWA.stop_gnss();

    // MQTT publish ------------------------------------------------------------
    if (LPWA.gnss_fix.count > 0) {
        LPWA.mqtt_open(MQTT_HOST, MQTT_PORT); 
        LPWA.mqtt_connect("Quecduino");
        char msg[96];
        snprintf(msg, sizeof(msg), "{\"lat\":%.6f,\"lon\":%.6f,\"ts\":%ld}",
                 LPWA.gnss_fix.lat, LPWA.gnss_fix.lon, LPWA.gnss_fix.timestamp);
        LPWA.mqtt_publish(MQTT_TOPIC, msg); 
        LPWA.mqtt_disconnect(); 
        LPWA.mqtt_close();
    }
    
    // Sleep -------------------------------------------------------------------
    LPWA.enable_drx();
    LPWA.enable_psm();
    LPWA.sleep();
    WAIT_FOR(!LPWA.ready, COMMAND_TIMEOUT); 
    // MCU can go to sleep here
    WAIT_FOR(LPWA.ready, 24L*3600*1000); 
    
    // Wake up -----------------------------------------------------------------
    LPWA.wakeup();
    LPWA.disable_psm();
    LPWA.disable_drx();
}
