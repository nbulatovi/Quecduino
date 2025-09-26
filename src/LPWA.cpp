#include "LPWA.h"
#include "gnss_errors.h"

LPWAClass LPWA;

void LPWAClass::recv_task() {
    while (at_port->available() && received.length() < AT_BUF_SIZE-1) {
        char ch = at_port->read();
        if (ch != 0x0D && ch != 0x0A) {
            received += ch;
        } else if (!received.empty()) {
            process_response();
            received.clear();
        }
    }
}

void LPWAClass::begin(HardwareSerial *at_port0, uint8_t dtr_pin0, uint8_t wakeup_pin) {
    at_port = at_port0;
    dtr_pin = dtr_pin0;
    pon_trig_pin = wakeup_pin;
    pinMode(pon_trig_pin, OUTPUT);
    // pinMode(dtr_pin, OUTPUT);
    received.reserve(AT_BUF_SIZE);
    result.reserve(AT_BUF_SIZE);
    urc_list.reserve(256);
    ticker.attach(0.01, [this]() { this->recv_task(); });
}

void LPWAClass::end() {
    ticker.detach();
}

void LPWAClass::update() {
    PRINT("[LPWA] %s\n", LPWA.get_urc_list().c_str());
}

void LPWAClass::wakeup() {
    PRINT("[LPWA] wakeup\n");
    digitalWrite(pon_trig_pin, HIGH);
    digitalWrite(dtr_pin, LOW);
}

void LPWAClass::sleep() {
    PRINT("[LPWA] sleep\n");
    digitalWrite(pon_trig_pin, LOW);
    digitalWrite(dtr_pin, HIGH);
}

void LPWAClass::process_response() {
    PRINT("[AT] %s\n", received.c_str());
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
    } else if (received.find("NP Package: RK_")==0 && received.find("NP Package: RK_03_00")!=0) {
        PRINT("[LPWA] ERROR: unsupported RK version");
    }
}

bool LPWAClass::wait_for(std::function<bool()> condition, char *cond_name, uint32_t timeout) {
    uint32_t start = millis();
    uint32_t log = millis();
    while (!condition() && millis() - start < timeout) {
        delay(10);
        if (millis() - log > 1000) {
            PRINT("[LPWA] WAIT_FOR(%s, %d/%d)\n", cond_name, millis()-start, timeout);
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
    WAIT_FOR(!result.empty(), COMMAND_TIMEOUT);
    return result;
}

std::string &LPWAClass::get_urc_list() {
    urc_list.clear();
    for (auto& [k, v] : urc_map) urc_list += k + "=" + v + " ";
    return urc_list;
}

void LPWAClass::reset() {
    received.clear();
    result.clear();
    urc_map.clear();

    PRINT("[LPWA] Wait modem ready\n");
    at_send("AT"); // for easier testing with dev board
    WAIT_FOR(ready, NETWORK_TIMEOUT);

    PRINT("[LPWA] Factory reset\n");
    LPWA.factory_reset();
    WAIT_FOR(ready, NETWORK_TIMEOUT);

    PRINT("[LPWA] Read versions\n");
    at_send("AT+CGMM");
    at_send("AT%VER");
    PRINT("[LPWA] Enable AGNSS\n");
    at_send("AT+QGPSXTRA=1");
    PRINT("[LPWA] Configure network\n");
    at_send("AT+QCFG=\"band\",0,80000,80000,1");    // Band 20
    PRINT("[LPWA] Reset modem\n");
    ready = false;
    at_send("AT+CFUN=1,1");
    WAIT_FOR(ready, NETWORK_TIMEOUT);

    if (!LPWA.ready) PRINT("[LPWA] ERROR: Timed out waiting for modem reset\n");

    PRINT("[LPWA] Subscribe to URC\n");
    at_send("AT+CMEE=2");
    at_send("AT+CEREG=1");
    at_send("AT+QCFG=\"psm/urc\",1");

    PRINT("[LPWA] Configure GPS\n");
    at_send("AT+QGPSCFG=\"priority\",1,1");
    at_send("AT%IGNSSEV=\"ALL\",1");
    at_send("AT%IGNSSTST=\"DEBUGNMEA\",\"0x0e0c03ef\"");
    PRINT("[LPWA] Configure AGNSS proxy server\n");
    at_send("AT+QGPSCFG=\"xtra_cfg\",\"" + config.proxy_url + "\"");
    at_send("AT+QGPSCFG=\"xtrafilesize\",7");

    at_send("AT+QGPSXTRA?");
    at_send("AT+QGPSXTRATIME?");

    PRINT("[LPWA] Save settings\n");
    at_send("AT&W");

    PRINT("[LPWA] Query status\n");
    at_send("AT+CEDRXRDP");
    at_send("AT+CPSMS?");
    at_send("AT+QGMR?");
}

void LPWAClass::factory_reset() {
    WAIT_FOR(ready, NETWORK_TIMEOUT);
    at_send("AT+QGPSDEL=0");
    at_send("AT&F1");
    WAIT_FOR(ready, NETWORK_TIMEOUT);
}

void LPWAClass::enable_drx() {
    WAIT_FOR(registered, REGISTRATION_TIMEOUT);
    at_send("AT+QPTWEDRXS=2,4,\"0011\",\"1111\"");
    at_send("AT+CEDRXRDP");
    digitalWrite(dtr_pin, HIGH);
    delay(200);
    digitalWrite(dtr_pin, LOW);
    at_send("AT+QSCLK=2");
}

void LPWAClass::disable_drx() {
    at_send("AT+QPTWEDRXS=0");
    at_send("AT+QPTWEDRXS?");
    at_send("AT+CEDRXRDP");
}

void LPWAClass::enable_psm() {
    WAIT_FOR(registered, REGISTRATION_TIMEOUT);
    at_send("AT+QCFG=\"psm/enter\",1");
    at_send("AT+CPSMS=1,,,\"10101010\",\"00001111\"");
}

void LPWAClass::disable_psm() {
    at_send("AT+CPSMS=0");
    at_send("AT+CPSMS?");
}

void LPWAClass::start_gnss() {
    at_send("AT+QGPS=1,3,0,1");
}

void LPWAClass::stop_gnss() {
    at_send("AT+QGPSEND");
}
