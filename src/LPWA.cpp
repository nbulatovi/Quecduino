// LPWA.cpp - Quecduino Arduino Library
// Copyright (c) 2025 Quectel Wireless Solutions, Co., Ltd.

#include "LPWA.h"
#include "gnss_errors.h"
#include <time.h>
#include <math.h>

LPWAClass LPWA;

void LPWAClass::recv_task() {
    while (at_port->available() && received.length() < AT_BUF_SIZE-1) {
        char ch = at_port->read();
        if (ch != 0x0D && ch != 0x0A) {
            received += ch;
        } else if (!received.empty()) {
            if (received.length() > 1) process_response();
            received.clear();
        }
    }
}

void LPWAClass::begin(HardwareSerial *at_port0, uint8_t dtr_pin0, uint8_t wakeup_pin) {
    at_port = at_port0;
    dtr_pin = dtr_pin0;
    pon_trig_pin = wakeup_pin;
    pinMode(pon_trig_pin, OUTPUT);
    pinMode(dtr_pin, OUTPUT);
    received.reserve(AT_BUF_SIZE);
    result.reserve(AT_BUF_SIZE);
    urc_list.reserve(256);
    ticker.attach(0.01, [this]() { this->recv_task(); });
    PRINT("[LPWA] Initialized\n");
}

void LPWAClass::end() {
    ticker.detach();
    PRINT("[LPWA] Deinitialized\n");
}

void LPWAClass::update() {
#ifdef SIM_GPS
    if (gps_on) parse_rmc("$GPRMC,120000.00,A,4023.4180,N,07948.6180,W,0.0,0.0,090226");
#endif
    PRINT("[LPWA] %s\n", LPWA.get_urc_list().c_str());
}

void LPWAClass::wakeup() {
    PRINT("[LPWA] Wakeup\n");
    digitalWrite(pon_trig_pin, HIGH);
    digitalWrite(dtr_pin, LOW);
}

void LPWAClass::sleep() {
    PRINT("[LPWA] Sleep\n");
    digitalWrite(dtr_pin, HIGH);
    digitalWrite(pon_trig_pin, LOW);
}

bool LPWAClass::is_alt1350() {
    return config.module == BG770S || config.module == BG950S;
}

void LPWAClass::factory_reset() {
    PRINT("[LPWA] Factory Reset\n");

    // Handle various startup scenarios
    at_port->println("AT");
    at_port->flush();
    WAIT_FOR(ready || result.find("OK")==0, NETWORK_TIMEOUT);
    
    // Send factory reset commands
    at_send("AT+QGPSDEL=0");
    at_send("AT&F1");
    WAIT_FOR(ready, NETWORK_TIMEOUT);
}

