// ============================================================
//            ESP-NOW DIAGNOSTIC TRANSMITTER
// ============================================================

#include "DiagnosticPing.h"
#include "config.h"
#include "modules/espnow_module.h"

// ============================================================
//                    STATE
// ============================================================

static uint32_t _sequenceNumber = 0;
static unsigned long _lastPingTime = 0;
static unsigned long _lastHeartbeatTime = 0;

// Statistics
static uint32_t _sendCount = 0;
static uint32_t _successCount = 0;
static uint32_t _failCount = 0;

// Target receiver MAC address (set in config.h)
static uint8_t _receiverMac[] = ESPNOW_RECEIVER_MAC;

// ============================================================
//                    HELPER FUNCTIONS
// ============================================================

static void formatUptime(unsigned long ms, char* buffer, size_t bufferSize) {
    unsigned long totalSecs = ms / 1000;
    unsigned long hours = totalSecs / 3600;
    unsigned long mins = (totalSecs % 3600) / 60;
    unsigned long secs = totalSecs % 60;
    snprintf(buffer, bufferSize, "%02lu:%02lu:%02lu", hours, mins, secs);
}

static void formatMac(const uint8_t* mac, char* buffer, size_t bufferSize) {
    snprintf(buffer, bufferSize, "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

// ============================================================
//                    PUBLIC FUNCTIONS
// ============================================================

void diagnosticPingInit() {
    _sequenceNumber = 0;
    _lastPingTime = 0;
    _lastHeartbeatTime = millis();
    _sendCount = 0;
    _successCount = 0;
    _failCount = 0;

    char macStr[18];
    formatMac(_receiverMac, macStr, sizeof(macStr));

    Serial.println();
    Serial.println("╔════════════════════════════════════════════════════════╗");
    Serial.println("║         ESP-NOW DIAGNOSTIC TRANSMITTER                 ║");
    Serial.println("╠════════════════════════════════════════════════════════╣");
    Serial.println("║  Mode: Unicast with auto-retry (up to 31 retries)      ║");
    Serial.println("║  Ping interval: 100ms (10 pings/sec)                   ║");
    Serial.println("║  Stats interval: 60 seconds                            ║");
    Serial.println("╠════════════════════════════════════════════════════════╣");
    Serial.printf("║  Target receiver: %s                 ║\n", macStr);
    Serial.println("╚════════════════════════════════════════════════════════╝");
    Serial.println();

    // Add receiver as peer for unicast
    if (!espnowAddPeer(_receiverMac)) {
        Serial.println("[WARN] Failed to add receiver peer - check MAC address");
    }
}

void diagnosticPingLoop() {
    unsigned long now = millis();

    // Send ping at configured interval
    if (now - _lastPingTime >= PING_INTERVAL_MS) {
        _lastPingTime = now;
        _sequenceNumber++;
        _sendCount++;

        // Build ping message
        PingMessage ping;
        ping.magic = PING_MAGIC;
        ping.sequenceNumber = _sequenceNumber;
        ping.uptimeMs = now;

        // Send unicast to receiver (enables auto-retry)
        espnowSend(_receiverMac, (const uint8_t*)&ping, sizeof(ping));
        // Result tracked via callback -> diagnosticPingOnSendResult()
    }

    // 60-second heartbeat status
    if (now - _lastHeartbeatTime >= PING_HEARTBEAT_MS) {
        _lastHeartbeatTime = now;

        char uptimeStr[16];
        formatUptime(now, uptimeStr, sizeof(uptimeStr));

        float successRate = 0;
        if (_sendCount > 0) {
            successRate = (_successCount * 100.0f) / _sendCount;
        }

        Serial.println();
        Serial.printf("[%s] === TRANSMITTER HEARTBEAT ===\n", uptimeStr);
        Serial.printf("             Sent: %lu | ACK'd: %lu | Failed: %lu | Success: %.1f%%\n",
                      _sendCount, _successCount, _failCount, successRate);
        Serial.println();
    }
}

void diagnosticPingOnSendResult(bool success) {
    if (success) {
        _successCount++;
    } else {
        _failCount++;
        // Log failures immediately since they indicate real problems
        char uptimeStr[16];
        formatUptime(millis(), uptimeStr, sizeof(uptimeStr));
        Serial.printf("[%s] SEND FAILED seq=%lu (after all retries)\n",
                      uptimeStr, _sequenceNumber);
    }
}

uint32_t diagnosticPingGetSequence() {
    return _sequenceNumber;
}

uint32_t diagnosticPingGetSendCount() {
    return _sendCount;
}

uint32_t diagnosticPingGetSuccessCount() {
    return _successCount;
}

uint32_t diagnosticPingGetFailCount() {
    return _failCount;
}
