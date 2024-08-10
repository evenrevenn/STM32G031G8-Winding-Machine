#ifndef EXTERNALS_H
#define EXTERNALS_H

#include "global_manager.h"
#include "steppers.h"

namespace Encoder
{
    void initEncoder();
    void leftTurnHandler();
    void rightTurnHandler();
} //namespace Encoder

namespace MicroSwitches
{
    void leftMicroSwitchHandler(); 
    void rightMicroSwitchHandler();
    void microswitchesSetup();

    bool readLeftMicroSwitches();
    bool readRightMicroSwitches();
} //namespace MicroSwitches

inline void Encoder::leftTurnHandler()
{
    MANAGER_ACCESS access = GlobalManager::getInstance().getManagerAccess();
    MANAGER_CALL call;

    if (GPIOB->IDR & GPIO_IDR_ID1)
    {
        call.id = MANAGER_CALL_IDS::SPEED_INCREASE_ID;
        call.value = 1;
    }
    else{
        call.id = MANAGER_CALL_IDS::DIRECTION_GUIDE_CHANGE_ID;
        call.value = Stepper::DIR_FORWARD;
    }

    access.sendCallFromISR(call);
}

inline void Encoder::rightTurnHandler()
{
    MANAGER_ACCESS access = GlobalManager::getInstance().getManagerAccess();
    MANAGER_CALL call;

    if (GPIOB->IDR & GPIO_IDR_ID1)
    {
        call.id = MANAGER_CALL_IDS::SPEED_INCREASE_ID;
        call.value = -1;
    }
    else{
        call.id = MANAGER_CALL_IDS::DIRECTION_GUIDE_CHANGE_ID;
        call.value = Stepper::DIR_BACKWARD;
    }

    access.sendCallFromISR(call);
}


inline void MicroSwitches::leftMicroSwitchHandler()
{
    MANAGER_ACCESS access = GlobalManager::getInstance().getManagerAccess();
    MANAGER_CALL call;

    while (xQueueReceiveFromISR(access.calls_queue, &call, NULL));

    call.id = MANAGER_CALL_IDS::WAVE_STOP_ID;
    access.sendCallFromISR(call);
}

inline void MicroSwitches::rightMicroSwitchHandler()
{
    MANAGER_ACCESS access = GlobalManager::getInstance().getManagerAccess();
    MANAGER_CALL call;

    while (xQueueReceiveFromISR(access.calls_queue, &call, NULL));

    call.id = MANAGER_CALL_IDS::WAVE_STOP_ID;
    access.sendCallFromISR(call);
}

inline bool MicroSwitches::readLeftMicroSwitches() 
{
    return (GPIOA->IDR & (GPIO_IDR_ID0 | GPIO_IDR_ID1)) == (GPIO_IDR_ID0 | GPIO_IDR_ID1);
}

inline bool MicroSwitches::readRightMicroSwitches() 
{
    return (GPIOB->IDR & (GPIO_IDR_ID4 | GPIO_IDR_ID5)) == (GPIO_IDR_ID4 | GPIO_IDR_ID5);
}

#endif //EXTERNALS_H