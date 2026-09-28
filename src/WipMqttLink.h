#pragma once
#include "WelcomeIPConfig.h"
#ifdef OPENKNX_WELCOMEIP


#include "OpenKNX/Network/MQTT/Module.h"
#include "WipProtocol.h"

#ifndef WIP_EVENT_QUEUE
#define WIP_EVENT_QUEUE 24
#endif

// Connection to the Smart Access Point's local API: owns a second MQTT client (the
// device's own one keeps talking to the house broker), sends the 30 s heartbeat the
// API requires, and hands decoded datapoints to the channels.
//
// Threading: on ESP32 the MQTT client runs in its own FreeRTOS task, so subscription
// callbacks do NOT run in the KNX loop. Writing a GroupObject from there would race
// the stack, so incoming datapoints are queued and delivered from loop() instead --
// the same record-and-defer rule the webserver follows for its callbacks.
class WipMqttLink
{
  public:
    struct Settings
    {
        const char *host = nullptr;
        uint16_t port = 8883;
        const char *username = nullptr;
        const char *password = nullptr;
        bool tls = true;
        const char *caCert = nullptr; // nullptr = no certificate validation
        size_t rxBufferSize = 0;      // 0 = module default
    };

    // Delivered from loop(), safe to touch KNX objects.
    using DatapointFn = std::function<void(const WelcomeIP::Address &, const char *value)>;
    using DeviceFn = std::function<void(const WelcomeIP::Device &)>;

    void setup(const Settings &s);
    void loop();

    bool connected() { return _mqtt.connected(); }

    // Queues a write. true = accepted for sending, not delivered.
    bool setDatapoint(const WelcomeIP::Address &a, const char *value, bool secondLock);

    // Asks for the full model. Sent once after connect, and on demand from the console.
    void requestGetAll();

    void onDatapoint(DatapointFn fn) { _onDatapoint = std::move(fn); }
    // Runs in the MQTT task -- console/diagnostic use only, never KNX.
    void onDevice(DeviceFn fn) { _onDevice = std::move(fn); }

    const char *smartApSerial() { return _protocol.smartApSerial(); }
    bool rawLog = false; // console: dump every received payload

  private:
    struct Event
    {
        WelcomeIP::Address address;
        char value[24];
    };

    OpenKNX::Network::MQTT::Module _mqtt;
    OpenKNX::Network::MQTT::Config _cfg;
    WelcomeIP::Protocol _protocol;

    DatapointFn _onDatapoint;
    DeviceFn _onDevice;

    bool _initialized = false;
    bool _wasConnected = false;
    uint32_t _lastHeartbeat = 0;

    // Single-producer (MQTT task) / single-consumer (KNX loop) ring.
    Event _events[WIP_EVENT_QUEUE];
    volatile uint16_t _evHead = 0; // written by the consumer
    volatile uint16_t _evTail = 0; // written by the producer
    uint32_t _evDropped = 0;

    void pushEvent(const WelcomeIP::Address &a, const char *value);
    void drainEvents();
    void publish(const std::string &topic, const std::string &payload);
};

#endif // OPENKNX_WELCOMEIP
