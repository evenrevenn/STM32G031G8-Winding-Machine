#include "UART_handler.h"
#include <cstdarg>
#include <cstdio>
#include <cstring>

#include "global_manager.h"

UARTHandler::UARTHandler():
buffer_(256),
vsprintf_buffer_(""),
msg_buffered_(false)
{
	initUSART();
}

void UARTHandler::initUSART()
{
	RCC->IOPENR |= RCC_IOPENR_GPIOBEN;
	RCC->APBENR2 |= RCC_APBENR2_USART1EN;

	// Configuring UART1 Pins
	GPIOB->MODER &= ~(GPIO_MODER_MODE6_0);
	GPIOB->MODER |= GPIO_MODER_MODE6_1;

	GPIOB->OTYPER |= GPIO_OTYPER_OT6;
	GPIOB->AFR[0] &= ~(GPIO_AFRL_AFSEL6);


	GPIOB->MODER &= ~(GPIO_MODER_MODE7_0);
	GPIOB->MODER |= GPIO_MODER_MODE7_1;

	GPIOB->AFR[0] &= ~(GPIO_AFRL_AFSEL7);

	// Configuring UART
	USART1->CR1 &= ~(USART_CR1_M);
	USART1->BRR = 555UL;

	USART1->CR1 |= USART_CR1_RXNEIE_RXFNEIE;

	USART1->CR1 |= USART_CR1_UE;
	USART1->CR1 |= USART_CR1_TE;
	USART1->CR1 |= USART_CR1_RE;


	NVIC_EnableIRQ(USART1_IRQn);
}

UARTHandler &UARTHandler::getInstance()
{
    static UARTHandler uart_handler;
	// static bool is_first = true;

	// if (is_first){
		// uart_handler.initUSART();
	// 	is_first = false;
	// }

    return uart_handler;
}

UARTHandler::~UARTHandler()
{
    RCC->APBENR2 &= ~(RCC_APBENR2_USART1EN);
    USART1->CR1 &= ~(USART_CR1_UE);
	USART1->CR1 &= ~(USART_CR1_TE);
	USART1->CR1 &= ~(USART_CR1_RE);
}

void UARTHandler::printLog(const char *format, ...)
{
	taskENTER_CRITICAL();

	std::va_list args;
	va_start(args, format);

	std::vsnprintf(vsprintf_buffer_, printBufferSize, format, args);

	va_end(args);

	buffer_.writeData((const uint8_t *)vsprintf_buffer_, strlen(vsprintf_buffer_));
	memset(vsprintf_buffer_, 0, sizeof(vsprintf_buffer_));
	
	taskEXIT_CRITICAL();
}

void UARTHandler::writeToQueue(const QueueHandle_t &queue)
{
	static uint8_t item = 0;
	if (buffer_.readData(&item, 1)){
		xQueueSendToBack(queue, &item, pdMS_TO_TICKS(10));
		msg_buffered_ = true;
	}
	else{
		if (msg_buffered_){
			msg_buffered_ = false;
			USART1->CR1 |= USART_CR1_TXEIE_TXFNFIE;
		}
		else{
			vTaskDelay(pdMS_TO_TICKS(10));
		}
	}
}

void UARTHandler::readFromQueue(const QueueHandle_t &queue)
{
	static uint8_t item = 0;
	if (xQueueReceive(queue, &item, pdMS_TO_TICKS(10)) == pdPASS){
		buffer_.writeData(&item, 1);
	}
}

void UARTHandler::vUARTTask(void *pvParameters)
{
	task_params::PARAMS_uart_t *uart_params = static_cast<task_params::PARAMS_uart_t *>(pvParameters);
    UARTHandler &uart_handler = UARTHandler::getInstance();

    while(true)
    {
        uart_handler.writeToQueue(uart_params->transmit_queue);
    }
}
