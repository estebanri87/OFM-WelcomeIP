#pragma once

#include "OpenKNX.h"
#include "WipProtocol.h"

class WelcomeIPModule;

// One channel = one Welcome IP device. Which datapoints it maps follows the device
// type (References table of the ABB developer portal); the channel itself only ever
// deals in WelcomeIP::Address, never in topics or JSON.
class WelcomeIPChannel : public OpenKNX::Channel
{
  public:
    WelcomeIPChannel(uint8_t index, WelcomeIPModule &module);

    const std::string name() override;

    void setup() override;
    void loop() override;
    void processInputKo(GroupObject &ko) override;

    // Called from the module for every datapoint update, in KNX loop context.
    void onDatapoint(const WelcomeIP::Address &a, const char *value);

  private:
    WelcomeIPModule &_module;

    // Filled from the ETS parameters once the application exists; the defaults below
    // come from the References table.
    char _serial[20] = {};
    uint16_t _deviceType = 0;

    uint8_t _ringChannel = WelcomeIP::OS_CH_INCOMING_CALL;
    uint8_t _ringDp = 0;
    uint8_t _openChannel = WelcomeIP::IPA_CH_LOCK;
    uint8_t _openDp = 0;
    bool _openSecondLock = true;

    uint32_t _ringPulseMs = 1000;
    uint32_t _ringBlockMs = 0;
    uint32_t _ringStarted = 0;  // 0 = no pulse running
    uint32_t _lastRingSent = 0; // for the block time

    bool matches(const WelcomeIP::Address &a, uint8_t channel, uint8_t dp, bool output) const;
};
