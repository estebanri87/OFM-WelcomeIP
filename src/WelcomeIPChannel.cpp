#include "WelcomeIPConfig.h"
#ifdef OPENKNX_WELCOMEIP

#include "WelcomeIPChannel.h"
#include "WelcomeIPModule.h"

WelcomeIPChannel::WelcomeIPChannel(uint8_t index, WelcomeIPModule &module)
    : _module(module)
{
    _channelIndex = index;
}

const std::string WelcomeIPChannel::name()
{
    return "WelcomeIPChannel";
}

void WelcomeIPChannel::setup()
{
    _type = ParamWIP_CHType;
    if (_type == TYPE_NONE) return;

    strncpy(_serial, (const char *)ParamWIP_CHSerial, sizeof(_serial) - 1);
    strncpy(_serialOutdoor, (const char *)ParamWIP_CHSerialOutdoor, sizeof(_serialOutdoor) - 1);

    _openTrigger = ParamWIP_CHOpenTrigger;
    _lockEnabled = ParamWIP_CHLockEnable;
    _ringHold = ParamWIP_CHRingMode;          // 0 = pulse, 1 = for the duration of the call
    _ringPulseMs = ParamWIP_CHRingPulse * 100; // parameter is in 0.1 s steps
    _ringBlockMs = ParamWIP_CHRingBlock * 1000;
    _lightEnabled = ParamWIP_CHLightEnable;
    _stateEnabled = ParamWIP_CHStateEnable;

    // Defaults follow the References table of the ABB developer portal; the expert
    // section overrides them, because that table is not versioned against firmware.
    const bool expert = ParamWIP_CHExpert;
    _ringChannel = expert ? ParamWIP_CHRingCh : WelcomeIP::OS_CH_INCOMING_CALL;
    _ringDp = expert ? ParamWIP_CHRingDp : 0;
    _stateChannel = expert ? ParamWIP_CHStateCh : 0;
    _stateDp = expert ? ParamWIP_CHStateDp : 0;

    resolveOpenPath(expert);

    logDebugP("channel %u: type=%u serial=%s open=ch%u/idp%u second=%u",
              (unsigned)_channelIndex, (unsigned)_type, _serial,
              (unsigned)_openChannel, (unsigned)_openDp, (unsigned)_openSecondLock);
}

// The H8304 has two separate lock outputs and the API channel that drives each one is
// undocumented, so the path is a parameter rather than a constant. See WIP-Oeffnungsweg.
void WelcomeIPChannel::resolveOpenPath(bool expert)
{
    _openTarget = _serial;
    _openSecondLock = false;

    if (_type == TYPE_OUTDOOR)
    {
        _openChannel = WelcomeIP::OS_CH_LOCK;
        _openDp = 0;
    }
    else if (_type == TYPE_ACTUATOR)
    {
        switch (ParamWIP_CHOpenPath)
        {
            case OPEN_POTENTIAL_FREE:
                _openChannel = WelcomeIP::IPA_CH_LOCK;
                _openDp = 0;
                _openSecondLock = true;
                break;
            case OPEN_LOCK_CONTACT:
                _openChannel = WelcomeIP::IPA_CH_LOCK;
                _openDp = 0;
                break;
            case OPEN_LIGHT_CHANNEL:
                _openChannel = WelcomeIP::IPA_CH_LIGHT;
                _openDp = 0;
                break;
            case OPEN_VIA_OUTDOOR:
                _openChannel = WelcomeIP::OS_CH_LOCK;
                _openDp = 0;
                _openSecondLock = true;
                _openTarget = _serialOutdoor;
                break;
        }
    }

    if (expert)
    {
        _openChannel = ParamWIP_CHOpenCh;
        _openDp = ParamWIP_CHOpenDp;
    }
}

bool WelcomeIPChannel::matches(const WelcomeIP::Address &a, uint8_t channel, uint8_t dp) const
{
    return a.output && a.channel == channel && a.dp == dp &&
           _serial[0] && strcmp(a.serial, _serial) == 0;
}

