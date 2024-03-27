#include "steppers.h"

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
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;

    GPIOA->CRL &= ~(GPIO_CRL_CNF3 | GPIO_CRL_CNF4 | GPIO_CRL_CNF5);

    GPIOA->CRL &= ~(GPIO_CRL_MODE3_0 | GPIO_CRL_MODE4_0 | GPIO_CRL_MODE5_0);
    GPIOA->CRL |= GPIO_CRL_MODE3_1 | GPIO_CRL_MODE4_1 | GPIO_CRL_MODE5_1;

    changeDirection(DIR_FORWARD);
    switchEnable(false);
}

TickType_t DrumStepper::calcHalfPeriod(const uint32_t frequency_Hz)
{
    /* 4000 (Hz) / x (Hz) / 2 */
    return configTICK_RATE_HZ / frequency_Hz / 2;
}

void DrumStepper::changeDirection(Direction direction)
{
    direction == DIR_FORWARD ? GPIOA->ODR |= GPIO_ODR_ODR3 : GPIOA->ODR &= ~(GPIO_ODR_ODR3);
}

void DrumStepper::switchEnable(bool enabled)
{
    enabled ? GPIOA->ODR &= ~(GPIO_ODR_ODR5) : GPIOA->ODR |= GPIO_ODR_ODR5;
}

void DrumStepper::vStepperDrumTask(void *pvParameters)
{
    task_params::PARAMS_stepper_t stepper_params = *static_cast<task_params::PARAMS_stepper_t *>(pvParameters);
    GuideStepper guide_stepper(stepper_params.steps, stepper_params.frequency, stepper_params.call_access);

    while(true)
    {
        guide_stepper.callbackGuide();
    }
}

void DrumStepper::callbackDrum()
{
    GPIOA->ODR ^= GPIO_ODR_ODR4;
    vTaskDelay(pdMS_TO_TICKS(half_period_));
}

/*-------------------------------------------------------*/
/* STEPPER GUIDE */

GuideStepper::GuideStepper(uint32_t &steps, uint32_t &half_period, MANAGER_ACCESS call_access):
Stepper(steps, half_period, call_access)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;

    GPIOA->CRL &= ~(GPIO_CRL_CNF6 | GPIO_CRL_CNF7);

    GPIOA->CRL &= ~(GPIO_CRL_MODE6_0 | GPIO_CRL_MODE7_0);
    GPIOA->CRL |= GPIO_CRL_MODE6_1 | GPIO_CRL_MODE7_1;

    GPIOB->CRL &= ~(GPIO_CRL_MODE0_0);
    GPIOB->CRL |= GPIO_CRL_MODE0_1;

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
    direction == DIR_FORWARD ? GPIOA->ODR |= GPIO_ODR_ODR6 : GPIOA->ODR &= ~(GPIO_ODR_ODR6);
}

void GuideStepper::switchEnable(bool enabled)
{
    enabled ? GPIOB->ODR &= ~(GPIO_ODR_ODR0) : GPIOB->ODR |= GPIO_ODR_ODR0;
}

void GuideStepper::vStepperGuideTask(void *pvParameters)
{
    task_params::PARAMS_stepper_t stepper_params = *static_cast<task_params::PARAMS_stepper_t *>(pvParameters);
    GuideStepper guide_stepper(stepper_params.steps, stepper_params.frequency, stepper_params.call_access);

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

    GPIOA->ODR |= GPIO_ODR_ODR4;
    vTaskDelay(pdMS_TO_TICKS(half_period_));
    GPIOA->ODR &= ~(GPIO_ODR_ODR4);
    steps_--;
    vTaskDelay(pdMS_TO_TICKS(half_period_));
}
