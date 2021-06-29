#include "FeedbackControl.hpp"

enum ControlMode_e : uint8_t {
    Disabled = 0,
    Normal = 1,
    Differential = 2
};

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

    if(mCmd.mode == Disabled) {
        return;
    }

    // Always Sum all of the positive feedback inputs
    // In differential mode, also sum the negative feedback inputs
    for(uint32_t ch=0; ch<event.measurements.size(); ch++) {
        // Compute scale value to convert raw counts to pF
        float gain = AppConfig::CapAmplifierGain() * AppConfig::HvControlTarget() * 1e12 * 4096 / 3.3;
        if(event.scanGroups.getGroupSetting(ch) & 1) {
            gain *= AppConfig::LowGainR();
        } else {
            gain *= AppConfig::HighGainR();
        }
        if(mCmd.measureGroupsPMask & (1<<ch)) {
            x += event.measurements[ch] / gain;
        }
        if(mCmd.mode == Differential && mCmd.measureGroupsNMask & (1<<ch)) {
            x -= event.measurements[ch] / gain;
        }
    }

    float error = mCmd.target - x;

    mIntegral += error * ki;
    if(mIntegral * ki > 40) {
        mIntegral = 40 / ki;
    } else if(mIntegral *ki < -40) {
        mIntegral = -40 / ki;
    }

    int16_t feedback = (int16_t)(error * kp + mIntegral * ki + (error - mLastError) * kd);
    mLastError = error;
    int16_t pos_output, neg_output;
    if(feedback > 0) {
        pos_output = (int16_t)mCmd.baseline + feedback;
        if(pos_output > 255) {
            pos_output = 255;
        }
        neg_output = pos_output - feedback;
        if(neg_output < 0) {
            neg_output = 0;
        }
    } else {
        neg_output = (int16_t)mCmd.baseline - feedback;
        if(neg_output > 255) {
            neg_output = 255;
        }
        pos_output = neg_output + feedback;
        if(pos_output < 0) {
            pos_output = 0;
        }
    }

    events::SetDutyCycle set_event;
    set_event.updateA = true;
    set_event.updateB = true;
    set_event.dutyCycleA = (uint8_t)pos_output;
    set_event.dutyCycleB = (uint8_t)neg_output;
    mEventBroker->publish(set_event);
}

void FeedbackControl::HandleFeedbackCommand(events::FeedbackCommand &e) {
    mCmd = e;
}