void LPWAClass::configure() {
    received.clear();
    result.clear();
    urc_map.clear();

    WAIT_FOR(ready, NETWORK_TIMEOUT);

    if (config.module == BG951A) {
        PRINT("[LPWA] Set GNSS mode\n");
        at_send("AT+QGPSCFG=\"gnss_mode\",1");      // CXD5605 I2C mode
    }
    PRINT("[LPWA] Configure Network\n");
    at_send("AT+QCFG=\"band\",0," + config.catm_bands + "," + config.catnb_bands + ",1");
    if (is_alt1350()) {
        at_send("AT+QCFG=\"iotopmode\",0");         // CatM mode
    } else {
        at_send("AT+QCFG=\"iotopmode\",0,1");       // CatM mode
    }

    PRINT("[LPWA] Enable AGNSS\n");
    if (is_alt1350()) {
        at_send("AT+QAGNSS=1");  
    } else {
        at_send("AT+QGPSXTRA=1");
    }

    PRINT("[LPWA] Reset modem\n");
    delay(1000);
    ready = false;
    at_send("AT+CFUN=1,1");
    WAIT_FOR(ready, NETWORK_TIMEOUT);

    if (!LPWA.ready) PRINT("[LPWA] ERROR: Timed out waiting for modem reset\n");

    PRINT("[LPWA] Subscribe to URC\n");
    at_send("AT+CMEE=2");
    at_send("AT+CEREG=1");
    at_send("AT+QCFG=\"psm/urc\",1");
    at_send("AT+CCLK?");

    PRINT("[LPWA] Configure GNSS\n");
    if (is_alt1350()) {
        at_send("AT+QGPSCFG=\"agnss_url\",\"" + config.proxy_url + "\"");
        at_send("AT%IGNSSCFG=\"SET\",\"BLANKING\",1,5000");
        at_send("AT%IGNSSEV=\"blanking\",1");
    } else if (config.module == BG951A) {
        at_send("AT+QGPSCFG=\"xtra_cfg\",\"" + config.proxy_url + "\"");
        at_send("AT+QGPSCFG=\"xtrafilesize\",14");
    } else {
        at_send("AT+QGPSCFG=\"priority\",1,1");
        at_send("AT+QGPSCFG=\"nmeasrc\",1");
        at_send("AT+QGPSCFG=\"gpsnmeatype\",31");
        at_send("AT+QGPSCFG=\"glonassnmeatype\",3");
        at_send("AT%IGNSSEV=\"ALL\",1");
        at_send("AT+QGPSCFG=\"xtra_cfg\",\"" + config.proxy_url + "\"");
        at_send("AT+QGPSCFG=\"xtrafilesize\",7");
    }

    // Query AGNSS status
    if (is_alt1350()) {
        at_send("AT+QAGNSS?");
    } else {
        at_send("AT+QGPSXTRA?");
        at_send("AT+QGPSXTRATIME?");
    }

    PRINT("[LPWA] Save settings\n");
    at_send("AT&W");

    PRINT("[LPWA] Query status\n");
    at_send("AT+CEDRXRDP");
    at_send("AT+CPSMS?");
    at_send("AT+QGMR?");
}

void LPWAClass::process_response() {
    // GNSS NMEA — extract clean sentence from wrapped or raw NMEA
    if (received.find("$G") != std::string::npos || received.find("$P") != std::string::npos) {
        const char *s = strstr(received.c_str(), "$");
        if (s) {
            const char *e = strchr(s, '*');
            PRINT("[NMEA] %.*s\n", (int)(e && e[1] && e[2] ? e + 3 - s : strlen(s)), s);
            if (strstr(s, "RMC,")) parse_rmc(s);
        }
        return;
    }

    PRINT("[AT] %s\n", received.c_str());

    // Process AT responses and URC
    if (received.find("OK") == 0) {
        result = received;
    } else if (received.find("ERROR") == 0) {
        result = received;
    } else if (received.find("APP RDY") == 0) {
        ready = true;
    } else if (received.find("POWER DOWN") != std::string::npos) {
        gps_on = ready = false;
    } else if (received.find("+") == 0) {
        // Parse URC
        urc_map[received.substr(1, received.find(':')-1)] = received.substr(received.find(':') + 2);
        // Process Registration Status
        registered = !urc_map["CEREG"].empty() && (urc_map["CEREG"].back()=='1' || urc_map["CEREG"].back()=='5');
        // Explain GNSS error codes
        if (received.find("+CME ERROR") == 0) {
            result = received;
            PRINT("[LPWA] GNSS ERROR: %s\n", gnss_error_table.at(urc_map["CME ERROR"]).c_str());
        }
    }
}

bool LPWAClass::wait_for(std::function<bool()> condition, char *cond_name, uint32_t timeout, int line) {
    uint32_t start = millis();
    uint32_t log = millis();
    while (!condition() && millis() - start < timeout) {
        delay(10);
        if (millis() - log > 1000) {
            PRINT("[LPWA] WAIT_FOR(%s, %d/%d) at line %d\n", cond_name, (millis()-start)/1000, timeout/1000, line);
            log = millis();
        }
    }
    delay(100);
    return condition();
}

std::string LPWAClass::at_send(std::string command) {
    result.clear();
    if (command.find("AT+CFUN=1,1") == 0) {
        gps_on = ready = false;
    } else if (command.find("AT&F1") == 0) {
        gps_on = ready = false;
        urc_map.clear();
    }
    at_port->println(command.c_str());
    at_port->flush();
    delay(500);
    WAIT_FOR(!result.empty(), COMMAND_TIMEOUT);
    return result;
}

