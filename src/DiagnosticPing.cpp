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

// ============================================================
//                    PUBLIC FUNCTIONS
// ============================================================

void diagnosticPingInit() {
    _sequenceNumber = 0;
    _lastPingTime = 0;

    Serial.println();
    Serial.println("╔════════════════════════════════════════════════════════╗");
    Serial.println("║         ESP-NOW DIAGNOSTIC TRANSMITTER                 ║");
    Serial.println("╠════════════════════════════════════════════════════════╣");
    Serial.println("║  Ping interval: 1 second                               ║");
    Serial.println("║  Broadcasting to all ESP-NOW receivers                 ║");
    Serial.println("╚════════════════════════════════════════════════════════╝");
    Serial.println();
}

void diagnosticPingLoop() {
    unsigned long now = millis();

    // Check if it's time to send a ping
    if (now - _lastPingTime >= PING_INTERVAL_MS) {
        _lastPingTime = now;
        _sequenceNumber++;

        // Build ping message
        PingMessage ping;
        ping.magic = PING_MAGIC;
        ping.sequenceNumber = _sequenceNumber;
        ping.uptimeMs = now;

        // Broadcast the ping
        bool success = espnowBroadcast((const uint8_t*)&ping, sizeof(ping));

        // Format uptime for display
        char uptimeStr[16];
        formatUptime(now, uptimeStr, sizeof(uptimeStr));

        // Log the send
        Serial.printf("[%s] PING #%lu %s\n",
                      uptimeStr,
                      _sequenceNumber,
                      success ? "sent" : "FAILED");
    }
}

uint32_t diagnosticPingGetSequence() {
    return _sequenceNumber;
}
