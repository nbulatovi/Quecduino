// TrackerDemo.ino - Quecduino Arduino Library
// Copyright (c) 2025 Quectel Wireless Solutions, Co., Ltd.

#include <LPWA.h>

#define MQTT_HOST "broker.emqx.io"
#define MQTT_PORT 1883
#define MQTT_TOPIC "lpwa/tracker/location"

void setup() {
    Serial.begin(115200);
    Serial2.begin(115200, SERIAL_8N1, 27, 25);
    delay(3000);

    LPWA.config.module = BG950A;
    LPWA.config.psm_t3412 = "00100001";  // 1h periodic TAU (network may override)
    LPWA.config.psm_t3324 = "00011001";  // 50s active timer (network may override)
    LPWA.begin(&Serial2, /*dtr_pin*/26, /*wakeup_pin*/33);
    LPWA.wakeup();
    LPWA.factory_reset();
    LPWA.configure();
    LPWA.enable_drx();
    LPWA.enable_psm();
    LPWA.prevent_psm();
}

void loop() {
    delay(1000);
    LPWA.update();

    // AGNSS download (one-shot) -----------------------------------------------
    static uint8_t agnss_dl_cnt = 0;
    if (LPWA.registered && !agnss_dl_cnt++) {
        LPWA.agnss_download();
    }
    if (!LPWA.agnss_valid) LPWA.agnss_check();

    // GNSS fix ----------------------------------------------------------------
    if (!LPWA.gps_on) {
        LPWA.start_gnss();
    }
    if (LPWA.gnss_fix.count < 5 && millis() - LPWA.gnss_start < 30000) return;
    LPWA.stop_gnss();

    // MQTT publish ------------------------------------------------------------
    if (LPWA.gnss_fix.count >= 1) {
        WAIT_FOR(LPWA.registered, REGISTRATION_TIMEOUT);
        LPWA.mqtt_open(MQTT_HOST, MQTT_PORT);
        LPWA.mqtt_connect("Quecduino");
        char msg[96];
        snprintf(msg, sizeof(msg), "{\"lat\":%.6f,\"lon\":%.6f,\"ts\":%ld}",
                 LPWA.gnss_fix.lat, LPWA.gnss_fix.lon, LPWA.gnss_fix.timestamp);
        LPWA.mqtt_publish(MQTT_TOPIC, msg);
        LPWA.mqtt_disconnect();
        LPWA.mqtt_close();
    }

    // Allow PSM entry and sleep -----------------------------------------------
    LPWA.allow_psm();
    LPWA.sleep();
    if (WAIT_FOR(!LPWA.ready, 60000L)) {
        //
        // Waiting for modem to wake up - MCU can go to sleep here
        //
        WAIT_FOR(LPWA.ready, 3600L*1000);
    } else {
        Serial.println("Modem did not enter sleep mode - check R0413 on TE-B board");
    }

    // Wake up -----------------------------------------------------------------
    LPWA.wakeup();
    LPWA.prevent_psm();  // Block PSM during next data cycle
}
