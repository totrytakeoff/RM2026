#include "infantry_firmware.h"

#include "bsp_dwt.h"
#include "bsp_can.h"
#include "bsp_log.h"
#include "bsp_usart.h"
#include "daemon.h"
#include "infantry_app.h"
#include "infantry_config.h"
#include "infantry_tasks.h"

bool InfantryFirmware_Init(void)
{
    DWT_Init(INFANTRY_CPU_FREQUENCY_MHZ);
    DaemonServiceInit();

    if (!CANConfigureDispatch(CAN_DISPATCH_DEFERRED) ||
        !USARTConfigureDispatch(USART_DISPATCH_DEFERRED)) {
        return false;
    }

#if MINIMAL_DEBUG_ENABLE && ((MINIMAL_DEBUG_MODE & MINIMAL_DEBUG_MODE_TEXT) != 0)
    BSPLogInit();
#endif

    if (!InfantryApp_Init()) {
        InfantryApp_ForceSafeStop();
        return false;
    }
    if (!InfantryTasks_Create()) {
        InfantryApp_ForceSafeStop();
        return false;
    }

    return true;
}
