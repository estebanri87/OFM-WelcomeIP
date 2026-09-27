#include "WelcomeIPModule.h"
#include "WelcomeIPChannel.h"
#include <stdio.h>

// The collective ring object always pulses -- unlike a channel it has no single call to
// follow, because a second station may ring while the first one is still active.
#define WIP_RING_ANY_PULSE_MS 1000

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
    if (!configured || !ParamWIP_WIPActive) return;

    _link.onDatapoint([this](const WelcomeIP::Address &a, const char *value) {
        dispatchDatapoint(a, value);
    });

    WipMqttLink::Settings s;
    s.host = (const char *)ParamWIP_WIPHost;
    s.port = ParamWIP_WIPPort;
    s.username = (const char *)ParamWIP_WIPUser;
    s.password = (const char *)ParamWIP_WIPPass;
    s.tls = ParamWIP_WIPTls;
    // TODO(Stufe3): ParamWIP_WIPCertCheck == 1 loads /wip_ca.pem from LittleFS.
    s.caCert = nullptr;
    // A getAll response is 17-50 KB of JSON; the 1 KB default would reconnect forever.
    s.rxBufferSize = 64 * 1024;
    _link.setup(s);
}

void WelcomeIPModule::loop(bool configured)
{
    WIPChannelOwnerModule::loop(configured);
    if (!configured || !ParamWIP_WIPActive) return;

    _link.loop();

    const bool up = _link.connected();
    if (up != _lastConnected)
    {
        _lastConnected = up;
        KoWIP_WIPConnected.value(up, DPT_Switch);
        KoWIP_WIPDiag.value(up ? "API verbunden" : "API getrennt", DPT_String_8859_1);
    }

    if (_ringAnyStarted && delayCheck(_ringAnyStarted, WIP_RING_ANY_PULSE_MS))
    {
        _ringAnyStarted = 0;
        KoWIP_WIPRingAny.value(false, DPT_Switch);
    }
}

void WelcomeIPModule::ringDetected()
{
    KoWIP_WIPRingAny.value(true, DPT_Switch);
    _ringAnyStarted = millis();
    if (_ringAnyStarted == 0) _ringAnyStarted = 1;
}

void WelcomeIPModule::processInputKo(GroupObject &ko)
{
    WIPChannelOwnerModule::processInputKo(ko);
    if (!ParamWIP_WIPActive) return;

    const uint16_t asap = ko.asap();
    if (asap == WIP_KoWIPLockAll)
    {
        _globalLock = ko.value(DPT_Enable);
        return;
    }
    if (!ParamWIP_WIPSapEnable) return;

    const char *serial = _link.smartApSerial();
    if (!serial[0]) return;

    WelcomeIP::Address a;
    strncpy(a.serial, serial, sizeof(a.serial) - 1);
    a.dp = 0;
    a.output = false;

    switch (asap)
    {
        case WIP_KoWIPSapMute:
            a.channel = WelcomeIP::SAP_CH_MUTE;
            _link.setDatapoint(a, ko.value(DPT_Switch) ? "1" : "0", false);
            break;
        case WIP_KoWIPSapDayNight:
            a.channel = WelcomeIP::SAP_CH_DAYNIGHT;
            // 1.024 has no named constant in the knx library.
            _link.setDatapoint(a, ko.value(Dpt(1, 24)) ? "1" : "0", false);
            break;
        case WIP_KoWIPSapBinOut:
            a.channel = WelcomeIP::SAP_CH_BINARY_OUT;
            _link.setDatapoint(a, ko.value(DPT_Switch) ? "1" : "0", false);
            break;
    }
}

// Runs in the KNX loop (WipMqttLink queues and defers), so touching KNX is safe here.
void WelcomeIPModule::dispatchDatapoint(const WelcomeIP::Address &a, const char *value)
{
    const bool on = value && value[0] == '1';

    if (ParamWIP_WIPSapEnable)
    {
        const char *sap = _link.smartApSerial();
        if (sap[0] && strcmp(a.serial, sap) == 0)
        {
            handleSmartApDatapoint(a, on);
            return;
        }
    }

    for (uint8_t i = 0; i < getNumberOfChannels(); i++)
    {
        auto *channel = static_cast<WelcomeIPChannel *>(getChannel(i));
        if (channel) channel->onDatapoint(a, value);
    }
}

void WelcomeIPModule::handleSmartApDatapoint(const WelcomeIP::Address &a, bool on)
{
    if (!a.output) return;
    switch (a.channel)
    {
        case WelcomeIP::SAP_CH_DOORBELL:
            KoWIP_WIPSapDoorbell.value(on, DPT_Switch);
            if (on) ringDetected();
            break;
        case WelcomeIP::SAP_CH_MUTE:
            KoWIP_WIPSapMuteStat.value(on, DPT_Switch);
            break;
        case WelcomeIP::SAP_CH_BINARY_IN:
            KoWIP_WIPSapBinIn.value(on, DPT_Switch);
            break;
        case WelcomeIP::SAP_CH_ALARM:
            KoWIP_WIPSapAlarm.value(on, DPT_Alarm);
            break;
        case WelcomeIP::SAP_CH_TAMPER:
            KoWIP_WIPSapTamper.value(on, DPT_Alarm);
            break;
    }
}

void WelcomeIPModule::showHelp()
{
    openknx.console.printHelpLine("wip", "Welcome IP status");
    openknx.console.printHelpLine("wip devices", "Request the device model (getAll)");
    openknx.console.printHelpLine("wip set <sn> <ch> <dp> <val>", "Write an input datapoint");
    openknx.console.printHelpLine("wip raw on|off", "Log every received payload");
}

bool WelcomeIPModule::processCommand(const std::string command, bool debugKo)
{
    if (command.rfind("wip", 0) != 0) return false;

    if (command == "wip")
    {
        logInfoP("active: %s", ParamWIP_WIPActive ? "yes" : "no");
        logInfoP("host: %s:%u tls=%s", (const char *)ParamWIP_WIPHost,
                 (unsigned)ParamWIP_WIPPort, ParamWIP_WIPTls ? "yes" : "no");
        logInfoP("connected: %s", _link.connected() ? "yes" : "no");
        logInfoP("SmartAP serial: %s",
                 _link.smartApSerial()[0] ? _link.smartApSerial() : "(unknown)");
        logInfoP("channels: %u, global lock: %s",
                 (unsigned)getNumberOfChannels(), _globalLock ? "on" : "off");
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
