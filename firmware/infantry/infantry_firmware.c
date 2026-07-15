#include "infantry_firmware.h"

#include "bsp_dwt.h"
#include "bsp_log.h"
#include "infantry_app.h"
#include "infantry_config.h"
#include "infantry_tasks.h"

bool InfantryFirmware_Init(void)
{
    DWT_Init(168U);

#if MINIMAL_DEBUG_ENABLE && ((MINIMAL_DEBUG_MODE & MINIMAL_DEBUG_MODE_TEXT) != 0)
    BSPLogInit();
#endif

    InfantryApp_Init();
    if (!InfantryTasks_Create()) {
        InfantryApp_ForceSafeStop();
        return false;
    }

    return true;
}
