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
    // TODO(Stufe3): read serial, device type, ring mode and the expert channel/datapoint
    // overrides from the ETS parameters once WelcomeIP.templ.xml exists.
}

bool WelcomeIPChannel::matches(const WelcomeIP::Address &a, uint8_t channel, uint8_t dp,
                               bool output) const
{
    return a.output == output && a.channel == channel && a.dp == dp &&
           _serial[0] && strcmp(a.serial, _serial) == 0;
}

void WelcomeIPChannel::onDatapoint(const WelcomeIP::Address &a, const char *value)
{
    if (!_serial[0]) return;

    if (matches(a, _ringChannel, _ringDp, true))
    {
        const bool ringing = value && value[0] == '1';
        if (!ringing) return;

        // Suppress repeats so one visitor leaning on the button does not restart the
        // chime over and over. 0 = every notification is passed on.
        if (_ringBlockMs && _lastRingSent && !delayCheck(_lastRingSent, _ringBlockMs)) return;
        _lastRingSent = millis();
        if (_lastRingSent == 0) _lastRingSent = 1;

        // TODO(Stufe3): KO_WIP_Ring = true, plus the module's collective ring KO.
        _ringStarted = millis();
        if (_ringStarted == 0) _ringStarted = 1;
        logDebugP("ring from %s", a.serial);
    }
}

void WelcomeIPChannel::loop()
{
    // Pulse mode: drop the ring KO again after the configured time.
    if (_ringStarted && delayCheck(_ringStarted, _ringPulseMs))
    {
        _ringStarted = 0;
        // TODO(Stufe3): KO_WIP_Ring = false
    }
}

void WelcomeIPChannel::processInputKo(GroupObject &ko)
{
    // TODO(Stufe3): map KO_WIP_Open -> _module.link().setDatapoint(...) once the ETS
    // application defines the objects. The address and the second-lock flag are already
    // resolved here, so only the KO plumbing is missing.
    (void)ko;
}
