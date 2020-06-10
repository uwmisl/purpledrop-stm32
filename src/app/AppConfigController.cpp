#include "AppConfigController.hpp"

#include "Events.hpp"
#include "SafeFlashPersist.hpp"

static const uint32_t FirstFlashSector = 2;
static const uint32_t FirstFlashAddr = 0x08008000;
static const uint32_t FirstFlashSize = 0x4000; // 16K
static const uint32_t SecondFlashSector = 3;
static const uint32_t SecondFlashAddr = 0x0800C000;
static const uint32_t SecondFlashSize = 0x4000; // 16K
static SafeFlashPersist<FirstFlashSector, FirstFlashAddr, FirstFlashSize,
    FirstFlashSector, FirstFlashAddr, FirstFlashSize> flashPersist;
static uint8_t flashBuf[sizeof(AppConfig::optionValues) + 8];

void AppConfigController::init(EventEx::EventBroker *broker) {
    mBroker = broker;

    // Load saved parameters from flash, or initialize with defaults
    load();

    mSetParameterHandler.setFunction([this](auto e) { HandleSetParameter(e); } );
    mBroker->registerHandler(&mSetParameterHandler);
}

void AppConfigController::HandleSetParameter(events::SetParameter &e) {
    uint32_t paramIdx = e.paramIdx;
    ConfigOptionValue paramValue = {.i32 = 0};
    if(e.paramIdx == 0xFFFFFFFF) {
        // Save parameters to flash
        if(persist()) {
            paramValue.i32 = 1; // Return 1 in paramValue to indicate successful save
        }
    } else if(e.paramIdx < AppConfig::MAX_OPT_ID) {
        if(e.writeFlag) {
            AppConfig::optionValues[e.paramIdx].i32 = e.paramValue.i32;
        }
        paramValue.i32 = AppConfig::optionValues[e.paramIdx].i32;
    }
    e.callback(paramIdx, paramValue);
}

bool AppConfigController::persist() {
    uint32_t *u32Buf = (uint32_t*)flashBuf;
    // Write out a block header with type and length first
    // to allow for storing multiple blocks in the future
    u32Buf[0] = 0; // Parameter section ID
    u32Buf[1] = sizeof(AppConfig::optionValues);
    memcpy(&u32Buf[2], AppConfig::optionValues, sizeof(AppConfig::optionValues));
    return flashPersist.write(flashBuf, sizeof(flashBuf));
}

void AppConfigController::load() {
    uint8_t *data;
    uint32_t size;
    uint32_t n_loaded = 0;

    if(flashPersist.read(&data, &size)) {
        if(size > 8) {
            // Allow for data with more or less options than the current code has in case
            // AppConfig::MAX_OPT_ID changes in the future
            // Skip first 8 bytes because these are data type and length
            uint32_t flash_length = ((uint32_t*)data)[1];
            uint32_t copySize = std::min(flash_length, (uint32_t)sizeof(AppConfig::optionValues));
            if(copySize + 8 <= size) {
                memcpy(AppConfig::optionValues, data + 8, copySize);
                n_loaded = copySize / sizeof(AppConfig::optionValues[0]);
            }
        }
    }

    // Load defaults for any options not loaded from flash
    for(uint32_t i=0; i<AppConfig::N_OPT_DESCRIPTOR; i++) {
        uint32_t id = AppConfig::optionDescriptors[i].id;
        if(id < AppConfig::MAX_OPT_ID && id > n_loaded) {
            AppConfig::optionValues[id] = AppConfig::optionDescriptors[i].defaultValue;
        }
    }
}