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
static unsigned long _testStartTime = 0;

// Statistics
static uint32_t _sendCount = 0;
static uint32_t _successCount = 0;
static uint32_t _failCount = 0;

// Test state
static bool _testComplete = false;
static bool _summaryPrinted = false;

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

static void printFinalSummary() {
    unsigned long duration = millis() - _testStartTime;
    char durationStr[16];
    formatUptime(duration, durationStr, sizeof(durationStr));

    float successRate = 0;
    if (_sendCount > 0) {
        successRate = (_successCount * 100.0f) / _sendCount;
    }

    Serial.println();
    Serial.println("╔════════════════════════════════════════════════════════╗");
    Serial.println("║           TRANSMITTER TEST COMPLETE                    ║");
    Serial.println("╠════════════════════════════════════════════════════════╣");
    Serial.printf("║  Test duration:      %s                         ║\n", durationStr);
    Serial.printf("║  Packets sent:       %-10lu                       ║\n", _sendCount);
    Serial.printf("║  ACKs received:      %-10lu                       ║\n", _successCount);
    Serial.printf("║  Failed (no ACK):    %-10lu                       ║\n", _failCount);
    Serial.printf("║  Success rate:       %6.2f%%                          ║\n", successRate);
    Serial.println("╚════════════════════════════════════════════════════════╝");
    Serial.println();
    Serial.println("Test finished. Reset device to run again.");
}

// ============================================================
//                    PUBLIC FUNCTIONS
// ============================================================

void diagnosticPingInit() {
    _sequenceNumber = 0;
    _lastPingTime = 0;
    _lastHeartbeatTime = millis();
    _testStartTime = millis();
    _sendCount = 0;
    _successCount = 0;
    _failCount = 0;
    _testComplete = false;
    _summaryPrinted = false;

    char macStr[18];
    formatMac(_receiverMac, macStr, sizeof(macStr));

    float testDuration = (PING_TEST_COUNT * PING_INTERVAL_MS) / 1000.0f / 60.0f;

    Serial.println();
    Serial.println("╔════════════════════════════════════════════════════════╗");
    Serial.println("║         ESP-NOW DIAGNOSTIC TRANSMITTER                 ║");
    Serial.println("╠════════════════════════════════════════════════════════╣");
    Serial.printf("║  Test: Send %d packets at %dms intervals          ║\n", PING_TEST_COUNT, PING_INTERVAL_MS);
    Serial.printf("║  Estimated duration: %.1f minutes                      ║\n", testDuration);
    Serial.println("║  Mode: Unicast with auto-retry (up to 31 retries)      ║");
    Serial.println("╠════════════════════════════════════════════════════════╣");
    Serial.printf("║  Target receiver: %s                 ║\n", macStr);
    Serial.println("╚════════════════════════════════════════════════════════╝");
    Serial.println();
    Serial.println("Starting test...");
    Serial.println();

    // Add receiver as peer for unicast
    if (!espnowAddPeer(_receiverMac)) {
        Serial.println("[WARN] Failed to add receiver peer - check MAC address");
    }
}

void diagnosticPingLoop() {
    // Don't do anything if test is complete
    if (_testComplete) {
        if (!_summaryPrinted) {
            // Wait a moment for final callbacks to complete
            delay(500);
            printFinalSummary();
            _summaryPrinted = true;
        }
        return;
    }

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

        // Check if test is complete
        if (_sequenceNumber >= PING_TEST_COUNT) {
            _testComplete = true;
        }
    }

    // 60-second progress update
    if (!_testComplete && (now - _lastHeartbeatTime >= PING_HEARTBEAT_MS)) {
        _lastHeartbeatTime = now;

        char uptimeStr[16];
        formatUptime(now - _testStartTime, uptimeStr, sizeof(uptimeStr));

        float progress = (_sendCount * 100.0f) / PING_TEST_COUNT;
        float successRate = (_sendCount > 0) ? (_successCount * 100.0f) / _sendCount : 0;

        Serial.println();
        Serial.printf("[%s] Progress: %lu/%d (%.1f%%) | ACK'd: %lu | Failed: %lu | Success: %.1f%%\n",
                      uptimeStr, _sendCount, PING_TEST_COUNT, progress,
                      _successCount, _failCount, successRate);
        Serial.println();
    }
}

void diagnosticPingOnSendResult(bool success) {
    if (success) {
        _successCount++;
    } else {
        _failCount++;
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
