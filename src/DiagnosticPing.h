// ============================================================
//            ESP-NOW DIAGNOSTIC TRANSMITTER
// ============================================================
//
// Broadcasts a ping message every second via ESP-NOW.
// Used with OER.Diagnostic.ESPNowReceiver to test signal quality.
//
// ============================================================

#ifndef DIAGNOSTICPING_H
#define DIAGNOSTICPING_H

#include <Arduino.h>

// ============================================================
//                   PING MESSAGE STRUCTURE
// ============================================================
// This structure is shared between transmitter and receiver.
// Keep it packed to ensure consistent byte layout.

#pragma pack(push, 1)
struct PingMessage {
    uint8_t magic;           // 0xAA to identify our messages
    uint32_t sequenceNumber; // Incrementing sequence for gap detection
    uint32_t uptimeMs;       // Transmitter uptime in milliseconds
};
#pragma pack(pop)

#define PING_MAGIC 0xAA

// ============================================================
//                    CONFIGURATION
// ============================================================

#define PING_INTERVAL_MS 1000  // Send ping every 1 second

// ============================================================
//                    FUNCTIONS
// ============================================================

// Initialize the diagnostic ping system
void diagnosticPingInit();

// Call from loop - sends ping at configured interval
void diagnosticPingLoop();

// Get current sequence number
uint32_t diagnosticPingGetSequence();

#endif
