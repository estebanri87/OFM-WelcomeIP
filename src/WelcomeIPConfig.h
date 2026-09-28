#pragma once

// OFM-WelcomeIP is ESP32-only and needs the MQTT client of OFM-Network.
//
// RP2040 is excluded for two independent reasons: MQTT::Client keeps a static instance
// pointer for its lwIP callbacks, so a second client would break the device's own one,
// and there is no TLS there. Without OPENKNX_MQTT the client is not compiled at all,
// so the module would not even build.
//
// Every header and source file of this module is wrapped in OPENKNX_WELCOMEIP, and the
// parent project guards its addModule() call with it. This header itself stays
// unguarded so that guard can be evaluated.

#if defined(ARDUINO_ARCH_ESP32) && defined(OPENKNX_MQTT) && (defined(KNX_IP_WIFI) || defined(KNX_IP_LAN))
#define OPENKNX_WELCOMEIP 1
#endif
