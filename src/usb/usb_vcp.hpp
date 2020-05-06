
#include "CircularBuffer.hpp"
void VCP_Setup(IProducer<uint8_t> *rx_queue, CircularBuffer<uint8_t> *tx_queue);
// Returns the number of bytes sent
uint32_t VCP_Send(uint8_t * pbuf, uint32_t buf_len);
// Returns the number of bytes read
uint32_t VCP_Read(uint8_t * pbuf, uint32_t buf_len);
