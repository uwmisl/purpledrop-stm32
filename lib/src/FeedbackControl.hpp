#pragma once

#include "Events.hpp"

/** Capacitance feedback controller. 
* Implements the control loop by listening to capacitance measurement events
* and firing SetDutyCycle events. 
*/
class FeedbackControl {
public:

    void init(EventBroker *event_broker);

private:
    EventBroker *mEventBroker;

    EventEx::EventHandlerFunction<events::CapGroups> mCapGroupsHandler;
    EventEx::EventHandlerFunction<events::FeedbackCommand> mFeedbackCommandHandler;

    events::FeedbackCommand mCmd;
    float mLastError;
    float mIntegral;

    void HandleCapGroups(events::CapGroups &e);
    void HandleFeedbackCommand(events::FeedbackCommand &e);
};