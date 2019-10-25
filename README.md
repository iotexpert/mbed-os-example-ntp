# mbed-os-example-ntp

This is an example project that works with Cypress' WiFi development kits including
* CY8CKIT_062S2_43012
* CY8CKIT_062_WIFI_BT

The project also uses the CY8CKIT-028-TFT Shield.

Specifically this project connects to the WiFI network.  Then uses an NTP library to find the time.  It then saves the time in the RTC on the PSoC 6 as well as updating the screen.

The Screen displays the connection information and the time.

To use the program

* mbed import https://github.com/iotexpert/mbed-os-example-ntp.git
* cd mbed-os-example-ntp
* mbed compile -m CY8CKIT_062_WIFI_BT -t GCC_ARM -f

You can read about how this code works at https://iotexpert.com/2019/09/23/mbed-os-cy8ckit_062s2_43012-segger-emwin-ntp/
