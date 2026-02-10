// gnss_errors.h - Quecduino Arduino Library
// Copyright (c) 2025 Quectel Wireless Solutions, Co., Ltd.

#ifndef GNSS_ERRORS_H
#define GNSS_ERRORS_H

#include <map>
#include <string>

// BGXXXS GNSS Error Codes (from alt1350 SDK)
static const std::map<std::string, std::string> gnss_error_table = {
    {"501", "Invalid parameter"},
    {"502", "Operation not supported"},
    {"503", "GNSS subsystem busy"},
    {"504", "Session is ongoing"},
    {"505", "Session not active"},
    {"506", "Operation timeout"},
    {"507", "Function not enabled"},
    {"508", "Time information error"},
    {"509", "AGNSS not enabled"},
    {"510", "AGNSS file open failed"},
    {"511", "Bad CRC for AGNSS data file"},
    {"512", "Validity time is out of range"},
    {"513", "Internal resource error"},
    {"514", "GNSS locked"},
    {"515", "End by E911"},
    {"516", "Not fixed now"},
    {"517", "Geo-fence ID does not exist"},
    {"518", "Not get time now"},
    {"519", "AGNSS file does not exist"},
    {"520", "AGNSS file downloading"},
    {"521", "AGNSS data is valid"},
    {"522", "GNSS is working"},
    {"523", "Time injection error"},
    {"524", "AGNSS data is invalid"},
    {"549", "Unknown error"}
};

#endif /* GNSS_ERRORS_H */
