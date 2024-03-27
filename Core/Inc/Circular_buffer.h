#ifndef CIRCULAR_BUFFER
#define CIRCULAR_BUFFER

#include <stdint.h>
#include <stddef.h>

struct CircularFreeSpace
{
    size_t total;
    size_t right;
    size_t left;
};

typedef CircularFreeSpace CircularOccupiedSpace;

class CircularBuffer
{
public:
    CircularBuffer(size_t size);
    ~CircularBuffer();

    bool writeData(const uint8_t *data, size_t length);
    size_t readData(uint8_t *destination, size_t length);

    CircularFreeSpace getFreeSpace() const;
    CircularOccupiedSpace getOccupiedSpace() const;

private:
    size_t readOne(uint8_t *destination);
    bool writeOne(const uint8_t *data);

    const uint8_t* const buffer_start_;
    const uint8_t* const buffer_end_;

    const uint8_t *read_pos_;
    uint8_t *write_pos_;
};

#endif //CIRCULAR_BUFFER