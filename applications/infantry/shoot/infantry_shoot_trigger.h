#ifndef INFANTRY_SHOOT_TRIGGER_H
#define INFANTRY_SHOOT_TRIGGER_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t consumed;
    uint32_t activation_count;
} InfantrySingleShotTrigger;

void InfantrySingleShotTrigger_Init(InfantrySingleShotTrigger *trigger);

/**
 * Consume one asserted trigger level while single-fire mode is active.
 * Releasing the trigger or leaving single-fire mode rearms the next shot.
 */
bool InfantrySingleShotTrigger_Update(InfantrySingleShotTrigger *trigger,
                                     bool single_mode,
                                     bool trigger_down);

#ifdef __cplusplus
}
#endif

#endif /* INFANTRY_SHOOT_TRIGGER_H */
