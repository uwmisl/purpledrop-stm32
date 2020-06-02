#include "modm/platform.hpp"

#define FLASH_KEY1 (0x45670123)
#define FLASH_KEY2 (0xCDEF89AB)

namespace flash {
  void unlock() {
    printf("Unlocking\n");
    FLASH->KEYR = FLASH_KEY1;
    FLASH->KEYR = FLASH_KEY2;
  }

  void lock() {
    printf("Locking\n");
    FLASH->SR = 0;
    FLASH->SR = (FLASH_CR_LOCK);
  }

  void erase_sector(uint8_t sector) {
    // Wait for flash to be not busy
    while(FLASH->SR & FLASH_SR_BSY);
    unlock();

    FLASH->CR = (sector << FLASH_CR_SNB_Pos) | (FLASH_CR_SER);

    FLASH->CR |= FLASH_CR_STRT;

    while(FLASH->SR & FLASH_SR_BSY);

    lock();
  }

  void write(uint8_t *dst, uint8_t *src, uint32_t length) {
    // Wait for flash to be not busy
    while(FLASH->SR & FLASH_SR_BSY);

    unlock();

    // Program mode, using u8 program size
    FLASH->CR = (0 << FLASH_CR_PSIZE_Pos);
    FLASH->CR = FLASH_CR_PG;

    for(uint32_t i=0; i<length; i++) {
      dst[i] = src[i];
    }

    while(FLASH->SR & FLASH_SR_BSY);
  }

} // namespace flash