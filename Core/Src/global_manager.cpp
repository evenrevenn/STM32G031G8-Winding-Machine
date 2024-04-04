#include "global_manager.h"
#include "UART_handler.h"
#include "ascii_decoder.h"
#include "steppers.h"

#define STEPS_PER_ROTATION 400U
#define MM_PER_ROTATION 2U

GlobalManager::GlobalManager() : PARAMS_manager_{manager_calls_queue_, manager_calls_semaphhore_},
                                 PARAMS_uart_{UART_transmit_queue_ /*, UART_receive_queue_*/},
                                 PARAMS_decoder_{UART_receive_queue_, manager_calls_queue_, manager_calls_semaphhore_},
                                 wire_thickness_(13),
                                 guide_steps_(0),
                                 drum_frequency_HZ_(10),
                                 guide_delay_ticks_(100000),
                                 drum_delay_ticks_(100000),
                                 PARAMS_stepper_guide_{guide_steps_, guide_delay_ticks_, manager_calls_queue_, manager_calls_semaphhore_},
                                 PARAMS_stepper_drum_{guide_steps_, drum_delay_ticks_, manager_calls_queue_, manager_calls_semaphhore_}
{
}


GlobalManager &GlobalManager::getInstance()
{
    static GlobalManager manager;

    return manager;
}

void GlobalManager::vManagerTask(void *pvParameters)
{
    task_params::PARAMS_manager_t manager_params = *static_cast<task_params::PARAMS_manager_t *>(pvParameters);
    GlobalManager &manager = GlobalManager::getInstance();

    while(true)
    {
        xSemaphoreTake(manager_params.call_access.calls_semaphore, portMAX_DELAY);

        manager.readCall(manager_params.call_access.calls_queue);
    }
}

bool GlobalManager::createTasks()
{
    if (!createSelf()){
        return false;
    }
    if (!createUART()){
        return false;
    }
    if (!createDecoder()){
        return false;
    }
    if (!createSteppers()){
        return false;
    }

    return true;
}

/*--------------------------------------------*/
/* Call process */

void GlobalManager::readCall(QueueHandle_t call_queue)
{
    MANAGER_CALL call;

    if (!xQueueReceive(call_queue, &call, 0)){
        return;
    }

    switch(call.id){
        case MANAGER_CALL_IDS::TEST_ID:
            print("tested %s %s\n", "000", "111");
            break;
        
        case MANAGER_CALL_IDS::GUIDE_DONE_ID:
            vTaskSuspend(TASK_HANDLE_stepper_drum_);
            vTaskSuspend(TASK_HANDLE_stepper_guide_);
            DrumStepper::switchEnable(false);
            GuideStepper::switchEnable(false);
            print("Movement completed\n");
            break;

        case MANAGER_CALL_IDS::WAVE_START_ID:
            /* s(steps) = L(mm) * 400(steps/rot) / 2(mm/rot) */
            guide_steps_ = call.value * STEPS_PER_ROTATION / MM_PER_ROTATION;
            DrumStepper::switchEnable(true);
            GuideStepper::switchEnable(true);
            vTaskResume(TASK_HANDLE_stepper_drum_);
            vTaskResume(TASK_HANDLE_stepper_guide_);
            print("Waving started, length: %u mm\n", call.value, guide_steps_);
            break;

        case MANAGER_CALL_IDS::SPEED_CHANGE_ID:
            if (call.value <= 1000 && call.value > 0){
                drum_frequency_HZ_ = call.value;
                drum_delay_ticks_ = DrumStepper::calcHalfPeriod(drum_frequency_HZ_);
                guide_delay_ticks_ = GuideStepper::calcHalfPeriod(drum_delay_ticks_, wire_thickness_);
                print("Speed changed: %u steps/sec\n", drum_frequency_HZ_);
            }
            break;

        case MANAGER_CALL_IDS::SPEED_INCREASE_ID:{
            int32_t value = *reinterpret_cast<int32_t *>(&call.value);

            if (value > 0 && drum_frequency_HZ_ <= 900){
                drum_frequency_HZ_ += 100;
                drum_delay_ticks_ = DrumStepper::calcHalfPeriod(drum_frequency_HZ_);
                guide_delay_ticks_ = GuideStepper::calcHalfPeriod(drum_delay_ticks_, wire_thickness_);
                print("Speed changed: %u steps/sec\n", drum_frequency_HZ_);
            }
            else if (value < 0 && drum_frequency_HZ_ > 100){
                drum_frequency_HZ_ -= 100;
                drum_delay_ticks_ = DrumStepper::calcHalfPeriod(drum_frequency_HZ_);
                guide_delay_ticks_ = GuideStepper::calcHalfPeriod(drum_delay_ticks_, wire_thickness_);
                print("Speed changed: %u steps/sec\n", drum_frequency_HZ_);
            }
            break;
        }
            
        case MANAGER_CALL_IDS::THICKNESS_CHANGE_ID:
            if (call.value <= 20 && call.value > 5){
                wire_thickness_ = call.value;
                guide_delay_ticks_ = GuideStepper::calcHalfPeriod(drum_delay_ticks_, wire_thickness_);
                print("Thickness changed: %u mm^-2\n", wire_thickness_);
            }
            break;

        case MANAGER_CALL_IDS::THICKNESS_INCREASE_ID:{
            int32_t value = *reinterpret_cast<int32_t *>(&call.value);

            if (value > 0 && call.value <= 19){
                wire_thickness_ += 1;
                guide_delay_ticks_ = GuideStepper::calcHalfPeriod(drum_delay_ticks_, wire_thickness_);
                print("Thickness changed: %u mm^-2\n", wire_thickness_);
            }
            else if (value < 0 && call.value > 6){
                wire_thickness_ -= 1;
                guide_delay_ticks_ = GuideStepper::calcHalfPeriod(drum_delay_ticks_, wire_thickness_);
                print("Thickness changed: %u mm^-2\n", wire_thickness_);
            }   
            break;
        }
        
        case MANAGER_CALL_IDS::DIRECTION_DRUM_CHANGE_ID:
            DrumStepper::changeDirection((Stepper::Direction)call.value);
            call.value == Stepper::DIR_FORWARD ? print("Drum direction changed to Forward\n") : print("Drum direction changed to Backward\n");
            break;

        case MANAGER_CALL_IDS::DIRECTION_GUIDE_CHANGE_ID:
            GuideStepper::changeDirection((Stepper::Direction)call.value);
            call.value == Stepper::DIR_FORWARD ? print("Guide direction changed to Forward\n") : print("Guide direction changed to Backward\n");
            break;

        case MANAGER_CALL_IDS::WAVE_PAUSE_ID:
            vTaskSuspend(TASK_HANDLE_stepper_drum_);
            vTaskSuspend(TASK_HANDLE_stepper_guide_);
            DrumStepper::switchEnable(false);
            GuideStepper::switchEnable(false);
            {/* L(mm) = s(steps) * 2(mm/rot) / 400(steps/rot)  */
            uint32_t mm_left = guide_steps_ * MM_PER_ROTATION / STEPS_PER_ROTATION;
            print("Waving paused, mm left: %u\n", mm_left);}
            break;

        case MANAGER_CALL_IDS::WAVE_STOP_ID:
            vTaskSuspend(TASK_HANDLE_stepper_drum_);
            vTaskSuspend(TASK_HANDLE_stepper_guide_);
            DrumStepper::switchEnable(false);
            GuideStepper::switchEnable(false);
            {/* L(mm) = s(steps) * 2(mm/rot) / 400(steps/rot)  */
            uint32_t mm_left = guide_steps_ * MM_PER_ROTATION / STEPS_PER_ROTATION;
            print("Waving stopped, mm left: %u\n", mm_left);}
            guide_steps_ = 0;
            break;

        case MANAGER_CALL_IDS::WAVE_CONTINUE_ID:
            DrumStepper::switchEnable(true);
            GuideStepper::switchEnable(true);
            vTaskResume(TASK_HANDLE_stepper_drum_);
            vTaskResume(TASK_HANDLE_stepper_guide_);
            {/* L(mm) = s(steps) * 2(mm/rot) / 400(steps/rot)  */
            uint32_t mm_left = guide_steps_ * MM_PER_ROTATION / STEPS_PER_ROTATION;
            print("Waving continued, mm left: %u\n", mm_left);}
            break;
        
        case MANAGER_CALL_IDS::HEAP_INFO_ID:
            print("Heap free size: %u\n", xPortGetFreeHeapSize());
            break;

        case MANAGER_CALL_IDS::STACK_INFO_ID:
            print("Manager stack: %u\n", uxTaskGetStackHighWaterMark(TASK_HANDLE_manager_));
            print("UART stack: %u\n", uxTaskGetStackHighWaterMark(TASK_HANDLE_uart_));
            print("Decoder stack: %u\n", uxTaskGetStackHighWaterMark(TASK_HANDLE_decoder_));
            print("Guide stack: %u\n", uxTaskGetStackHighWaterMark(TASK_HANDLE_stepper_guide_));
            print("Drum stack: %u\n", uxTaskGetStackHighWaterMark(TASK_HANDLE_stepper_drum_));
            break;
    }
}

