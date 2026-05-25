#include "pd42s1_debug.h"

#include "pd42s1.h"

#define PD42S1_DEBUG_ACCEL               50U
#define PD42S1_DEBUG_RPM                 60U
#define PD42S1_DEBUG_REFRESH_MS          500U

void PD42S1_Debug_Init(void)
{
    PD42S1_Init();
    
    PD42S1_EnableMotor();

}

void PD42S1_Debug_Update(void)
{
    (void)PD42S1_SetSpeed(PD42S1_DIRECTION_CW, PD42S1_DEBUG_ACCEL, PD42S1_DEBUG_RPM);
}

void PD42S1_Debug_DeInit(void)
{
    (void)PD42S1_SetSpeed(PD42S1_DIRECTION_CW, PD42S1_DEBUG_ACCEL, 0);
}