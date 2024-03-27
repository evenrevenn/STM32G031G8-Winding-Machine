#include "Circular_buffer.h"

#ifdef __cplusplus
extern "C"{
#endif

#include "FreeRTOS.h"

#ifdef __cplusplus
}
#endif

#include <string.h>

CircularBuffer::CircularBuffer(size_t size):
buffer_start_((const uint8_t *)pvPortCalloc(size, sizeof(uint8_t))),
buffer_end_(buffer_start_ + size),
read_pos_(buffer_start_),
write_pos_((uint8_t *)read_pos_ + 1)
{
}

CircularBuffer::~CircularBuffer()
{
    vPortFree((void *)buffer_start_);
}

CircularFreeSpace CircularBuffer::getFreeSpace() const
{
    CircularFreeSpace free;
    
    free.right = write_pos_ > read_pos_ ? buffer_end_ - write_pos_ : read_pos_ - write_pos_ - 1;
    free.left = write_pos_ > read_pos_ ? read_pos_ - buffer_start_ - 1 : 0;
    free.total = free.left + free.right;

    return free;
}

CircularOccupiedSpace CircularBuffer::getOccupiedSpace() const
{
    CircularFreeSpace occupied;
    
    occupied.right = write_pos_ > read_pos_ ? write_pos_ - read_pos_ - 1 : buffer_end_ - read_pos_;
    occupied.left = write_pos_ > read_pos_ ? 0 : write_pos_ - buffer_start_;
    occupied.total = occupied.left + occupied.right;

    return occupied;
}

size_t CircularBuffer::readOne(uint8_t *destination)
{
    if(read_pos_ != buffer_end_)
    {
        if (read_pos_ + 1 < write_pos_){
            read_pos_++;
            *destination = *read_pos_;
            return 1;
        }
        else{
            return 0;
        }
    }
    else{
        if (write_pos_ != buffer_start_){
            read_pos_ = buffer_start_;
            *destination = *read_pos_;
            return 1;
        }
        else{
            return 0;
        }
    }
}

bool CircularBuffer::writeOne(const uint8_t *data)
{
    if(write_pos_ != buffer_end_)
    {
        if (write_pos_ + 1 != read_pos_){
            *write_pos_ = *data;
            write_pos_++;
            return true;
        }
        else{
            return false;
        }
    }
    else{
        if (read_pos_ != buffer_start_){
            *write_pos_ = *data;
            write_pos_ = (uint8_t *)buffer_start_;
            return true;
        }
        else{
            return false;
        }
    }
}

bool CircularBuffer::writeData(const uint8_t *data, size_t length)
{
    if (length == 1){
        return writeOne(data);
    }

    CircularFreeSpace free = getFreeSpace();
    if (free.total < length){
        return false;
    }

    if (free.right > length){
        memcpy(write_pos_, data, length);
        write_pos_ += length;
        return true;
    }
    else if (free.right == length){
        if (read_pos_ == buffer_start_){
            return false;
        }
        memcpy(write_pos_, data, length);
        write_pos_ = (uint8_t *)buffer_start_;
        return true;
    }
    else if (free.right < length){
        memcpy(write_pos_, data, free.right);
        write_pos_ = (uint8_t *)buffer_start_;
        memcpy(write_pos_, data, length - free.right);
        write_pos_ += length - free.right;
        return true;
    }

    return false;
}

size_t CircularBuffer::readData(uint8_t *destination, size_t length)
{
    if (length == 1){
        return readOne(destination);
    }

    size_t read_counter = 0;
    CircularFreeSpace occupied = getOccupiedSpace();

    if (occupied.right > 0){
        memcpy(destination, read_pos_ + 1, occupied.right);
        destination += occupied.right;
        read_pos_ += occupied.right;
        read_counter += occupied.right;
    }
    if (occupied.left > 0 && read_pos_ == buffer_end_){
        read_pos_ = buffer_start_;
        memcpy(destination, read_pos_, occupied.left);
        read_counter += occupied.left;
        read_pos_ += occupied.left - 1;
    }
    
    return read_counter;
}
