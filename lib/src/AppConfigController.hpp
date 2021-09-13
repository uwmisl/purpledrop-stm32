#pragma once

#include "AppConfig.hpp"
#include "Events.hpp"
#include "SafeFlashPersist.hpp"

#include <memory>

template<class Flash, uint32_t SectorAIdx, uint32_t SectorBIdx>
struct AppConfigController {
    void init(EventEx::EventBroker *broker) {
        mBroker = broker;

        // Load default app config
        AppConfig::init();

        // Load saved parameters from flash, or initialize with defaults
        load();

        // Register for relevant system events
        mSetParameterHandler.setFunction([this](auto e) { HandleSetParameter(e); } );
        mBroker->registerHandler(&mSetParameterHandler);
    }

private:
    EventEx::EventBroker *mBroker;
    EventEx::EventHandlerFunction<events::SetParameter> mSetParameterHandler;

    static SafeFlashPersist<Flash, SectorAIdx, SectorBIdx> flashPersist;
    static const uint32_t ParameterBlockId = 1;

    struct StoredParamRecord {
        uint16_t id;
        ConfigOptionValue value;
    } __attribute__ ((packed));

    void HandleSetParameter(events::SetParameter &e) {
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

    void load() {
        uint8_t *data;
        uint32_t size;

        if(flashPersist.read(&data, &size)) {
            if(size > 8) {
                uint32_t block_id = ((uint32_t *)data)[0];
                if(block_id != ParameterBlockId) {
                    // Someday: we may support multiple blocks
                    // Now: this means no parameters are stored
                    return;
                }
                uint32_t flash_length = ((uint32_t*)data)[1];
                StoredParamRecord *records = (StoredParamRecord*)&data[8];
                uint32_t num_records = flash_length / sizeof(StoredParamRecord);
                for(uint32_t i=0; i<num_records; i++) {
                    if(records[i].id >= AppConfig::MAX_OPT_ID) {
                        continue;
                    }
                    AppConfig::optionValues[records[i].id] = records[i].value;
                }
            }
        }
    }

    bool persist() {
        const uint32_t buf_size = AppConfig::N_OPT_DESCRIPTOR * sizeof(StoredParamRecord) + 8;
        std::unique_ptr<uint8_t[]> buf(new uint8_t[buf_size]);
        // Write out a block header with type and length first
        // to allow for storing multiple blocks in the future
        uint32_t *u32Buf = (uint32_t*)&buf[0];
        u32Buf[0] = ParameterBlockId;
        u32Buf[1] = buf_size - 8;
        for(uint32_t i=0; i<AppConfig::N_OPT_DESCRIPTOR; i++) {
            StoredParamRecord record;
            record.id = AppConfig::optionDescriptors[i].id;
            record.value = AppConfig::optionValues[record.id];
            memcpy(&buf[i * sizeof(record) + 8], &record, sizeof(record));
        }
        return flashPersist.write(buf.get(), buf_size + 8);
    }
};

template<class Flash, uint32_t SectorAIdx, uint32_t SectorBIdx> 
SafeFlashPersist<Flash, SectorAIdx, SectorBIdx> 
AppConfigController<Flash, SectorAIdx, SectorBIdx>::flashPersist;