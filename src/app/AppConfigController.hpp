#pragma once
#include "AppConfig.hpp"
#include "Events.hpp"

struct AppConfigController {
    void init(EventEx::EventBroker *broker);

private:    
    EventEx::EventBroker *mBroker;
    EventEx::EventHandlerFunction<events::SetParameter> mSetParameterHandler;


    void HandleSetParameter(events::SetParameter &e);
};