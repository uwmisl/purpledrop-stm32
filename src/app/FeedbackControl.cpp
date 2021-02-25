#include "FeedbackControl.hpp"

void FeedbackControl::init(EventBroker *event_broker) {
    mEventBroker = event_broker;
    mLastError = 0.0;

    mCapGroupsHandler.setFunction([this](auto &e){HandleCapGroups(e);});
    mEventBroker->registerHandler(&mCapGroupsHandler);
    mFeedbackCommandHandler.setFunction([this](auto &e){HandleFeedbackCommand(e);});
    mEventBroker->registerHandler(&mFeedbackCommandHandler);
}

void FeedbackControl::HandleCapGroups(events::CapGroups &event) {
    float x = 0.0;
    float kp = AppConfig::FeedbackKp();
    float ki = AppConfig::FeedbackKi();
    float kd = AppConfig::FeedbackKd();

    if(mCmd.enable == 0) {
        return;
    }

    for(uint32_t ch=0; ch<event.measurements.size(); ch++) {
        if(mCmd.inputGroupsMask & (1<<ch)) {
            x += event.measurements[ch];
        }
    }
    float error = mCmd.targetCapacitance - x;

    mIntegral += error * ki;
    if(mIntegral * ki > 40) {
        mIntegral = 40 / ki;
    } else if(mIntegral *ki < -40) {
        mIntegral = -40 / ki;
    }

    float dutycycle = 200 + error * kp + mIntegral * ki + (error - mLastError) * kd;
    if(dutycycle > 255) {
        dutycycle = 255;
    } else if(dutycycle < 0) {
        dutycycle = 0;
    }

    events::SetDutyCycle set_event;
    set_event.updateA = true;
    set_event.updateB = false;
    set_event.dutyCycleA = (uint8_t)dutycycle;
    mEventBroker->publish(set_event);

    mLastError = error;
}

void FeedbackControl::HandleFeedbackCommand(events::FeedbackCommand &e) {
    mCmd = e;
}