/*--------------------------------------------*/

bool GlobalManager::createSelf()
{
    manager_calls_queue_ = xQueueCreate(8, sizeof(MANAGER_CALL));
    manager_calls_semaphhore_ = xSemaphoreCreateCounting(8, 0);

    if (!manager_calls_queue_ || !manager_calls_semaphhore_){
        return false;
    }

    return xTaskCreate(GlobalManager::vManagerTask, "Manager task", 512, &PARAMS_manager_, configMAX_PRIORITIES - 1, &TASK_HANDLE_manager_);
}

bool GlobalManager::createUART()
{
	UART_transmit_queue_ = xQueueCreate(256, sizeof(char));

    if (!UART_transmit_queue_){
        return false;
    }

    return xTaskCreate(UARTHandler::vUARTTask, "UART task", configMINIMAL_STACK_SIZE, &PARAMS_uart_, 2, &TASK_HANDLE_uart_);
}

bool GlobalManager::createDecoder()
{
    UART_receive_queue_ = xQueueCreate(64, sizeof(char));

    if (!UART_receive_queue_){
        return false;
    }

    return xTaskCreate(AsciiDecoder::vDecoderTask, "Decoder task", configMINIMAL_STACK_SIZE, &PARAMS_decoder_, 2, &TASK_HANDLE_decoder_);
}

bool GlobalManager::createSteppers()
{
    wire_thickness_ = 13;
    guide_steps_ = 0;
    drum_frequency_HZ_ = 10;
    drum_delay_ticks_ = DrumStepper::calcHalfPeriod(drum_frequency_HZ_);
    guide_delay_ticks_ = GuideStepper::calcHalfPeriod(drum_delay_ticks_, wire_thickness_);

    return xTaskCreate(DrumStepper::vStepperDrumTask, "Step drum task", configMINIMAL_STACK_SIZE, &PARAMS_stepper_drum_, 3, &TASK_HANDLE_stepper_drum_) && 
    xTaskCreate(GuideStepper::vStepperGuideTask, "Step guide task", configMINIMAL_STACK_SIZE, &PARAMS_stepper_guide_, 3, &TASK_HANDLE_stepper_guide_);
}