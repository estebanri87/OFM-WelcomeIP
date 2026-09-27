#include "WipProtocol.h"
#include "OpenKNX.h"
#include <ArduinoJson.h>
#include <stdio.h>
#include <stdlib.h>

namespace WelcomeIP
{
    // ---------------------------------------------------------------------------
    // Topics
    //
    // TODO(Stufe2): every string in this block is a placeholder. The ABB portal page
    // "wip_local/definition" names the three operations but ships empty code blocks,
    // so the actual topics are unknown. They are recovered by subscribing to '#' on
    // the Smart Access Point and by reading the web UI's own WebSocket traffic --
    // the getAll envelope (method/queryid/jid/sessionjwt) suggests the UI speaks the
    // same JSON. Until then these keep the module testable against a local broker.
    // ---------------------------------------------------------------------------
    static const char *TOPIC_REQUEST = "welcomeip/request";     // TODO(Stufe2)
    static const char *TOPIC_HEARTBEAT = "welcomeip/heartbeat"; // TODO(Stufe2)
    // One filter for responses and notifications alike: incoming messages are told
    // apart by their payload, not by their topic, so the exact response topic does
    // not have to be known in advance.
    static const char *TOPIC_FILTER = "welcomeip/#"; // TODO(Stufe2)

    const char *channelName(uint8_t index, char *buf, size_t bufLen)
    {
        snprintf(buf, bufLen, "ch%04u", (unsigned)index);
        return buf;
    }

    const char *datapointName(uint8_t index, bool output, char *buf, size_t bufLen)
    {
        snprintf(buf, bufLen, "%sdp%04u", output ? "oi" : "i", (unsigned)index);
        return buf;
    }

    // ---------------------------------------------------------------------------
    // Outgoing
    // ---------------------------------------------------------------------------

    bool Protocol::buildGetAll(std::string &topic, std::string &payload)
    {
        topic = TOPIC_REQUEST;
        JsonDocument doc;
        doc["method"] = "getAll"; // TODO(Stufe2): method name confirmed, envelope guessed
        doc["queryid"] = _queryId++;
        payload.clear();
        serializeJson(doc, payload);
        return true;
    }

    bool Protocol::buildSetDatapoint(const Address &a, const char *value, bool secondLock,
                                     std::string &topic, std::string &payload)
    {
        if (!a.serial[0] || a.output) return false; // only input datapoints are writable

        char ch[10], dp[12];
        topic = TOPIC_REQUEST;

        JsonDocument doc;
        doc["method"] = "setDataPoint"; // TODO(Stufe2)
        doc["queryid"] = _queryId++;
        JsonObject d = doc["data"].to<JsonObject>();
        d["serialNumber"] = a.serial;
        d["channel"] = channelName(a.channel, ch, sizeof(ch));
        d["datapoint"] = datapointName(a.dp, false, dp, sizeof(dp));
        d["value"] = value;
        // Documented by name in the ABB sample text: 0 = default lock, 1 = auxiliary.
        d["is_Secondlock"] = secondLock ? 1 : 0;

        payload.clear();
        serializeJson(doc, payload);
        return true;
    }

    bool Protocol::buildHeartbeat(std::string &topic, std::string &payload)
    {
        topic = TOPIC_HEARTBEAT;
        JsonDocument doc;
        doc["method"] = "heartbeat"; // TODO(Stufe2)
        doc["queryid"] = _queryId++;
        payload.clear();
        serializeJson(doc, payload);
        return true;
    }

    const char *Protocol::notificationFilter() const
    {
        return TOPIC_FILTER;
    }

    // ---------------------------------------------------------------------------
    // Incoming
    // ---------------------------------------------------------------------------

    // A datapoint value arrives as a string, a number or -- for list-valued channels --
    // an array. Everything is handed upwards as text; the channels interpret it.
    static void valueToString(JsonVariantConst v, char *buf, size_t bufLen)
    {
        buf[0] = '\0';
        if (v.isNull()) return;
        if (v.is<const char *>())
        {
            const char *s = v.as<const char *>();
            if (s) strncpy(buf, s, bufLen - 1);
            buf[bufLen - 1] = '\0';
            return;
        }
        if (v.is<bool>() || v.is<int>() || v.is<unsigned>())
        {
            snprintf(buf, bufLen, "%ld", (long)v.as<long>());
            return;
        }
        if (v.is<float>())
        {
            snprintf(buf, bufLen, "%.3f", v.as<float>());
            return;
        }
        // Arrays/objects (e.g. an empty list value) stay empty -- nothing to map to KNX.
    }