void WelcomeIPChannel::onDatapoint(const WelcomeIP::Address &a, const char *value)
{
    if (_type == TYPE_NONE || !_serial[0]) return;

    const bool on = value && value[0] == '1';

    if (_type == TYPE_OUTDOOR && matches(a, _ringChannel, _ringDp))
    {
        handleRing(on);
        return;
    }

    if (_stateEnabled && matches(a, _stateChannel, _stateDp))
    {
        KoWIP_CHDoorState.value(on, DPT_OpenClose);
        return;
    }

    // The actuator reports every unlock it performs, which is what "Tueroeffner aktiv"
    // shows -- including one triggered at the device itself, not only by us.
    if (matches(a, _openChannel, _openDp))
    {
        KoWIP_CHOpenActive.value(on, DPT_Switch);
        return;
    }

    if (_lightEnabled && _type == TYPE_ACTUATOR &&
        matches(a, WelcomeIP::IPA_CH_LIGHT, 0))
    {
        KoWIP_CHLightStatus.value(on, DPT_Switch);
        return;
    }

    if (_type == TYPE_GENERIC && a.output && a.channel == ParamWIP_CHRingCh &&
        a.dp == ParamWIP_CHRingDp)
    {
        KoWIP_CHGenericOut.value(on, DPT_Switch);
    }
}

void WelcomeIPChannel::handleRing(bool ringing)
{
    if (!ringing)
    {
        // Only "for the duration of the call" tracks the falling edge; a pulse ends on
        // its own timer.
        if (_ringHold && _ringActive)
        {
            _ringActive = false;
            KoWIP_CHRing.value(false, DPT_Switch);
        }
        return;
    }

    // Suppress repeats so one visitor leaning on the button does not restart an
    // external chime over and over. 0 = pass every notification on.
    if (_ringBlockMs && _lastRingSent && !delayCheck(_lastRingSent, _ringBlockMs)) return;
    _lastRingSent = millis();
    if (_lastRingSent == 0) _lastRingSent = 1;

    _ringActive = true;
    KoWIP_CHRing.value(true, DPT_Switch);
    _module.ringDetected();

    if (!_ringHold)
    {
        _ringStarted = millis();
        if (_ringStarted == 0) _ringStarted = 1;
    }
}

void WelcomeIPChannel::loop()
{
    if (_ringStarted && delayCheck(_ringStarted, _ringPulseMs))
    {
        _ringStarted = 0;
        _ringActive = false;
        KoWIP_CHRing.value(false, DPT_Switch);
    }
}

void WelcomeIPChannel::processInputKo(GroupObject &ko)
{
    if (_type == TYPE_NONE) return;

    const uint8_t index = WIP_KoCalcIndex(ko.asap());
    switch (index)
    {
        case WIP_KoCHLock:
            _locked = ko.value(DPT_Enable);
            break;

        case WIP_KoCHOpen:
            if (triggerMatches(ko.value(DPT_Switch))) openDoor();
            break;

        case WIP_KoCHLight:
            if (_lightEnabled && _type == TYPE_ACTUATOR)
            {
                WelcomeIP::Address a = address(_serial, WelcomeIP::IPA_CH_LIGHT, 0);
                _module.link().setDatapoint(a, ko.value(DPT_Switch) ? "1" : "0", false);
            }
            break;

        case WIP_KoCHGenericIn:
            if (_type == TYPE_GENERIC)
            {
                WelcomeIP::Address a = address(_serial, ParamWIP_CHRingCh, ParamWIP_CHRingDp);
                _module.link().setDatapoint(a, ko.value(DPT_Switch) ? "1" : "0", false);
            }
            break;
    }
}

bool WelcomeIPChannel::triggerMatches(bool value) const
{
    switch (_openTrigger)
    {
        case TRIGGER_ON: return value;
        case TRIGGER_OFF: return !value;
        default: return true; // both
    }
}

WelcomeIP::Address WelcomeIPChannel::address(const char *serial, uint8_t channel, uint8_t dp)
{
    WelcomeIP::Address a;
    strncpy(a.serial, serial ? serial : "", sizeof(a.serial) - 1);
    a.channel = channel;
    a.dp = dp;
    a.output = false;
    return a;
}

void WelcomeIPChannel::openDoor()
{
    if (_locked || _module.globallyLocked())
    {
        logInfoP("open rejected: locked");
        KoWIP_CHError.value(true, DPT_Alarm);
        return;
    }
    if (!_openTarget || !_openTarget[0])
    {
        logErrorP("open rejected: no serial number configured");
        KoWIP_CHError.value(true, DPT_Alarm);
        return;
    }

    WelcomeIP::Address a = address(_openTarget, _openChannel, _openDp);
    const bool sent = _module.link().setDatapoint(a, "1", _openSecondLock);
    KoWIP_CHError.value(!sent, DPT_Alarm);
    if (!sent) logErrorP("open failed: local API not connected");
}

#endif // OPENKNX_WELCOMEIP
