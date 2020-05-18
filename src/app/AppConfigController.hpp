#pragma once
#include "AppConfig.hpp"
#include "Events.hpp"

struct AppConfigController {
    void init(EventEx::EventBroker *broker) {
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

private:    
    EventEx::EventBroker *mBroker;
    EventEx::EventHandlerFunction<events::SetParameter> mSetParameterHandler;


    void HandleSetParameter(events::SetParameter &e);
};