std::string &LPWAClass::get_urc_list() {
    urc_list.clear();
    for (auto& [k, v] : urc_map) urc_list += k + "=" + v + " ";
    return urc_list;
}

void LPWAClass::enable_drx() {
    PRINT("[LPWA] Enable eDRX\n");
    WAIT_FOR(registered, REGISTRATION_TIMEOUT);
    at_send("AT+QPTWEDRXS=2,4,\"" + config.edrx_cycle + "\",\"" + config.edrx_ptw + "\"");
    at_send("AT+CEDRXRDP");
    PRINT("[LPWA] Enable sleep mode\n");
    digitalWrite(dtr_pin, HIGH);
    delay(200);
    digitalWrite(dtr_pin, LOW);
    at_send("AT+QSCLK=2");
}

void LPWAClass::disable_drx() {
    PRINT("[LPWA] Disable eDRX\n");
    at_send("AT+QPTWEDRXS=0");
    at_send("AT+QPTWEDRXS?");
    at_send("AT+CEDRXRDP");
}

void LPWAClass::enable_psm() {
    PRINT("[LPWA] Enable PSM\n");
    WAIT_FOR(registered, REGISTRATION_TIMEOUT);
    at_send("AT+QCFG=\"psm/enter\",1");
    at_send("AT+CPSMS=1,,,\"" + config.psm_t3324 + "\",\"" + config.psm_t3412 + "\"");
}

void LPWAClass::disable_psm() {
    PRINT("[LPWA] Disable PSM\n");
    at_send("AT+CPSMS=0");
    at_send("AT+CPSMS?");
}

void LPWAClass::start_gnss() {
    PRINT("[LPWA] Start GNSS\n");
    at_send("AT+QGPS=1,3,0,1");
    memset(&gnss_fix, 0, sizeof(gnss_fix));
    gnss_start = millis();
    gps_on = true;
}

void LPWAClass::stop_gnss() {
    PRINT("[LPWA] Stop GNSS\n");
    at_send("AT+QGPSEND");
    gps_on = false;
}

