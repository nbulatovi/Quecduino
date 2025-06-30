#ifndef GNSS_ERRORS_H
#define GNSS_ERRORS_H

#include <map>
#include <string>

// BG77xA-GL&BG95xA-GL GNSS Application Note
static const std::map<std::string, std::string> gnss_error_table = {
    {"501", "Invalid parameter"},
    {"502", "Operation not supported"},
    {"503", "GNSS subsystem busy"},
    {"504", "Session is ongoing"},
    {"505", "Session not active"},
    {"506", "Operation timeout"},
    {"507", "Function not enabled"},
    {"508", "Time information error"},
    {"509", "XTRA not enabled"},
    {"512", "Validity time is out of range"},
    {"513", "Internal resource error"},
    {"514", "GNSS locked"},
    {"515", "End by E911"},
    {"516", "No fix"},
    {"517", "Geo-fence ID does not exist"},
    {"518", "Sync time failed"},
    {"519", "XTRA file does not exist"},
    {"520", "XTRA file on downloading"},
    {"521", "XTRA file is valid"},
    {"522", "GNSS is working"},
    {"523", "Time injection error"},
    {"524", "XTRA file is invalid"},
    {"549", "Unknown error"}
};

#endif /* GNSS_ERRORS_H */
