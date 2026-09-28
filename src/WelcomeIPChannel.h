#pragma once
#include "WelcomeIPConfig.h"
#ifdef OPENKNX_WELCOMEIP


#include "OpenKNX.h"
#include "WipProtocol.h"

class WelcomeIPModule;

// One channel = one Welcome IP device. Which datapoints it maps follows the device type
// (References table of the ABB developer portal); the channel itself only ever deals in
// WelcomeIP::Address, never in topics or JSON.
class WelcomeIPChannel : public OpenKNX::Channel
{
  public:
    // Values of ParamWIP_CHType
    enum ChannelType : uint8_t
    {
        TYPE_NONE = 0,
        TYPE_OUTDOOR = 1,
        TYPE_ACTUATOR = 2,
        TYPE_INDOOR = 3,
        TYPE_GENERIC = 4,
    };

    WelcomeIPChannel(uint8_t index, WelcomeIPModule &module);

    const std::string name() override;

    void setup() override;
    void loop() override;
    void processInputKo(GroupObject &ko) override;

    // Called from the module for every datapoint update, in KNX loop context.
    void onDatapoint(const WelcomeIP::Address &a, const char *value);

  private:
    // Values of ParamWIP_CHOpenPath
    enum OpenPath : uint8_t
    {
        OPEN_POTENTIAL_FREE = 0,
        OPEN_LOCK_CONTACT = 1,
        OPEN_LIGHT_CHANNEL = 2,
        OPEN_VIA_OUTDOOR = 3,
    };
    enum Trigger : uint8_t
    {
        TRIGGER_ON = 0,
        TRIGGER_OFF = 1,
        TRIGGER_BOTH = 2,
    };

    WelcomeIPModule &_module;

    uint8_t _type = TYPE_NONE;
    char _serial[20] = {};
    char _serialOutdoor[20] = {};

    uint8_t _ringChannel = 0;
    uint8_t _ringDp = 0;
    uint8_t _openChannel = 0;
    uint8_t _openDp = 0;
    uint8_t _stateChannel = 0;
    uint8_t _stateDp = 0;
    const char *_openTarget = nullptr; // points at _serial or _serialOutdoor
    bool _openSecondLock = false;

    uint8_t _openTrigger = TRIGGER_ON;
    bool _lockEnabled = false;
    bool _lightEnabled = false;
    bool _stateEnabled = false;
    bool _ringHold = false; // false = pulse, true = for the duration of the call
    bool _locked = false;

    uint32_t _ringPulseMs = 1000;
    uint32_t _ringBlockMs = 0;
    uint32_t _ringStarted = 0;  // 0 = no pulse running
    uint32_t _lastRingSent = 0; // for the block time
    bool _ringActive = false;

    void resolveOpenPath(bool expert);
    bool matches(const WelcomeIP::Address &a, uint8_t channel, uint8_t dp) const;
    bool triggerMatches(bool value) const;
    void handleRing(bool ringing);
    void openDoor();
    static WelcomeIP::Address address(const char *serial, uint8_t channel, uint8_t dp);
};

#endif // OPENKNX_WELCOMEIP
