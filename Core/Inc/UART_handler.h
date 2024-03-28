#ifndef UART_HANDLER
#define UART_HANDLER

#include "stm32f1xx.h"

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

#include "Circular_buffer.h"

#define printBufferSize 128

#define print(...) UARTHandler::getInstance().printLog(__VA_ARGS__)

class UARTHandler
{
public:
    static UARTHandler &getInstance();
    ~UARTHandler();

    void printLog(const char *format, ...);

    void writeToQueue(const QueueHandle_t &queue);
    void readFromQueue(const QueueHandle_t &queue);

    static void vUARTTask(void *pvParameters);

protected:
    UARTHandler();

private:
    void initUSART();

    CircularBuffer buffer_;
    char vsprintf_buffer_[printBufferSize];

    bool msg_buffered_;
};

#endif //UART_HANDLER