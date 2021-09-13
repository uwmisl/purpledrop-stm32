#include <array>
#include <cstdint>
#include "modm/platform.hpp"
#include <modm/architecture/interface/assert.hpp>

#define FLASH_KEY1 (0x45670123)
#define FLASH_KEY2 (0xCDEF89AB)

struct FlashSection {
  uint32_t sector;
  uint32_t addr;
  uint32_t size;
};

static constexpr std::array<FlashSection, 2> Sectors = {
  FlashSection { 
    2,
    0x08008000,
    0x4000 // 16K
  },
  FlashSection { 
    3,
    0x0800C000,
    0x4000 // 16K
  }
};

/** Creates an interface to erase and program discrete blocks of flash
 * This is used to abstract the flash access on different chips for
 * SafeFlashPersist.
 */
class Stm32Flash {
public:
  static void unlock() {
    FLASH->KEYR = FLASH_KEY1;
    FLASH->KEYR = FLASH_KEY2;
  }

  static void lock() {
    FLASH->CR = 0;
    FLASH->CR = (FLASH_CR_LOCK);
  }

  static void erase_sector(uint8_t section_idx) {
    modm_assert(section_idx < Sectors.size(), "bad section_idx", "Invalid flash section was provided to erase_sector");
    auto section = Sectors[section_idx];
    // Wait for flash to be not busy
    while(FLASH->SR & FLASH_SR_BSY);

    FLASH->CR = (section.sector << FLASH_CR_SNB_Pos) | (FLASH_CR_SER);

    FLASH->CR |= FLASH_CR_STRT;

    while(FLASH->SR & FLASH_SR_BSY);
  }

  static void write(uint8_t section_idx, uint32_t offset, uint8_t *src, uint32_t length) {
    modm_assert(section_idx < Sectors.size(), "bad section_idx", "Invalid flash section was provided to write");
    auto section = Sectors[section_idx];

    // Wait for flash to be not busy
    while(FLASH->SR & FLASH_SR_BSY);

    // Program mode, using u8 program size
    FLASH->CR = (0 << FLASH_CR_PSIZE_Pos);
    FLASH->CR |= FLASH_CR_PG;
    
    uint8_t *dst = (uint8_t*)(section.addr + offset);
    for(uint32_t i=0; i<length; i++) {
      dst[i] = src[i];
      while(FLASH->SR & FLASH_SR_BSY);
    }

    while(FLASH->SR & FLASH_SR_BSY);
  }

  // Return a pointer which can be used for reading from the section
  static constexpr uint8_t * address(uint8_t section_idx) {
    modm_assert(section_idx < Sectors.size(), "bad section_idx", "Invalid flash section was provided to address");
    auto section = Sectors[section_idx];
    return (uint8_t*)section.addr;
  }

  static constexpr uint32_t size(uint8_t section_idx) {
    modm_assert(section_idx < Sectors.size(), "bad section_idx", "Invalid flash section was provided to address");
    auto section = Sectors[section_idx];
    return section.size;
  }

};