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

/* Fill one self-contained field set and return its actual channel count. */
uint16_t InfantryTuningTelemetry_GetYawChannels(float channels[]);
uint16_t InfantryTuningTelemetry_GetPitchChannels(float channels[]);
uint16_t InfantryTuningTelemetry_GetLoaderChannels(float channels[]);
uint16_t InfantryTuningTelemetry_GetFrictionLeftChannels(float channels[]);
uint16_t InfantryTuningTelemetry_GetFrictionRightChannels(float channels[]);
uint16_t InfantryTuningTelemetry_GetShootStateChannels(float channels[]);

uint32_t InfantryTuningTelemetry_GetSentCount(void);
uint32_t InfantryTuningTelemetry_GetDroppedCount(void);

#ifdef __cplusplus
}
#endif

#endif /* INFANTRY_TUNING_TELEMETRY_H */
