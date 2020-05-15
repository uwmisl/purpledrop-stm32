#pragma once

/** Simple event publishing framework
 * 
 * All allocation is done at the subscriber end, so that it is possible to the
 * library with no heap allocation. 
 * 
 * An event may be any class inheriting from EventEx::Event. 
 */
namespace EventEx {

struct Event {
};

struct EventRegistrationList;


/** An event handler serves as the target for an event callback. It can be 
 * derived and the functor overridden to define custom event handlers. 
 * Alternatively, consider using EventHandlerFunction and a lambda. 
 */
template <typename T>
struct EventHandler {
    EventHandler() {
        static_assert(
            std::is_base_of<Event, T>::value,
            "Error instantiating EventHandler<T>: T must be a base class of Event"
        );
    }

    virtual void operator ()(T &event) = 0;

    void dispatch(Event &event) {
        (*this)(dynamic_cast<T &>(event));
    }

private:

};

/** Utility class for creating an EventHandler from a lambda. 
 * 
 * For example, to create a handler that wires the event to a member method: 
 * 
 *      EventHandlerFunction<SomeEvent> handler([&](SomeEvent &e) { this->HandleSomeEvent(e); })
 * 
 * As I understand it, creating std::functions *can* allocate but almost certainly
 * will not do so when only storing a single pointer (e.g. `this`) with the lambda,
 * even if this isn't strictly guaranteed by C++17.
 */
template <typename T>
struct EventHandlerFunction : public EventHandler<T> {
    EventHandlerFunction(std::function<void(T &)> f) : EventHandler<T>(), mFunc(f) {}

    void operator()(T &event) { mFunc(event); }

private:
    const std::function<void(T &)> mFunc;
};


struct EventRegistration {
    struct iter {
        iter(EventRegistration *p) : mItem(p) {}

        EventRegistration & operator*() {
            return *mItem;
        }
        
        bool operator!=(const iter &rhs) { return mItem != rhs.mItem; }

        iter & operator++() {
            mItem = mItem->mNext;
            return *this;
        }

        iter &operator++(int) {
            mItem = mItem->mNext;
            return *this;
        }

    private:
        EventRegistration *mItem;
    };

    struct Owner { 
        virtual void remove(EventRegistration *) = 0;
    };

    template<typename T>
    EventRegistration(EventHandler<T> *handler) : mEventHandler(handler), mOwner(NULL) {}

    EventRegistration() : mEventHandler(NULL), mOwner(NULL) {}

    ~EventRegistration() {
        unregister();
    }

    void * handler() { return mEventHandler; }

    void unregister() {
        if(mOwner) {
            mOwner->remove(this);
            mOwner = NULL;
        }
    }

private:
    void * mEventHandler;
    EventRegistration *mNext;
    Owner *mOwner;
    std::type_info *mEventTypeId;

    friend class EventRegistrationList;
    friend class EventBroker;
};

struct EventRegistrationList : public EventRegistration::Owner {
    EventRegistrationList() : mTop(NULL) {}

    EventRegistration::iter begin() { return EventRegistration::iter(mTop); }
    EventRegistration::iter end() { return EventRegistration::iter(NULL); }

    void push_back(EventRegistration *r) {
        r->mNext = NULL;
        
        if(mTop == NULL) {
            mTop = r;
        } else {
            EventRegistration *p = mTop;
            while(p->mNext != NULL) {
                p = p->mNext;
            }
            p->mNext = r;
        }
    }

    void remove(EventRegistration *r) {
        if(mTop == r) {
            mTop = r->mNext;
        } else {
            EventRegistration *p = mTop;
            while(p->mNext != r && p->mNext != NULL) {
                p = p->mNext;
            }
            if(p->mNext == r) {
                p->mNext = r->mNext;
            }
        }
    }

private:
    EventRegistration *mTop;
};

struct EventBroker {

    ~EventBroker() {
        for(auto event : mEvents) {
            event.unregister();
        }
    }

    template<typename T>
    void publish(T &event) {
        for(auto &handler : mEvents) {
            if(*handler.mEventTypeId == typeid(event)) {
                static_cast<EventHandler<Event>*>(handler.handler())->dispatch(event);
            }
        }
    }

    template<typename T>
    void registerHandler(EventRegistration *reg, EventHandler<T> *handler) {
        reg->mOwner = &mEvents;
        reg->mEventHandler = (void*)handler;
        reg->mEventTypeId = const_cast<std::type_info*>(&typeid(T));
        mEvents.push_back(reg);
    }

    void unregisterHandler(EventRegistration *reg) {
        mEvents.remove(reg);
    }

private:
    EventRegistrationList mEvents;
};

} // namespace EventEx