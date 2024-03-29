#include "steppers.h"
#include "UART_handler.h"

/*-------------------------------------------------------*/
/* STEPPER BASE */

Stepper::Stepper(uint32_t &steps, uint32_t &half_period, MANAGER_ACCESS call_access):
manager_access_(call_access),
steps_(steps),
half_period_(half_period)
{
}

/*-------------------------------------------------------*/
/* STEPPER DRUM */

DrumStepper::DrumStepper(uint32_t &steps, uint32_t &half_period, MANAGER_ACCESS call_access):
Stepper(steps, half_period, call_access)
{
    RCC->IOPENR |= RCC_IOPENR_GPIOAEN;

    GPIOA->MODER &= ~(GPIO_MODER_MODE3_1 | GPIO_MODER_MODE4_1 | GPIO_MODER_MODE5_1);
    GPIOA->MODER |= GPIO_MODER_MODE3_0 | GPIO_MODER_MODE4_0 | GPIO_MODER_MODE5_0;

    changeDirection(DIR_FORWARD);
    switchEnable(false);
}

TickType_t DrumStepper::calcHalfPeriod(const uint32_t frequency_Hz)
{
    /* 10000 (Hz) / x (Hz) / 2 */
    return configTICK_RATE_HZ / frequency_Hz / 2;
}

void DrumStepper::changeDirection(Direction direction)
{
    direction == DIR_FORWARD ? GPIOA->ODR |= GPIO_ODR_OD3 : GPIOA->ODR &= ~(GPIO_ODR_OD3);
}

void DrumStepper::switchEnable(bool enabled)
{
    enabled ? GPIOA->ODR &= ~(GPIO_ODR_OD5) : GPIOA->ODR |= GPIO_ODR_OD5;
}

void DrumStepper::vStepperDrumTask(void *pvParameters)
{
    task_params::PARAMS_stepper_t stepper_params = *static_cast<task_params::PARAMS_stepper_t *>(pvParameters);
    DrumStepper drum_stepper(stepper_params.steps, stepper_params.frequency, stepper_params.call_access);

    // print("Drum task started\n");

    while(true)
    {
        drum_stepper.callbackDrum();
    }
}

void DrumStepper::callbackDrum()
{
    GPIOA->ODR ^= GPIO_ODR_OD4;
    vTaskDelay(half_period_);
}

/*-------------------------------------------------------*/
/* STEPPER GUIDE */

GuideStepper::GuideStepper(uint32_t &steps, uint32_t &half_period, MANAGER_ACCESS call_access):
Stepper(steps, half_period, call_access)
{
    RCC->IOPENR |= RCC_IOPENR_GPIOBEN;

    GPIOA->MODER &= ~(GPIO_MODER_MODE6_1 | GPIO_MODER_MODE7_1);
    GPIOA->MODER |= GPIO_MODER_MODE6_0 | GPIO_MODER_MODE7_0;

    GPIOB->MODER &= ~(GPIO_MODER_MODE0_1);
    GPIOB->MODER |= GPIO_MODER_MODE0_0;

    changeDirection(DIR_FORWARD);
    switchEnable(false);
}

TickType_t GuideStepper::calcHalfPeriod(const TickType_t drum_half_period, const uint16_t thickness)
{
    /* f (ticks) * 200 (mm^-2) / t (mm^-2) */
    return drum_half_period * 200 / thickness;
}

void GuideStepper::changeDirection(Direction direction)
{
    direction == DIR_FORWARD ? GPIOA->ODR |= GPIO_ODR_OD6 : GPIOA->ODR &= ~(GPIO_ODR_OD6);
}

void GuideStepper::switchEnable(bool enabled)
{
    enabled ? GPIOB->ODR &= ~(GPIO_ODR_OD0) : GPIOB->ODR |= GPIO_ODR_OD0;
}

void GuideStepper::vStepperGuideTask(void *pvParameters)
{
    task_params::PARAMS_stepper_t stepper_params = *static_cast<task_params::PARAMS_stepper_t *>(pvParameters);
    GuideStepper guide_stepper(stepper_params.steps, stepper_params.frequency, stepper_params.call_access);

    // print("Guide task started\n");

    while(true)
    {
        guide_stepper.callbackGuide();
    }
}

void GuideStepper::callbackGuide()
{
    if (steps_ == 0){
        MANAGER_CALL done_call{MANAGER_CALL_IDS::GUIDE_DONE_ID, 0};
        manager_access_.sendCall(done_call);

        return;
    }

    GPIOA->ODR |= GPIO_ODR_OD7;
    vTaskDelay(half_period_);
    GPIOA->ODR &= ~(GPIO_ODR_OD7);
    steps_--;
    vTaskDelay(half_period_);
}
