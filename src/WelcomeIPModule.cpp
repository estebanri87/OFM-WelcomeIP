#include "WelcomeIPModule.h"
#include "WelcomeIPChannel.h"
#include <stdio.h>

WelcomeIPModule::WelcomeIPModule()
    : WIPChannelOwnerModule(WIP_ChannelCount)
{
}

const std::string WelcomeIPModule::name()
{
    return "WelcomeIP";
}

const std::string WelcomeIPModule::version()
{
    return "0.1.0";
}

OpenKNX::Channel *WelcomeIPModule::createChannel(uint8_t _channelIndex)
{
    return new WelcomeIPChannel(_channelIndex, *this);
}

void WelcomeIPModule::setup(bool configured)
{
    WIPChannelOwnerModule::setup(configured);

    _link.onDatapoint([this](const WelcomeIP::Address &a, const char *value) {
        dispatchDatapoint(a, value);
    });
}

void WelcomeIPModule::loop(bool configured)
{
    WIPChannelOwnerModule::loop(configured);
    _link.loop();
}

// Runs in the KNX loop (WipMqttLink queues and defers), so touching KNX is safe here.
void WelcomeIPModule::dispatchDatapoint(const WelcomeIP::Address &a, const char *value)
{
    for (uint8_t i = 0; i < getNumberOfChannels(); i++)
    {
        auto *channel = static_cast<WelcomeIPChannel *>(getChannel(i));
        if (channel) channel->onDatapoint(a, value);
    }
}

void WelcomeIPModule::connect()
{
    WipMqttLink::Settings s;
    s.host = _host.c_str();
    s.port = _port;
    s.username = _user.empty() ? nullptr : _user.c_str();
    s.password = _pass.empty() ? nullptr : _pass.c_str();
    s.tls = _tls;
    // A getAll response is 17-50 KB of JSON; the 1 KB default would reconnect forever.
    s.rxBufferSize = 64 * 1024;
    _link.setup(s);
}

void WelcomeIPModule::showHelp()
{
    openknx.console.printHelpLine("wip", "Welcome IP status");
    openknx.console.printHelpLine("wip connect <host> [user] [pass]", "Connect to the local API");
    openknx.console.printHelpLine("wip notls", "Use plain MQTT (port 1883) for the next connect");
    openknx.console.printHelpLine("wip devices", "Request the device model (getAll)");
    openknx.console.printHelpLine("wip set <sn> <ch> <dp> <val>", "Write an input datapoint");
    openknx.console.printHelpLine("wip raw on|off", "Log every received payload");
}

bool WelcomeIPModule::processCommand(const std::string command, bool debugKo)
{
    if (command.rfind("wip", 0) != 0) return false;

    if (command == "wip")
    {
        logInfoP("host: %s:%u tls=%s", _host.empty() ? "(unset)" : _host.c_str(),
                 (unsigned)_port, _tls ? "yes" : "no");
        logInfoP("connected: %s", _link.connected() ? "yes" : "no");
        logInfoP("SmartAP serial: %s",
                 _link.smartApSerial()[0] ? _link.smartApSerial() : "(unknown)");
        logInfoP("channels: %u", (unsigned)getNumberOfChannels());
        return true;
    }

    if (command.rfind("wip connect ", 0) == 0)
    {
        char host[64] = {}, user[64] = {}, pass[64] = {};
        const int n = sscanf(command.c_str() + 12, "%63s %63s %63s", host, user, pass);
        if (n < 1)
        {
            logErrorP("usage: wip connect <host> [user] [pass]");
            return true;
        }
        _host = host;
        _user = (n > 1) ? user : "";
        _pass = (n > 2) ? pass : "";
        connect();
        logInfoP("connecting to %s:%u", _host.c_str(), (unsigned)_port);
        return true;
    }

    if (command == "wip notls")
    {
        _tls = false;
        _port = 1883;
        logInfoP("plain MQTT on port %u for the next connect", (unsigned)_port);
        return true;
    }

    if (command == "wip devices")
    {
        _link.onDevice([this](const WelcomeIP::Device &d) {
            logInfoP("%-16s type=%-3u ch=%-2u %s", d.serial, (unsigned)d.deviceTypeId,
                     (unsigned)d.channelCount, d.displayName ? d.displayName : "");
        });
        _link.requestGetAll();
        logInfoP("getAll requested");
        return true;
    }

    if (command.rfind("wip set ", 0) == 0)
    {
        char sn[24] = {}, val[24] = {};
        unsigned ch = 0, dp = 0;
        if (sscanf(command.c_str() + 8, "%23s %u %u %23s", sn, &ch, &dp, val) != 4)
        {
            logErrorP("usage: wip set <sn> <ch> <dp> <val>");
            return true;
        }
        WelcomeIP::Address a;
        strncpy(a.serial, sn, sizeof(a.serial) - 1);
        a.channel = (uint8_t)ch;
        a.dp = (uint8_t)dp;
        a.output = false;
        logInfoP("set %s ch%u dp%u = %s: %s", sn, ch, dp, val,
                 _link.setDatapoint(a, val, false) ? "sent" : "failed");
        return true;
    }

    if (command.rfind("wip raw", 0) == 0)
    {
        _link.rawLog = (command.find("on") != std::string::npos);
        logInfoP("raw logging %s", _link.rawLog ? "on" : "off");
        return true;
    }

    return false;
}
