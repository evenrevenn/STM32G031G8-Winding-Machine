#ifndef STEPPER_H
#define STEPPER_H

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"

#include "global_manager.h"

class Stepper
{
public:
    enum Direction{DIR_FORWARD, DIR_BACKWARD};
    
    Stepper(uint32_t &steps, uint32_t &period, MANAGER_ACCESS call_access);

protected:
    MANAGER_ACCESS manager_access_;
    uint32_t &steps_;
    uint32_t &half_period_;
};

class DrumStepper : public Stepper
{
public:
    DrumStepper(uint32_t &steps, uint32_t &period, MANAGER_ACCESS call_access);
    static TickType_t calcHalfPeriod(const uint32_t frequency_Hz);
    static void changeDirection(Direction direction);
    static void switchEnable(bool enabled);

    static void vStepperDrumTask(void *pvParameters);

    void callbackDrum();
};

class GuideStepper : public Stepper
{
public:
    GuideStepper(uint32_t &steps, uint32_t &period, MANAGER_ACCESS call_access);
    static TickType_t calcHalfPeriod(const TickType_t drum_frequency, const uint16_t thickness);
    static void changeDirection(Direction direction);
    static void switchEnable(bool enabled);

    static void vStepperGuideTask(void *pvParameters);

    void callbackGuide();
};

#endif //STEPPER_H  