    // Walks devices[] / devicelist[] of a getAll response. Free function so the header
    // stays free of ArduinoJson; smartApSerial is filled from the devices[] pass.
    static void parseDeviceList(JsonArrayConst devices, const DeviceFn &onDevice,
                                const DatapointFn &onDatapoint,
                                char *smartApSerial, size_t smartApSerialLen)
    {
        char value[32];

        for (JsonObjectConst dev : devices)
        {
            const char *serial = dev["serialNumber"];
            if (!serial || !serial[0]) continue;

            JsonArrayConst channels = dev["channels"];

            if (smartApSerial && !smartApSerial[0])
            {
                strncpy(smartApSerial, serial, smartApSerialLen - 1);
                smartApSerial[smartApSerialLen - 1] = '\0';
            }

            if (onDevice)
            {
                Device d;
                d.serial = serial;
                d.displayName = dev["displayName"] | "";
                d.deviceTypeId = dev["deviceTypeId"] | 0;
                d.channelCount = channels.isNull() ? 0 : (uint8_t)channels.size();
                onDevice(d);
            }

            if (!onDatapoint || channels.isNull()) continue;

            // getAll carries the current value of every output, so one response
            // initialises all channels -- no extra read per datapoint.
            for (JsonObjectConst chn : channels)
            {
                Address a;
                strncpy(a.serial, serial, sizeof(a.serial) - 1);
                a.channel = chn["i"] | 0;
                a.output = true;

                JsonArrayConst outputs = chn["outputs"];
                if (outputs.isNull()) continue;
                for (JsonObjectConst odp : outputs)
                {
                    a.dp = odp["i"] | 0;
                    valueToString(odp["value"], value, sizeof(value));
                    onDatapoint(a, value);
                }
            }
        }
    }

    bool Protocol::parse(const char *topic, const uint8_t *payload, size_t len,
                         const DatapointFn &onDatapoint,
                         const DeviceFn &onDevice,
                         const ResultFn &onResult)
    {
        if (!payload || !len) return false;

        // Keep only what the channels need. Without this filter a getAll response
        // (17-50 KB of JSON) would need several times its own size as a DOM.
        JsonDocument filter;
        filter["method"] = true;
        filter["queryid"] = true;
        filter["result"] = true;
        // Built twice rather than copied: assigning one subtree of a document to
        // another position in the same document is not safe in ArduinoJson.
        for (const char *key : {"devices", "devicelist"})
        {
            JsonObject devFilter = filter["data"][key].add<JsonObject>();
            devFilter["serialNumber"] = true;
            devFilter["deviceTypeId"] = true;
            devFilter["displayName"] = true;
            JsonObject chnFilter = devFilter["channels"].add<JsonObject>();
            chnFilter["i"] = true;
            chnFilter["functionId"] = true;
            JsonObject odpFilter = chnFilter["outputs"].add<JsonObject>();
            odpFilter["i"] = true;
            odpFilter["value"] = true;
        }
        // A notification names one datapoint instead of the whole model.
        filter["data"]["serialNumber"] = true;
        filter["data"]["channel"] = true;
        filter["data"]["datapoint"] = true;
        filter["data"]["value"] = true;

        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, payload, len,
                                                  DeserializationOption::Filter(filter));
        if (err)
        {
            logError("WIP", "JSON parse failed: %s", err.c_str());
            return false;
        }

        const char *method = doc["method"] | "";
        const uint32_t queryId = doc["queryid"] | 0;

        if (onResult && doc["result"].is<int>())
            onResult(queryId, doc["result"].as<int>());

        JsonObjectConst data = doc["data"];
        if (data.isNull()) return true;

        // getAll: the SmartAP itself is in devices[], everything attached to it in
        // devicelist[]. Both carry the same device/channel/datapoint shape.
        JsonArrayConst devices = data["devices"];
        JsonArrayConst devicelist = data["devicelist"];
        if (!devices.isNull())
            parseDeviceList(devices, onDevice, onDatapoint, _smartApSerial, sizeof(_smartApSerial));
        if (!devicelist.isNull())
            parseDeviceList(devicelist, onDevice, onDatapoint, nullptr, 0);
        if (!devices.isNull() || !devicelist.isNull()) return true;

        // Notification: one datapoint. TODO(Stufe2) -- field names follow the getAll
        // model and the sample text, the actual payload is unconfirmed.
        const char *serial = data["serialNumber"];
        const char *chn = data["channel"];
        const char *dpn = data["datapoint"];
        if (serial && chn && dpn && onDatapoint)
        {
            Address a;
            strncpy(a.serial, serial, sizeof(a.serial) - 1);
            a.channel = (uint8_t)strtoul(chn + 2, nullptr, 10);       // skip "ch"
            a.output = (dpn[0] == 'o');
            a.dp = (uint8_t)strtoul(dpn + (a.output ? 4 : 3), nullptr, 10); // skip oidp/idp
            char value[32];
            valueToString(data["value"], value, sizeof(value));
            onDatapoint(a, value);
            return true;
        }

        (void)topic;
        (void)method;
        return true;
    }
} // namespace WelcomeIP