void LPWAClass::agnss_download() {
    PRINT("[LPWA] Download AGNSS data\n");
    WAIT_FOR(registered, REGISTRATION_TIMEOUT);

    urc_map.erase("QGPSURC");
    if (is_alt1350()) {
        at_send("AT+QGPSCFG=\"agnss_download\",1");
    } else {
        at_send("AT+QGPSCFG=\"xtra_download\",1");

        // Inject XTRA time (UTC) from network clock before triggering download
        // CCLK format: "YY/MM/DD,HH:MM:SS+TZ" where TZ is in quarter-hours from UTC
        at_send("AT+CCLK?");
        std::string clk = urc_map["CCLK"];  // e.g. "26/02/10,11:44:24+04"
        int yy, mo, dd, hh, mn, ss, tz = 0;
        sscanf(clk.c_str(), "\"%d/%d/%d,%d:%d:%d%d\"", &yy, &mo, &dd, &hh, &mn, &ss, &tz);
        struct tm t = {};
        t.tm_year = 100 + yy;       // years since 1900
        t.tm_mon  = mo - 1;
        t.tm_mday = dd;
        t.tm_hour = hh;
        t.tm_min  = mn - tz * 15;   // subtract TZ offset (quarter-hours) to get UTC
        t.tm_sec  = ss;
        mktime(&t);                 // normalize overflows
        char xtra_time[24];
        snprintf(xtra_time, sizeof(xtra_time), "%04d/%02d/%02d,%02d:%02d:%02d",
                    t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
        PRINT("[LPWA] Inject XTRA time: %s (CCLK TZ=%+d)\n", xtra_time, tz);
        at_send("AT+QGPSXTRATIME=0,\"" + std::string(xtra_time) + "\"");
        at_send("AT+QGPSXTRATIME?");
    }
    agnss_valid = false;
}

bool LPWAClass::agnss_check() {
    if (is_alt1350()) {
        if (urc_map["QGPSURC"].find("AGNSS_DL") == 1) {
            PRINT("[LPWA] AGNSS download complete, verify info\n");
            urc_map.erase("QGPSURC");
            at_send("AT+QGPSCFG=\"agnss_info\"");
            agnss_valid = true;
            return true;
        }
    } else {
        if (urc_map["QGPSURC"].find("XTRA_DL") == 1) {
            PRINT("[LPWA] AGNSS download complete, verify info\n");
            urc_map.erase("QGPSURC");
            at_send("AT+QGPSXTRADATA?");
            agnss_valid = true;
            return true;
        }
    }
    return false;
}

bool LPWAClass::mqtt_open(const std::string &host, uint16_t port) {
    at_send("AT+QIACT=1");
    at_send("AT+QMTCFG=\"keepalive\",0,0");
    urc_map.erase("QMTOPEN");
    at_send("AT+QMTOPEN=0,\"" + host + "\"," + std::to_string(port));
    if (!WAIT_FOR(!urc_map["QMTOPEN"].empty(), MQTT_TIMEOUT) 
                || urc_map["QMTOPEN"].find("0,0") != 0) {
        PRINT("[MQTT] Open failed: %s\n", urc_map["QMTOPEN"].c_str());
        return false;
    }
    PRINT("[MQTT] Opened %s:%d\n", host.c_str(), port);
    return true;
}

bool LPWAClass::mqtt_connect(const std::string &client_id) {
    urc_map.erase("QMTCONN");
    at_send("AT+QMTCONN=0,\"" + client_id + "\"");
    if (!WAIT_FOR(!urc_map["QMTCONN"].empty(), MQTT_TIMEOUT) 
                || urc_map["QMTCONN"].find("0,0,0") != 0) {
        PRINT("[MQTT] Connect failed: %s\n", urc_map["QMTCONN"].c_str());
        return false;
    }
    PRINT("[MQTT] Connected as %s\n", client_id.c_str());
    return true;
}

bool LPWAClass::mqtt_publish(const std::string &topic, const std::string &msg) {
    urc_map.erase("QMTPUB");
    at_send("AT+QMTPUBEX=0,0,0,0,\"" + topic + "\",\"" + msg + "\"");
    if (!WAIT_FOR(!urc_map["QMTPUB"].empty(), COMMAND_TIMEOUT) 
                || urc_map["QMTPUB"].find("0,0,0") != 0) {
        PRINT("[MQTT] Publish failed: %s\n", urc_map["QMTPUB"].c_str());
        return false;
    }
    PRINT("[MQTT] Published to %s\n", topic.c_str());
    return true;
}

bool LPWAClass::mqtt_disconnect() {
    urc_map.erase("QMTDISC");
    at_send("AT+QMTDISC=0");
    return true;
}

bool LPWAClass::mqtt_close() {
    urc_map.erase("QMTCLOSE");
    at_send("AT+QMTCLOSE=0");
    return true;
}

void LPWAClass::parse_rmc(const char *nmea) {
    int hh, mm, ss, day, mon, yr;
    float lat_raw, lon_raw;
    char status, ns, ew;

    if (sscanf(nmea, "$%*2cRMC,%2d%2d%2d.%*d,%c,%f,%c,%f,%c,%*f,%*f,%2d%2d%2d",
               &hh, &mm, &ss, &status, &lat_raw, &ns, &lon_raw, &ew, &day, &mon, &yr) >= 10) {
        if (status == 'A') {
            gnss_fix.lat = ((int)(lat_raw / 100) + fmod(lat_raw, 100.0) / 60.0) * (ns == 'S' ? -1 : 1);
            gnss_fix.lon = ((int)(lon_raw / 100) + fmod(lon_raw, 100.0) / 60.0) * (ew == 'W' ? -1 : 1);
            struct tm t = {ss, mm, hh, day, mon - 1, yr + 100, 0, 0, 0};
            gnss_fix.timestamp = mktime(&t);
            gnss_fix.count++;
        }
    }
}
