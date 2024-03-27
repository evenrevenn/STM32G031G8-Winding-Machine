#ifndef ASCII_DECODER_H
#define ASCII_DECODER_H

#include "global_manager.h"

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include <string>

class AsciiDecoder
{
public:
    AsciiDecoder(MANAGER_ACCESS call_access);
    ~AsciiDecoder();

    bool readFromQueue(const QueueHandle_t &receive_queue);
    bool parseCommand(const char *ascii_command);

    static void vDecoderTask(void *pvParameters);

private:
    char ascii_cmd_[100];
    char *ascii_sym_;

    MANAGER_ACCESS manager_access_;
    MANAGER_CALL call_struct_;
};

#endif //ASCII_DECODER_H