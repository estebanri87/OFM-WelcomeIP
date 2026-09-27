#pragma once

#include "WIPChannelOwnerModule.h"
#include "WipMqttLink.h"

// Busch-Welcome IP door entry system on the KNX bus, via the Smart Access Point's
// local API (MQTT). Ring, door opener, door state and the IP actuator outputs.
// No video, no audio -- deliberately out of scope.
class WelcomeIPModule : public WIPChannelOwnerModule
{
  public:
    WelcomeIPModule();

    const std::string name() override;
    const std::string version() override;

    void setup(bool configured) override;
    void loop(bool configured) override;
    void processInputKo(GroupObject &ko) override;

    OpenKNX::Channel *createChannel(uint8_t _channelIndex) override;

    void showHelp() override;
    bool processCommand(const std::string command, bool debugKo) override;

    WipMqttLink &link() { return _link; }

    // Set from the global lock object; a channel checks it before opening.
    bool globallyLocked() const { return _globalLock; }
    // A channel reports its ring so the collective object can follow.
    void ringDetected();

  private:
    WipMqttLink _link;

    bool _globalLock = false;
    bool _lastConnected = false;
    uint32_t _ringAnyStarted = 0;

    void dispatchDatapoint(const WelcomeIP::Address &a, const char *value);
    void handleSmartApDatapoint(const WelcomeIP::Address &a, bool on);
};

extern WelcomeIPModule openknxWelcomeIPModule;
