#include "external.h"
#include "stm32g0xx.h"

#include "global_manager.h"

void Encoder::initEncoder()
{
    /* TURN PINS */
	GPIOB->MODER &= ~(GPIO_MODER_MODE3);
	GPIOB->OTYPER |= GPIO_OTYPER_OT3;

	GPIOA->MODER &= ~(GPIO_MODER_MODE8);
	GPIOA->OTYPER |= GPIO_OTYPER_OT8;

	EXTI->EXTICR[0] |= EXTI_EXTICR1_EXTI3_0;

	EXTI->FTSR1 |= EXTI_FTSR1_FT3;

	NVIC_EnableIRQ(EXTI2_3_IRQn);

	/* TODO: CHANGE PINOUT TO ADD BUTTON INTERRUPT */
	GPIOB->MODER &= ~(GPIO_MODER_MODE0);
	GPIOB->OTYPER |= GPIO_OTYPER_OT0;
}

void MicroSwitches::microswitchesSetup()
{
	/* LEFT MICROSWITCHES */
	GPIOA->MODER &= ~(GPIO_MODER_MODE1);
	GPIOA->OTYPER |= GPIO_OTYPER_OT1;

	EXTI->EXTICR[0] &= ~(EXTI_EXTICR1_EXTI0 | EXTI_EXTICR1_EXTI1);

	EXTI->FTSR1 |= EXTI_FTSR1_FT0 | EXTI_FTSR1_FT1;

	NVIC_EnableIRQ(EXTI0_1_IRQn);

	/* RIGHT MICROSWITCHES */
	GPIOB->MODER &= ~(GPIO_MODER_MODE5);
	GPIOB->OTYPER |= GPIO_OTYPER_OT5;

	EXTI->EXTICR[1] |= EXTI_EXTICR2_EXTI4_0 | EXTI_EXTICR2_EXTI5_0;

	EXTI->FTSR1 |= EXTI_FTSR1_FT4 | EXTI_FTSR1_FT5;

	NVIC_EnableIRQ(EXTI4_15_IRQn);
}

extern "C"
{

/* Encoder pins handler */
void EXTI2_3_IRQHandler()
{
	GPIOA->IDR & GPIO_IDR_ID8 ? Encoder::leftTurnHandler() : Encoder::rightTurnHandler();

	EXTI->FPR1 |= EXTI_FPR1_FPIF3;
}


/* Left MicroSwitches handlers */
void EXTI0_1_IRQHandler()
{
	if (EXTI->FPR1 & EXTI_FPR1_FPIF0){
		MicroSwitches::leftMicroSwitchHandler();	
		
		EXTI->FPR1 |= EXTI_FPR1_FPIF0;
	}
	if (EXTI->FPR1 & EXTI_FPR1_FPIF1){
		MicroSwitches::leftMicroSwitchHandler();	
		
		EXTI->FPR1 |= EXTI_FPR1_FPIF1;
	}
}

/* Right MicroSwitches handlers */
void EXTI4_15_IRQHandler()
{
	if (EXTI->FPR1 & EXTI_FPR1_FPIF4){
		MicroSwitches::rightMicroSwitchHandler();
		
		EXTI->FPR1 |= EXTI_FPR1_FPIF4;
	}
	if (EXTI->FPR1 & EXTI_FPR1_FPIF5){
		MicroSwitches::rightMicroSwitchHandler();
		
		EXTI->FPR1 |= EXTI_FPR1_FPIF5;
	}
}

}