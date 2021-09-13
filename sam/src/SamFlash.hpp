#include <array>
#include <cstdint>
#include "modm/platform.hpp"
#include <modm/architecture/interface/assert.hpp>

struct FlashSection {
  uint32_t start_page;
};

static const uint32_t NumPages = 8;

static constexpr std::array<FlashSection, 2> Sectors = {
  FlashSection {
    16
  },
  FlashSection {
    24
  }
};

constexpr uint8_t* startAddress(const uint32_t page) {
  return (uint8_t*)((uint8_t*)IFLASH_ADDR + page * 512);
}

/** Creates an interface to erase and program discrete blocks of flash
 * This is used to abstract the flash access on different chips for
 * SafeFlashPersist.
 */
class SamFlash {
public:
  static void unlock() {
    // NOP
  }

  static void lock() {
    // NOP
    // No locking done on SAMG
  }

  static void erase_sector(uint8_t section_idx) {
    static_assert(NumPages == 8, "erase_sector() only supports 8 page erase. Sorry.");
    modm_assert(section_idx < Sectors.size(), "bad section_idx", "Invalid flash section was provided to erase_sector");
    auto section = Sectors[section_idx];

    // Wait for flash to be not busy
    while(!(EFC->EEFC_FSR & EEFC_FSR_FRDY));

    // Erase 8 pages
    EFC->EEFC_FCR = EEFC_FCR_FCMD_EPA | EEFC_FCR_FARG((section.start_page & ~7) | 1) | EEFC_FCR_FKEY_PASSWD;

    modm_assert(!(EFC->EEFC_FSR & (EEFC_FSR_FLOCKE | EEFC_FSR_FCMDE)), "eraseerr", "Flash lock error after write");

    // Wait for flash to be not busy
    while(!(EFC->EEFC_FSR & EEFC_FSR_FRDY));
  }

  static void write_single_page(uint32_t page, uint32_t offset, uint8_t *src, uint32_t length) {
    uint8_t* addr = startAddress(page);
    uint32_t idx = 0;
    uint32_t word;

    // We have to do 32-bit accesses, so if the offset isn't aligned, some
    // extra work is needed
    if(offset & 0x3) {
      uint32_t i = offset & 0x3;
      word = 0xffffffff;
      while(i < 4 && i < length) {
        word &= ~(0xff<<(i*8)) | (src[idx] << (i * 8));
        i++;
        idx++;
      }
      *((volatile uint32_t*)(addr + (offset & ~3))) = word;
    }
    while(idx < length - 3) {
      *((volatile uint32_t*)(addr + idx + offset)) = *((uint32_t*)(src + idx));
      idx += 4;
    }

    if(idx < length) {
      word = 0xffffffff;
      uint32_t i = 0;
      while(idx + i < length) {
        word &= ~(0xff<<(i*8)) | src[idx+i] << (i*8);
        i++;
      }
      *((volatile uint32_t*)(addr + idx + offset)) = word;
    }

    EFC->EEFC_FCR = EEFC_FCR_FCMD_WP | EEFC_FCR_FARG(page) | EEFC_FCR_FKEY_PASSWD;

    // Wait for flash to be not busy
    while(!(EFC->EEFC_FSR & EEFC_FSR_FRDY));

    modm_assert(!(EFC->EEFC_FSR & EEFC_FSR_FLOCKE), "flocke", "Flash lock error after write");
  }

  static void write(uint8_t section_idx, uint32_t offset, uint8_t *src, uint32_t length) {
    modm_assert(section_idx < Sectors.size(), "bad section_idx", "Invalid flash section was provided to write");
    auto section = Sectors[section_idx];

    for(uint32_t page = section.start_page; page < section.start_page + NumPages; page++) {
      uint32_t first = (page - section.start_page) * 512;
      uint32_t last = first + 512;
      if(offset < last && offset + length > first) {
        uint32_t page_offset = offset;
        uint32_t page_write_length = 512;
        if(offset > first) {
          page_offset = offset - first;
          page_write_length -= page_offset;
        }
        if(offset + length < last) {
          page_write_length -= last - offset - length;
        }
        write_single_page(page, page_offset, src, page_write_length);
        src += page_write_length;
      }
    }
  }

  // Return a pointer which can be used for reading from the section
  static constexpr uint8_t * address(uint8_t section_idx) {
    modm_assert(section_idx < Sectors.size(), "bad section_idx", "Invalid flash section was provided to address");
    auto section = Sectors[section_idx];
    return startAddress(section.start_page);
  }

  static constexpr uint32_t size(uint8_t section_idx) {
    modm_assert(section_idx < Sectors.size(), "bad section_idx", "Invalid flash section was provided to address");
    return NumPages * 512;
  }

};