#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include "FreeRTOS.h"
#include "queue.h"
#include "stm32g0xx.h"
#include "semphr.h"

// #include "stepper.h"

/*--------------------------------------------*/
/* Manager access */

namespace MANAGER_CALL_IDS{

enum CALL_IDS{
    TEST_ID = 0x0,
    WAVE_START_ID = 0x10,
    WAVE_PAUSE_ID = 0x11,
    WAVE_STOP_ID = 0x12,
    WAVE_CONTINUE_ID = 0x13,
    SPEED_CHANGE_ID = 0x20,
    SPEED_INCREASE_ID = 0x21,
    DIRECTION_DRUM_CHANGE_ID = 0x22,
    DIRECTION_GUIDE_CHANGE_ID = 0x23,
    THICKNESS_CHANGE_ID = 0x24,
    THICKNESS_INCREASE_ID = 0x25,
    GUIDE_DONE_ID = 0x30,
};

} //namespace CALL_IDS

struct MANAGER_CALL{
    MANAGER_CALL_IDS::CALL_IDS id;
    uint32_t value;
};

struct MANAGER_ACCESS{
    bool sendCall(MANAGER_CALL call)
    {
        return xQueueSendToBack(calls_queue, &call, 0) ? xSemaphoreGive(calls_semaphore) : false;
    }
    bool sendCallFromISR(MANAGER_CALL call)
    {
        return xQueueSendFromISR(calls_queue, &call, NULL) ? xSemaphoreGiveFromISR(calls_semaphore, NULL) : false;
    }

    const QueueHandle_t& calls_queue;
    const SemaphoreHandle_t& calls_semaphore;
};

/*--------------------------------------------*/
/* Task parameters */

namespace task_params{

struct PARAMS_manager_t{
    MANAGER_ACCESS call_access;
};

struct PARAMS_uart_t{
    const QueueHandle_t& transmit_queue;
    // const QueueHandle_t& receive_queue;
};

struct PARAMS_decoder_t{
    const QueueHandle_t& receive_queue;
    MANAGER_ACCESS call_access;
};

struct PARAMS_stepper_t{
    uint32_t &steps;
    uint32_t &frequency;
    MANAGER_ACCESS call_access;
};

} //namespace task_params

/*--------------------------------------------*/




/*--------------------------------------------*/


class GlobalManager
{
public:
    static GlobalManager &getInstance();
    bool createTasks();

    void readCall(QueueHandle_t call_queue);

    const QueueHandle_t &getUARTTransmitQueue() const {return UART_transmit_queue_;}
    const QueueHandle_t &getUARTReceiveQueue() const {return UART_receive_queue_;}
    const MANAGER_ACCESS getManagerAccess() const {return MANAGER_ACCESS{manager_calls_queue_, manager_calls_semaphhore_};}

    static void vManagerTask(void *pvParameters);

private:
    GlobalManager();
/*--------------------------------------------*/
/* Manager (this) */
    bool createSelf();
    QueueHandle_t manager_calls_queue_;
    SemaphoreHandle_t manager_calls_semaphhore_;
    task_params::PARAMS_manager_t PARAMS_manager_;
    TaskHandle_t TASK_HANDLE_manager_;
/*--------------------------------------------*/
/* UART */
    bool createUART();
    QueueHandle_t UART_transmit_queue_;
    QueueHandle_t UART_receive_queue_;
    task_params::PARAMS_uart_t PARAMS_uart_;
    TaskHandle_t TASK_HANDLE_uart_;
/*--------------------------------------------*/
/* Decoder */
    bool createDecoder();
    task_params::PARAMS_decoder_t PARAMS_decoder_;
    TaskHandle_t TASK_HANDLE_decoder_;
/*--------------------------------------------*/
/* Steppers */
    bool createSteppers();
    uint16_t wire_thickness_;
    uint32_t guide_steps_;
    uint32_t drum_frequency_HZ_;
    TickType_t guide_delay_ticks_;
    TickType_t drum_delay_ticks_;
    task_params::PARAMS_stepper_t PARAMS_stepper_guide_;
    TaskHandle_t TASK_HANDLE_stepper_guide_;
    task_params::PARAMS_stepper_t PARAMS_stepper_drum_;
    TaskHandle_t TASK_HANDLE_stepper_drum_;
/*--------------------------------------------*/
};

#endif //CONFIG_MANAGER_H