# Quecduino

Quecduino is an Arduino ESP32 library example showing how to command Quectel LPWA modules to:
* Configure network category and band
* Configure connection to Quectel AGNSS demo proxy server
* Download XTRA AGNSS aiding file

## Supported modules

* BG770A-GL - Ultra-compact Cat M1/NB2 with GNSS
* BG77xA-GL series variants
* BG950A-GL - Compact Cat M1/NB1/NB2/GPRS with GNSS and iSIM
* BG952A-GL - Part of BG95xA-GL series
* BG953A-GL - Part of BG95xA-GL series
* BG955A-GL - Part of BG95xA-GL series

## Hardware setup used for testing 

* ESP-WROOM-32 board
* Quectel BG950A-GL-TE-B_V1.1
    * D25 <-> RXD
    * D26 <-> TXD
    * D27 <-> DTR
    * D14 <-> PON_TRIG
    * GND <-> GND

## Future plans

This project will be developed to show how to make a low power tracker device using Quectel LPWA modules.
