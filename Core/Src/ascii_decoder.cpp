#include "ascii_decoder.h"
#include <stdint.h>
#include <sys/types.h>
#include <cstring>

AsciiDecoder::AsciiDecoder(MANAGER_ACCESS call_access):
ascii_sym_(ascii_cmd_),
manager_access_(call_access)
{
}

AsciiDecoder::~AsciiDecoder()
{
}

bool AsciiDecoder::readFromQueue(const QueueHandle_t &receive_queue)
{
    char temp;

    while (ascii_sym_ < ascii_cmd_ + sizeof(ascii_cmd_) - 1)
    {
        if (!xQueueReceive(receive_queue, &temp, pdMS_TO_TICKS(10))){
            return false;
        }

        if (temp != '\r'){
            *ascii_sym_ = temp;
            ascii_sym_++;
            continue;
        }

        char *cmd = std::strtok(ascii_cmd_, "=");

        char *value_str = std::strtok(nullptr, "=");
        if (value_str){
            call_struct_.value = std::atoi(value_str);
        }
        
        if (parseCommand(cmd)){
            manager_access_.sendCall(call_struct_);
        }

        memset(ascii_cmd_, 0, sizeof(ascii_cmd_));
        ascii_sym_ = ascii_cmd_;

        return true;
    }

    memset(ascii_cmd_, 0, sizeof(ascii_cmd_));
    ascii_sym_ = ascii_cmd_;
    return false;
}

bool AsciiDecoder::parseCommand(const char *ascii_command)
{
    if (!strcmp(ascii_command, "test")){
        call_struct_.id = MANAGER_CALL_IDS::TEST_ID;
        return true;
    }
    else if (!strcmp(ascii_command, "StartWaving")){
        call_struct_.id = MANAGER_CALL_IDS::WAVE_START_ID;
        return true;
    }
    else if (!strcmp(ascii_command, "SetSpeed")){
        call_struct_.id = MANAGER_CALL_IDS::SPEED_CHANGE_ID;
        return true;
    }
    else if (!strcmp(ascii_command, "SetThickness")){
        call_struct_.id = MANAGER_CALL_IDS::THICKNESS_CHANGE_ID;
        return true;
    }
    else if (!strcmp(ascii_command, "SetDrumDirection")){
        call_struct_.id = MANAGER_CALL_IDS::DIRECTION_DRUM_CHANGE_ID;
        return true;
    }
    else if (!strcmp(ascii_command, "SetGuideDirection")){
        call_struct_.id = MANAGER_CALL_IDS::DIRECTION_GUIDE_CHANGE_ID;
        return true;
    }
    else if (!strcmp(ascii_command, "PauseWaving")){
        call_struct_.id = MANAGER_CALL_IDS::WAVE_PAUSE_ID;
        return true;
    }
    else if (!strcmp(ascii_command, "StopWaving")){
        call_struct_.id = MANAGER_CALL_IDS::WAVE_STOP_ID;
        return true;
    }
    else if (!strcmp(ascii_command, "ContinueWaving")){
        call_struct_.id = MANAGER_CALL_IDS::WAVE_CONTINUE_ID;
        return true;
    }
    else if (!strcmp(ascii_command, "GetHeapInfo")){
        call_struct_.id = MANAGER_CALL_IDS::HEAP_INFO_ID;
        return true;
    }
    else if (!strcmp(ascii_command, "GetStackInfo")){
        call_struct_.id = MANAGER_CALL_IDS::STACK_INFO_ID;
        return true;
    }

    return false;
}

void AsciiDecoder::vDecoderTask(void *pvParameters)
{
    task_params::PARAMS_decoder_t decoder_params = *static_cast<task_params::PARAMS_decoder_t *>(pvParameters);
    AsciiDecoder decoder(decoder_params.call_access);

    while(true)
    {
        decoder.readFromQueue(decoder_params.receive_queue);
    }
}
