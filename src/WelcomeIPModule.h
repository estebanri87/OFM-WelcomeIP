#pragma once

#include "WIPChannelOwnerModule.h"
#include "WipMqttLink.h"

#ifndef WIP_ChannelCount
#define WIP_ChannelCount 0 // until the ETS application defines it
#endif

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

    OpenKNX::Channel *createChannel(uint8_t _channelIndex) override;

    void showHelp() override;
    bool processCommand(const std::string command, bool debugKo) override;

    WipMqttLink &link() { return _link; }

  private:
    WipMqttLink _link;

    // Runtime configuration until the ETS application exists. Console: wip connect.
    std::string _host;
    std::string _user;
    std::string _pass;
    uint16_t _port = 8883;
    bool _tls = true;

    void connect();
    void dispatchDatapoint(const WelcomeIP::Address &a, const char *value);
};

extern WelcomeIPModule openknxWelcomeIPModule;
