#include "gtest/gtest.h"
#include "EventEx.hpp"

using namespace EventEx;

struct TestEvent1 : public Event {
    TestEvent1() : value(0) {}
    TestEvent1(uint32_t v) : value(v) {}

    uint32_t value;
};
struct TestEvent2 : public Event {
    TestEvent2() : value(0) {}
    TestEvent2(uint32_t v) : value(v) {}

    uint32_t value;
};

template<typename T>
struct FlagHandler : EventHandler<T> {
    FlagHandler() : fired(false) {}

    virtual void operator()(T &e) {
        fired = true;
    }

    bool fired;
};


struct EventBrokerTest : public ::testing::Test {
    void SetUp() {
    
    }

    EventBroker broker;
};

TEST_F(EventBrokerTest, dispatch_event_with_no_listeners) {
    TestEvent1 event;
    broker.publish(event);
}

TEST_F(EventBrokerTest, register_handler_object) {
    FlagHandler<TestEvent1> flag;
    TestEvent1 event;

    broker.registerHandler(&flag);
    ASSERT_FALSE(flag.fired);
    broker.publish(event);
    ASSERT_TRUE(flag.fired);
}

TEST_F(EventBrokerTest, register_handler_function) {
    TestEvent1 event(42);
    uint32_t flag = 0;
    EventHandlerFunction<TestEvent1> handler([&](TestEvent1 &event) { flag = event.value; });
    broker.registerHandler(&handler);
    broker.publish(event);
    ASSERT_EQ(flag, event.value);
}

TEST_F(EventBrokerTest, out_of_scope_handler_unregisters) {
    TestEvent1 event(42);
    uint32_t flag = 0;
    {
        EventHandlerFunction<TestEvent1> handler([&](TestEvent1 &event) { flag = event.value; });
        broker.registerHandler(&handler);
    }
    broker.publish(event);
    ASSERT_EQ(flag, 0);
}

TEST_F(EventBrokerTest, multiple_events) {
    TestEvent1 event1(1);
    TestEvent2 event2(2);
    uint32_t flag1 = 0, flag2 = 0, flag3 = 0;
    EventHandlerFunction<TestEvent1> handler1([&](TestEvent1 &event) { flag1 = event.value; });
    EventHandlerFunction<TestEvent1> handler2([&](TestEvent1 &event) { flag2 = event.value; });
    EventHandlerFunction<TestEvent2> handler3([&](TestEvent2 &event) { flag3 = event.value; });
    broker.registerHandler(&handler1);
    broker.registerHandler(&handler2);
    broker.registerHandler(&handler3);
    broker.publish(event1);
    broker.publish(event2);
    ASSERT_EQ(flag1, 1);
    ASSERT_EQ(flag2, 1);
    ASSERT_EQ(flag3, 2);
}