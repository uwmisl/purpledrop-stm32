#include "AppConfigController.hpp"

#include "Events.hpp"


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