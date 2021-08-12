
// Thing name, master password etc.

#define STF_THING_NAME     "SimpleThing"
#define STF_THING_PASSWORD "Password"

// Wifi

#define STFTASK_WIFI 1

#define STFWIFI_SSID     "SSID"
#define STFWIFI_PASSWORD ""

#define STFWIFI_IOTWEBCONF_CONFVERSION "ver1"

// LED Handling

#define STFTASK_LED 1

#define STFLED_PINS LED_BUILTIN
#define STFLED_PWMS 0

// BLE Test mode -> the last octet of the MAC must be this value (before xor)
//#define STFBLE_TEST_MAC 0x0c
// BLE Test mode -> xor the last octet of the MAC so won't mess with the whole system
//#define STFBLE_TEST_XOR 0xff

// OTA

#define STFTASK_OTA 1

