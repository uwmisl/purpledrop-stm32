#include "AppConfigController.hpp"

#include "Events.hpp"
#include "SafeFlashPersist.hpp"

static const uint32_t FirstFlashSector = 2;
static const uint32_t FirstFlashAddr = 0x08008000;
static const uint32_t FirstFlashSize = 0x4000; // 16K
static const uint32_t SecondFlashSector = 2;
static const uint32_t SecondFlashAddr = 0x08008000;
static const uint32_t SecondFlashSize = 0x4000; // 16K
static SafeFlashPersist<FirstFlashSector, FirstFlashAddr, FirstFlashSize,
    FirstFlashSector, FirstFlashAddr, FirstFlashSize> persist;

void AppConfigController::init(EventEx::EventBroker *broker) {
    // This test of persistent storage is waiting on getting a custom linker
    // script that works with modm sorted out so that the flash sectors can
    // be set aside for it
    // uint8_t *stored_data;
    // uint32_t stored_length;
    // uint32_t counter = 0;
    // if(persist.read(&stored_data, &stored_length)) {
    //     counter = *((uint32_t*)stored_data);
    //     printf("Read back config %d", (int)counter);
    // } else {
    //     printf("Failed to read back config");
    // }
    // persist.write((uint8_t*)&counter, 4);

    mBroker = broker;

    mSetParameterHandler.setFunction([this](auto e) { HandleSetParameter(e); } );
    mBroker->registerHandler(&mSetParameterHandler);

    for(uint32_t i=0; i<AppConfig::N_OPT_DESCRIPTOR; i++) {
        uint32_t id = AppConfig::optionDescriptors[i].id;
        if(id < AppConfig::MAX_OPT_ID) {
            AppConfig::optionValues[id] = AppConfig::optionDescriptors[i].defaultValue;
        }
    }
}

void AppConfigController::HandleSetParameter(events::SetParameter &e) {
    events::SetParameterAck ackEvent;
    ackEvent.paramIdx = e.paramIdx;
    ackEvent.paramValue.i32 = 0;
    if(e.paramIdx < AppConfig::MAX_OPT_ID) {
        if(e.writeFlag) {
            AppConfig::optionValues[e.paramIdx].i32 = e.paramValue.i32;
        }
        ackEvent.paramValue.i32 = AppConfig::optionValues[e.paramIdx].i32;
    }
    mBroker->publish(ackEvent);
}