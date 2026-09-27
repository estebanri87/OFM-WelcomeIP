#include "WipMqttLink.h"
#include "OpenKNX.h"

void WipMqttLink::setup(const Settings &s)
{
    if (_initialized) return;
    if (!s.host || !s.host[0]) return;

    _cfg.server = s.host;
    _cfg.port = s.port;
    _cfg.username = s.username;
    _cfg.password = s.password;
    _cfg.clientId = "openknx-wip";
    _cfg.prefix = "";
    // A foreign broker rejects our status topics by ACL, and a last will on a topic
    // we do not own would be refused at CONNECT.
    _cfg.publishStatus = false;
    _cfg.useWill = false;
    _cfg.tls = s.tls;
    _cfg.caCert = s.caCert;
    _cfg.rxBufferSize = s.rxBufferSize;

    _mqtt.configure(_cfg);
    _mqtt.setup(true);

    // Runs in the MQTT task: parse here (cheap, no KNX), queue the result.
    _mqtt.subscribe(_protocol.notificationFilter(),
                    [this](const char *topic, const void *payload, size_t len) {
                        if (rawLog)
                            logInfo("WIP", "rx %s (%u B)", topic, (unsigned)len);
                        _protocol.parse(
                            topic, (const uint8_t *)payload, len,
                            [this](const WelcomeIP::Address &a, const char *v) { pushEvent(a, v); },
                            [this](const WelcomeIP::Device &d) { if (_onDevice) _onDevice(d); },
                            [](uint32_t queryId, int result) {
                                if (result != 0)
                                    logError("WIP", "request %u failed: result=%d", (unsigned)queryId, result);
                            });
                    },
                    0);

    _initialized = true;
}

void WipMqttLink::loop()
{
    if (!_initialized) return;
    _mqtt.loop(true);

    const bool up = _mqtt.connected();
    if (up && !_wasConnected)
    {
        logInfo("WIP", "local API connected");
        requestGetAll(); // the model may have changed while we were away
        _lastHeartbeat = millis();
    }
    else if (!up && _wasConnected)
    {
        logError("WIP", "local API disconnected");
    }
    _wasConnected = up;

    // Without this the Smart Access Point stops sending notifications.
    if (up && delayCheck(_lastHeartbeat, WelcomeIP::Protocol::heartbeatMs))
    {
        _lastHeartbeat = millis();
        std::string topic, payload;
        if (_protocol.buildHeartbeat(topic, payload)) publish(topic, payload);
    }

    drainEvents();

    if (_evDropped)
    {
        logError("WIP", "%u events dropped (queue full)", (unsigned)_evDropped);
        _evDropped = 0;
    }
}

void WipMqttLink::publish(const std::string &topic, const std::string &payload)
{
    _mqtt.publish(topic.c_str(), payload.c_str(), payload.size(), 0, false);
}

bool WipMqttLink::setDatapoint(const WelcomeIP::Address &a, const char *value, bool secondLock)
{
    if (!_initialized || !_mqtt.connected()) return false;
    std::string topic, payload;
    if (!_protocol.buildSetDatapoint(a, value, secondLock, topic, payload)) return false;
    publish(topic, payload);
    return true;
}

void WipMqttLink::requestGetAll()
{
    if (!_initialized) return;
    std::string topic, payload;
    if (_protocol.buildGetAll(topic, payload)) publish(topic, payload);
}

void WipMqttLink::pushEvent(const WelcomeIP::Address &a, const char *value)
{
    const uint16_t next = (uint16_t)((_evTail + 1) % WIP_EVENT_QUEUE);
    if (next == _evHead)
    {
        _evDropped++; // consumer is behind; dropping beats overwriting an unread event
        return;
    }
    Event &e = _events[_evTail];
    e.address = a;
    strncpy(e.value, value ? value : "", sizeof(e.value) - 1);
    e.value[sizeof(e.value) - 1] = '\0';
    _evTail = next; // published last, so the consumer never sees a half-written entry
}

void WipMqttLink::drainEvents()
{
    while (_evHead != _evTail)
    {
        const Event &e = _events[_evHead];
        if (_onDatapoint) _onDatapoint(e.address, e.value);
        _evHead = (uint16_t)((_evHead + 1) % WIP_EVENT_QUEUE);
    }
}
