#pragma once

#include <cstdint>

#define CB_DEBUG

/* Abstract type for writing to a queue */
template<typename T>
struct IProducer {
    virtual bool push(T item) = 0;
};

/* Abstract type for consuming from a queue */
template <typename T>
struct IConsumer {
public: 
    virtual T pop() = 0;
    virtual bool empty() = 0;
    virtual uint32_t count() = 0;
};

/* Abstract type for a CircularBuffer */
template<typename T>
struct CircularBuffer : public IProducer<T>, IConsumer<T> {
    virtual bool push(T item) = 0;
    virtual T pop() = 0;
    virtual bool empty() = 0;
    virtual uint32_t count() = 0;
};

template <typename T, int size>
struct StaticCircularBuffer : public CircularBuffer<T>{
public:
    StaticCircularBuffer() {
        head = 0;
        tail = 0;
    }

    // Push a byte onto the back of the queue
    bool push(T item) {
        uint32_t fill_count = count();

#ifdef CB_DEBUG
        if(fill_count > highwater_mark) {
            highwater_mark = fill_count;
        }
#endif
        uint32_t new_head = (head + 1) % size;
        if(new_head == tail) {
            return false; // Full
        }
        buf[head] = item;
        head = new_head;
        return true;
    }

    // Pop a byte from the front of the queue
    // Caller must use empty() to check if bytes are available
    T pop() {
        T result = T();
        if(tail == head) {
            return result; 
        }
        //uint32_t new_tail = (tail + 1) % size;
        result = buf[tail];
        tail = (tail + 1) % size;
        return result;
    }

    // Get the number of items in the queue
    uint32_t count() {
        return (head - tail) % size;
    }

    bool empty() {
        return head == tail;
    }
private:
    uint32_t head;
    uint32_t tail;
    T buf[size];
#ifdef CB_DEBUG
    uint32_t highwater_mark;
#endif
};



