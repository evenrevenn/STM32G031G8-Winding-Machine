#include "external.h"
#include "stm32f1xx.h"

#include "global_manager.h"

void Encoder::initEncoder()
{
    /* TURN PINS */
	GPIOB->CRL &= ~(GPIO_CRL_CNF3_1);
	GPIOB->CRL |= GPIO_CRL_CNF3_0;
	GPIOB->CRL &= ~(GPIO_CRL_MODE3);

    GPIOA->CRH &= ~(GPIO_CRH_CNF8_1);
	GPIOA->CRH |= GPIO_CRH_CNF8_0;
	GPIOA->CRH &= ~(GPIO_CRH_MODE8);

	AFIO->EXTICR[0] |= AFIO_EXTICR1_EXTI3_PB;

	EXTI->IMR |=  EXTI_IMR_MR3;
	EXTI->FTSR |= EXTI_FTSR_FT3;

	NVIC_EnableIRQ(EXTI3_IRQn);

	/* TODO: CHANGE PINOUT TO ADD BUTTON INTERRUPT */
	GPIOB->CRL &= ~(GPIO_CRL_CNF0_1);
	GPIOB->CRL |= GPIO_CRL_CNF0_0;
	GPIOB->CRL &= ~(GPIO_CRL_MODE0);
}

void MicroSwitches::microswitchesSetup()
{
	/* LEFT MICROSWITCHES */
	GPIOA->CRL &= ~(GPIO_CRL_CNF0_1 | GPIO_CRL_CNF1_1);
	GPIOA->CRL |= GPIO_CRL_CNF0_0 | GPIO_CRL_CNF1_0;
	GPIOA->CRL &= ~(GPIO_CRL_MODE0 | GPIO_CRL_MODE1);

	AFIO->EXTICR[0] |= AFIO_EXTICR1_EXTI0_PA | AFIO_EXTICR1_EXTI1_PA;

	EXTI->IMR |=  EXTI_IMR_MR0 | EXTI_IMR_MR1;
	EXTI->FTSR |= EXTI_FTSR_FT0 | EXTI_FTSR_FT1;

	NVIC_EnableIRQ(EXTI0_IRQn);
	NVIC_EnableIRQ(EXTI1_IRQn);

	/* RIGHT MICROSWITCHES */
	GPIOB->CRL &= ~(GPIO_CRL_CNF4_1 | GPIO_CRL_CNF5_1);
	GPIOB->CRL |= GPIO_CRL_CNF4_0 | GPIO_CRL_CNF5_0;
	GPIOB->CRL &= ~(GPIO_CRL_MODE4 | GPIO_CRL_MODE5);

	AFIO->EXTICR[1] |= AFIO_EXTICR2_EXTI4_PB | AFIO_EXTICR2_EXTI5_PB;

	EXTI->IMR |=  EXTI_IMR_MR4 | EXTI_IMR_MR5;
	EXTI->FTSR |= EXTI_FTSR_FT4 | EXTI_FTSR_FT5;

	NVIC_EnableIRQ(EXTI4_IRQn);
	NVIC_EnableIRQ(EXTI9_5_IRQn);
}

extern "C"
{

/* Encoder pins handler */
void EXTI3_IRQHandler()
{
	GPIOA->IDR & GPIO_IDR_IDR8 ? Encoder::leftTurnHandler() : Encoder::rightTurnHandler();

	EXTI->PR |= EXTI_PR_PR3;
}


/* Left MicroSwitches handlers */
void EXTI0_IRQHandler()
{
	MicroSwitches::leftMicroSwitchHandler();	

	EXTI->PR |= EXTI_PR_PR0;
}

void EXTI1_IRQHandler()
{
	MicroSwitches::leftMicroSwitchHandler();

	EXTI->PR |= EXTI_PR_PR1;
}


/* Right MicroSwitches handlers */
void EXTI4_IRQHandler()
{
	MicroSwitches::rightMicroSwitchHandler();	

	EXTI->PR |= EXTI_PR_PR4;
}

void EXTI9_5_IRQHandler()
{
	MicroSwitches::rightMicroSwitchHandler();

	EXTI->PR |= EXTI_PR_PR5;
}

}