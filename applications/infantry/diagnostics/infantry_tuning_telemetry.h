#ifndef INFANTRY_TUNING_TELEMETRY_H
#define INFANTRY_TUNING_TELEMETRY_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Initialize the dedicated tuning UART without enabling generic logs. */
bool InfantryTuningTelemetry_Init(void);

/** Build and start one non-blocking VOFA+ JustFloat DMA frame. */
void InfantryTuningTelemetry_Publish(uint32_t now_ms);

uint32_t InfantryTuningTelemetry_GetSentCount(void);
uint32_t InfantryTuningTelemetry_GetDroppedCount(void);

#ifdef __cplusplus
}
#endif

#endif /* INFANTRY_TUNING_TELEMETRY_H */
