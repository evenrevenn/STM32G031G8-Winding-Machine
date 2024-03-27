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
                                 guide_frequency_(1),
                                 drum_frequency_(1),
                                 PARAMS_stepper_guide_{guide_steps_, guide_frequency_, manager_calls_queue_, manager_calls_semaphhore_},
                                 PARAMS_stepper_drum_{guide_steps_, guide_frequency_, manager_calls_queue_, manager_calls_semaphhore_}
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

void GlobalManager::readCall(QueueHandle_t call_queue)
{
    MANAGER_CALL call;

    if (!xQueueReceive(call_queue, &call, 0)){
        return;
    }

    switch(call.id){
        case MANAGER_CALL_IDS::TEST_ID:
            print("tested %d", 0);
            break;
        
        case MANAGER_CALL_IDS::GUIDE_DONE_ID:
            vTaskSuspend(TASK_HANDLE_stepper_drum_);
            vTaskSuspend(TASK_HANDLE_stepper_guide_);
            DrumStepper::switchEnable(false);
            GuideStepper::switchEnable(false);
            print("Movement completed");
            break;

        case MANAGER_CALL_IDS::WAVE_START_ID:
            /* s(steps) = L(mm) / 2(mm) * 400(steps/rot) */
            guide_steps_ = call.value / MM_PER_ROTATION * STEPS_PER_ROTATION;
            DrumStepper::switchEnable(true);
            GuideStepper::switchEnable(true);
            vTaskResume(TASK_HANDLE_stepper_drum_);
            vTaskResume(TASK_HANDLE_stepper_guide_);
            print("Waving started, length: %u mm\n", guide_steps_);
            break;

        case MANAGER_CALL_IDS::SPEED_CHANGE_ID:
            if (call.value <= 1000){
                drum_frequency_ = DrumStepper::calcHalfPeriod(call.value);
                guide_frequency_ = GuideStepper::calcHalfPeriod(drum_frequency_, wire_thickness_);
                print("Speed changed: %u steps/sec\n", configTICK_RATE_HZ / 2 / drum_frequency_);
            }
            break;

        case MANAGER_CALL_IDS::SPEED_INCREASE_ID:
            if ((int)call.value > 0 && drum_frequency_ <= DrumStepper::calcHalfPeriod(900)){
                drum_frequency_ += DrumStepper::calcHalfPeriod(100);
                guide_frequency_ = GuideStepper::calcHalfPeriod(drum_frequency_, wire_thickness_);
                print("Speed changed: %u steps/sec\n", configTICK_RATE_HZ / 2 / drum_frequency_);
            }
            else if ((int)call.value < 0 && drum_frequency_ > DrumStepper::calcHalfPeriod(100)){
                drum_frequency_ -= DrumStepper::calcHalfPeriod(100);
                guide_frequency_ = GuideStepper::calcHalfPeriod(drum_frequency_, wire_thickness_);
                print("Speed changed: %u steps/sec\n", configTICK_RATE_HZ / 2 / drum_frequency_);
            }
            break;
            
        case MANAGER_CALL_IDS::THICKNESS_CHANGE_ID:
            if (call.value <= 20 && call.value > 5){
                wire_thickness_ = call.value;
                guide_frequency_ = GuideStepper::calcHalfPeriod(drum_frequency_, wire_thickness_);
                print("Thickness changed: %u mm^-2\n", wire_thickness_);
            }
            break;

        case MANAGER_CALL_IDS::THICKNESS_INCREASE_ID:
            if ((int)call.value > 0 && call.value <= 19){
                wire_thickness_ += 1;
                guide_frequency_ = GuideStepper::calcHalfPeriod(drum_frequency_, wire_thickness_);
                print("Thickness changed: %u mm^-2\n", wire_thickness_);
            }
            else if ((int)call.value < 0 && call.value > 6){
                wire_thickness_ -= 1;
                guide_frequency_ = GuideStepper::calcHalfPeriod(drum_frequency_, wire_thickness_);
                print("Thickness changed: %u mm^-2\n", wire_thickness_);
            }   
            break;
        
        case MANAGER_CALL_IDS::DIRECTION_DRUM_CHANGE_ID:
            DrumStepper::changeDirection((Stepper::Direction)call.value);
            call.value == Stepper::DIR_FORWARD ? print("Drum direction changed to Forward\n") : print("Drum direction changed to Backward\n");
            break;

        case MANAGER_CALL_IDS::WAVE_PAUSE_ID:
            vTaskSuspend(TASK_HANDLE_stepper_drum_);
            vTaskSuspend(TASK_HANDLE_stepper_guide_);
            DrumStepper::switchEnable(false);
            GuideStepper::switchEnable(false);
            print("Waving paused\n");
            break;

        case MANAGER_CALL_IDS::WAVE_STOP_ID:
            vTaskSuspend(TASK_HANDLE_stepper_drum_);
            vTaskSuspend(TASK_HANDLE_stepper_guide_);
            DrumStepper::switchEnable(false);
            GuideStepper::switchEnable(false);
            guide_steps_ = 0;
            print("Waving stopped\n");
            break;
    }
}

bool GlobalManager::createSelf()
{
    manager_calls_queue_ = xQueueCreate(16, sizeof(MANAGER_CALL));
    manager_calls_semaphhore_ = xSemaphoreCreateCounting(16, 0);

    if (!manager_calls_queue_ || !manager_calls_semaphhore_){
        return false;
    }

    return xTaskCreate(GlobalManager::vManagerTask, "Manager task", configMINIMAL_STACK_SIZE, &PARAMS_manager_, configMAX_PRIORITIES - 1, &TASK_HANDLE_manager_);
}

bool GlobalManager::createUART()
{
	UART_transmit_queue_ = xQueueCreate(512, sizeof(char));

    if (!UART_transmit_queue_){
        return false;
    }

    return xTaskCreate(UARTHandler::vUARTTask, "UART task", configMINIMAL_STACK_SIZE, &PARAMS_uart_, 2, &TASK_HANDLE_uart_);
}

bool GlobalManager::createDecoder()
{
    UART_receive_queue_ = xQueueCreate(128, sizeof(char));

    if (!UART_receive_queue_){
        return false;
    }

    return xTaskCreate(AsciiDecoder::vDecoderTask, "Decoder task", configMINIMAL_STACK_SIZE, &PARAMS_decoder_, 2, &TASK_HANDLE_uart_);
}

bool GlobalManager::createSteppers()
{
    wire_thickness_ = 13;
    guide_steps_ = 0;
    guide_frequency_ = 1;
    drum_frequency_ = 1;

    return xTaskCreate(DrumStepper::vStepperDrumTask, "Step drum task", configMINIMAL_STACK_SIZE, &PARAMS_stepper_drum_, 3, &TASK_HANDLE_stepper_drum_) &&
           xTaskCreate(GuideStepper::vStepperGuideTask, "Step guide task", configMINIMAL_STACK_SIZE, &PARAMS_stepper_guide_, 3, &TASK_HANDLE_stepper_guide_);
}