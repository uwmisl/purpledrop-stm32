#pragma once

#include "Events.hpp"

struct ControlCmd {
    float target;
    uint8_t inputGroupsMask;
    uint8_t outputGroup;
    uint8_t enable;
};

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