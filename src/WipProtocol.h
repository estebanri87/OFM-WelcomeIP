#pragma once

#include <functional>
#include <stdint.h>
#include <string.h>
#include <string>

// Everything that knows the ABB Welcome IP wire format lives here -- topics, payload
// assembly and parsing. The channels above work on WipAddress only, so when the format
// is finally confirmed on real hardware, this file is the only one that changes.
//
// State: the published ABB documentation (developer.eu.mybuildings.abb.com/wip_local)
// describes the data model and shows a getAll response, but its code blocks for topics,
// setDatapoint, heartbeat and notification are empty. Every TODO(Stufe2) below marks a
// value taken from the surrounding evidence rather than from documentation; they are
// verified by sniffing the Smart Access Point once it is installed.

namespace WelcomeIP
{
    // References table, ABB developer portal ("References" -> DeviceTypeId)
    enum DeviceType : uint16_t
    {
        DT_SMART_ACCESS_POINT = 1,
        DT_CYLINDER = 2,
        DT_REPEATER = 3,
        DT_RF_IP_GATEWAY = 4,
        DT_OUTDOOR_STATION = 40,
        DT_INDOOR_STATION = 41,
        DT_IP_ACTUATOR = 42,
        DT_GUARD_UNIT = 45,
        DT_ELEVATOR_CONTROLLER = 49,
    };

    // Channel indices per device type, same table. These are defaults only: every one of
    // them is overridable as an ETS expert parameter, because the table is not versioned
    // against firmware.
    enum SmartApChannel : uint8_t
    {
        SAP_CH_DOORBELL = 1,
        SAP_CH_ALARM = 2,
        SAP_CH_BINARY_IN = 3,
        SAP_CH_BINARY_OUT = 4,
        SAP_CH_TAMPER = 5,
        SAP_CH_SECURITY_SWITCH = 6,
        SAP_CH_MUTE = 7,
        SAP_CH_DAYNIGHT = 8,
    };

    enum OutdoorChannel : uint8_t
    {
        OS_CH_TRIGGER = 0,
        OS_CH_LOCK = 1,
        OS_CH_INCOMING_CALL = 2,
    };

    enum ActuatorChannel : uint8_t
    {
        IPA_CH_TRIGGER = 0,
        IPA_CH_LIGHT = 1,
        IPA_CH_LOCK = 2,
    };

    // One datapoint. Channel and datapoint are indices, rendered as chXXXX / idpXXXX /
    // oidpXXXX on the wire.
    struct Address
    {
        char serial[20] = {};
        uint8_t channel = 0;
        uint8_t dp = 0;
        bool output = false; // true = oidp (device -> us), false = idp (us -> device)

        bool matches(const Address &o) const
        {
            return channel == o.channel && dp == o.dp && output == o.output &&
                   strcmp(serial, o.serial) == 0;
        }
    };

    // One device from a getAll response. The strings point into the parse buffer and are
    // only valid for the duration of the callback -- copy what you keep.
    struct Device
    {
        const char *serial = nullptr;
        const char *displayName = nullptr;
        uint16_t deviceTypeId = 0;
        uint8_t channelCount = 0;
    };

    using DatapointFn = std::function<void(const Address &, const char *value)>;
    using ResultFn = std::function<void(uint32_t queryId, int result)>;
    using DeviceFn = std::function<void(const Device &)>;

    class Protocol
    {
      public:
        // ---- outgoing ------------------------------------------------------------
        // Each builds topic + payload for one request. false = could not build.
        bool buildGetAll(std::string &topic, std::string &payload);
        bool buildSetDatapoint(const Address &a, const char *value, bool secondLock,
                               std::string &topic, std::string &payload);
        bool buildHeartbeat(std::string &topic, std::string &payload);

        // Topic filter to subscribe to for notifications.
        const char *notificationFilter() const;

        // ---- incoming ------------------------------------------------------------
        // Parses one received message and dispatches it. Datapoint updates go to
        // onDatapoint, getAll devices to onDevice, command acknowledgements to onResult.
        // Any of them may be empty. false = not understood (logged by the caller).
        bool parse(const char *topic, const uint8_t *payload, size_t len,
                   const DatapointFn &onDatapoint,
                   const DeviceFn &onDevice,
                   const ResultFn &onResult);

        // The Smart Access Point's own serial, learned from the getAll response. Empty
        // until the first one arrives.
        const char *smartApSerial() const { return _smartApSerial; }

        // Heartbeat interval. The API requires one every 30 s or notifications stop.
        static constexpr uint32_t heartbeatMs = 30000;

      private:
        uint32_t _queryId = 1;
        char _smartApSerial[20] = {};
    };

    // Renders "chXXXX" / "idpXXXX" / "oidpXXXX" into buf (at least 10 bytes).
    const char *channelName(uint8_t index, char *buf, size_t bufLen);
    const char *datapointName(uint8_t index, bool output, char *buf, size_t bufLen);
} // namespace WelcomeIP
