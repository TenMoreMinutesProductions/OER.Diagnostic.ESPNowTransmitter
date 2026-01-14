// ============================================================
//            ESP-NOW DIAGNOSTIC TRANSMITTER
// ============================================================
//
// Sends ping messages via ESP-NOW unicast with automatic retries.
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

#define PING_INTERVAL_MS 100       // Send ping every 100ms (10/sec)
#define PING_HEARTBEAT_MS 60000    // Stats output every 60 seconds
#define PING_TEST_COUNT 10000      // Stop after this many pings

// ============================================================
//                    FUNCTIONS
// ============================================================

// Initialize the diagnostic ping system
void diagnosticPingInit();

// Call from loop - sends ping at configured interval
void diagnosticPingLoop();

// Called by ESP-NOW send callback to track success/failure
void diagnosticPingOnSendResult(bool success);

// Get statistics
uint32_t diagnosticPingGetSequence();
uint32_t diagnosticPingGetSendCount();
uint32_t diagnosticPingGetSuccessCount();
uint32_t diagnosticPingGetFailCount();

#endif
