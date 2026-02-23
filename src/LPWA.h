// LPWA.h - Quecduino Arduino Library
// Copyright (c) 2025 Quectel Wireless Solutions, Co., Ltd.

#ifndef LPWA_H
#define LPWA_H

#include <Arduino.h>
#include <Ticker.h>
#include <map>
#include <string>
 
#define AT_BUF_SIZE             128
#define COMMAND_TIMEOUT         10000
#define MQTT_TIMEOUT            30000
#define NETWORK_TIMEOUT         180000
#define REGISTRATION_TIMEOUT    300000

#define WAIT_FOR(condition, timeout) LPWA.wait_for([&]() { return (condition); }, #condition, timeout, __LINE__)
#define PRINT Serial.printf

// Types of cellular modules this library supports
typedef enum {
    UNKNOWN,  // Not set
    BG770A, // BG770A-GL - Ultra-compact Cat M1/NB2 with GNSS
    BG772A, // BG77xA-GL series variants
    BG773A, // BG77xA-GL series variants
    BG950A, // BG950A-GL - Compact Cat M1/NB1/NB2/GPRS with GNSS and iSIM
    BG951A, // BG951A-GL - Part of BG95xA-GL series
    BG952A, // BG952A-GL - Part of BG95xA-GL series
    BG953A, // BG953A-GL - Part of BG95xA-GL series
    BG955A, // BG955A-GL - Part of BG95xA-GL series
    BG770S, // BG770S-GL - Ultra-compact Cat M1/NB2 with GNSS
    BG950S  // BG950S-GL - Compact Cat M1/NB1/NB2 with GNSS
} Module_t;

// GPS fix data from NMEA RMC message
struct GNSSFix {
    time_t      timestamp = 0;  // Unix timestamp (UTC)
    double      lat = 0.0;      // Latitude in decimal degrees
    double      lon = 0.0;      // Longitude in decimal degrees
    uint16_t    count = 0;      // Number of valid fixes since GPS start
};

// Main class for controlling cellular modules
class LPWAClass {
public:
    // Status flags - tell you what's happening with the module
    bool            ready = false;      // true when module is on and ready for commands
    bool            registered = false; // true when connected to cellular network
    bool            gps_on = false;     // true when GPS is turned on
    bool            agnss_valid = false; // true after AGNSS download completes

    // Latest GNSS fix from RMC parsing
    GNSSFix         gnss_fix;

    // GPS session tracking
    uint32_t        gnss_start = 0;

    // Unsolicited Response Codes received from the module
    std::map<std::string, std::string> urc_map;

    // Default settings for the module
    struct {
        Module_t    module = UNKNOWN;                                       // Which type of module you have
        std::string proxy_url = "https://44.228.248.147/BG950/cep_pak.bin"; // URL of Quectel AGNSS proxy server
        std::string catm_bands  = "308181A";    // Cat-M1 bands: US(2,4,5,12,13,25,26) + EU(20)
        std::string catnb_bands = "308181A";    // Cat-NB bands: same as Cat-M1
        std::string psm_t3324   = "00000010";   // PSM active timer (2s)
        std::string psm_t3412   = "00100001";   // PSM periodic TAU (1h)
        std::string edrx_cycle  = "1100";       // eDRX cycle (~22 min)
        std::string edrx_ptw    = "0011";       // eDRX paging time window (5.12s)
    } config;

    // Constructor
    LPWAClass() {}

    // Start the driver - call this first in setup()
    // at_port: Serial port for AT commands (for example: &Serial1)
    // dtr_pin: pin number for DTR control
    // wakeup_pin: pin number connected to PON_TRIG, for sleep/wake control 
    void begin(HardwareSerial *at_port, uint8_t dtr_pin, uint8_t wakeup_pin);

    // Stop the driver
    void end();

    // Configure the module for use - call this after begin()
    void configure();

    // Reset module back to factory defaults
    void factory_reset();

    // Wake up module from sleep mode
    void wakeup();

    // Put module to sleep to save battery
    void sleep();

    // Call this regularly in your loop() - handles background stuff
    void update();

    // Enable eDRX mode
    void enable_drx();

    // Disable eDRX mode
    void disable_drx();

    // Enable PSM mode to save power
    void enable_psm();

    // Disable PSM mode
    void disable_psm();

    // Turn on GPS to get location
    void start_gnss();

    // Turn off GPS to save power
    void stop_gnss();

    // Start AGNSS download (non-blocking)
    void agnss_download();

    // Check AGNSS download status, returns true if ready
    bool agnss_check();

    // Check if module uses ALT1350 chipset (S-series: BG770S, BG950S)
    bool is_alt1350();

    // MQTT functions
    bool mqtt_open(const std::string &host, uint16_t port);
    bool mqtt_connect(const std::string &client_id);
    bool mqtt_publish(const std::string &topic, const std::string &msg);
    bool mqtt_disconnect();
    bool mqtt_close();

    // Wait for something to happen, give up after timeout (milliseconds)
    // Returns true if it happened, false if timed out
    bool wait_for(std::function<bool()> condition, char *cond_name, uint32_t timeout, int line);

    // Send command to module and get response
    std::string at_send(std::string command);

    // Concatenate current list of Unsolicited Result Codes from urc_map
    std::string &get_urc_list();

protected:
    // Hardware pins and connections
    uint8_t             pon_trig_pin;
    uint8_t             dtr_pin;
    HardwareSerial *    at_port;

    // Internal data buffers
    std::string         received;
    std::string         result;
    std::string         urc_list;
    Ticker              ticker;

    // Internal functions
    void recv_task();
    void process_response();
    void parse_rmc(const char *nmea);
};

// Main LPWA object
extern LPWAClass LPWA;

#endif // LPWA_H
