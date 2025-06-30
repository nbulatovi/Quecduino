#ifndef LPWA_H
#define LPWA_H

#include <Arduino.h>
#include <Ticker.h>

#include <map>
#include <string>

// Types of cellular modules this library supports
typedef enum {
    BG770A, // BG770A-GL - Ultra-compact Cat M1/NB2 with GNSS
    BG772A, // BG77xA-GL series variants
    BG773A, // BG77xA-GL series variants
    BG950A, // BG950A-GL - Compact Cat M1/NB1/NB2/GPRS with GNSS and iSIM
    BG951A, // BG951A-GL - Part of BG95xA-GL series
    BG952A, // BG952A-GL - Part of BG95xA-GL series
    BG953A, // BG953A-GL - Part of BG95xA-GL series
    BG955A  // BG955A-GL - Part of BG95xA-GL series
} Module_t;

// Main class for controlling cellular modules
class LPWAClass {
public:
    // Status flags - tell you what's happening with the module
    bool            ready = false;      // true when module is on and ready for commands
    bool            registered = false; // true when connected to cellular network
    bool            gps_on = false;     // true when GPS is turned on

    // Unsolicited Response Codes received from the module
    std::map<std::string, std::string> urc_map;

    // Default settings for the module
    struct {
        Module_t    module = BG950A;                                        // Which type of module you have
        std::string proxy_url = "https://44.228.248.147/BG950/cep_pak.bin"; // URL for AGNSS data server
    } config;

    // Constructor
    LPWAClass() {}

    // Start the driver - call this first in setup()
    // at_port: which Serial port to use (like &Serial1)
    // dtr_pin: pin number for DTR control
    // wakeup_pin: PON_TRIG pin number for sleep/wake control on RK3_00-based firmware versions
    void begin(HardwareSerial *at_port, uint8_t dtr_pin, uint8_t wakeup_pin);

    // Stop the driver
    void end();

    // Reset module back to factory defaults
    void factory_reset();

    // Configure the module for use - call this after begin()
    void configure();

    // Wake up module from sleep mode
    void wakeup();

    // Put module to sleep to save battery
    void sleep();

    // Call this regularly in your loop() - handles background stuff
    void update();

    // Turn on GPS to get location
    void start_gnss();

    // Turn off GPS to save power
    void stop_gnss();

    // Wait for something to happen, give up after timeout (milliseconds)
    // Returns true if it happened, false if timed out
    bool wait_for(std::function<bool()> condition, char *cond_name, uint32_t timeout);

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
};

// Main LPWA object
extern LPWAClass LPWA;

#endif // LPWA_H
