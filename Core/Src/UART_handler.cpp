#include "UART_handler.h"
#include <cstdarg>
#include <cstdio>
#include <cstring>

#include "global_manager.h"

UARTHandler::UARTHandler():
buffer_(512),
vsprintf_buffer_("")
{
}

void UARTHandler::initUSART()
{
	RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
	RCC->APB2ENR |= RCC_APB2ENR_USART1EN;

	// Configuring UART1 Pins
	GPIOA->CRH |= GPIO_CRH_CNF9_1;
	GPIOA->CRH &= ~(GPIO_CRH_CNF9_0);

	GPIOA->CRH |= GPIO_CRH_MODE9_Msk;


	GPIOA->CRH &= ~(GPIO_CRH_CNF10_1);
	GPIOA->CRH |= GPIO_CRH_CNF10_0;

	GPIOA->CRH &= ~(GPIO_CRH_MODE10_Msk);


	// Configuring UART
	USART1->CR1 |= USART_CR1_UE;
	USART1->CR1 |= USART_CR1_TE;
	USART1->CR1 |= USART_CR1_RE;

	USART1->CR1 &= ~(USART_CR1_M);
	USART1->BRR = 625UL;

	USART1->CR1 |= USART_CR1_RXNEIE;

	NVIC_EnableIRQ(USART1_IRQn);
}

UARTHandler &UARTHandler::getInstance()
{
    static UARTHandler uart_handler;
	static bool is_first = true;

	if (is_first){
		uart_handler.initUSART();
		is_first = false;
	}

    return uart_handler;
}

UARTHandler::~UARTHandler()
{
    RCC->APB2ENR &= ~(RCC_APB2ENR_USART1EN);
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
		USART1->CR1 |= USART_CR1_TXEIE;
	}
	else{
		vTaskDelay(pdMS_TO_TICKS(10));
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
	task_params::PARAMS_uart_t uart_params = *static_cast<task_params::PARAMS_uart_t *>(pvParameters);
    UARTHandler &uart_handler = UARTHandler::getInstance();

    while(true)
    {
        uart_handler.writeToQueue(uart_params.transmit_queue);
    }
}
