#pragma once

#include <functional>

/** Simple event publishing framework
 *
 * All allocation is done at the subscriber end, so that it is possible to use 
 * the library with no heap allocation.
 *
 * An event may be any class inheriting from EventEx::Event.
 */
namespace EventEx {

struct Event {

};

struct Registerable {
    struct Owner {
        virtual void remove(Registerable *) = 0;
    };

    Registerable() : mOwner(nullptr) {}

    ~Registerable() {
        unregister();
    }

    void unregister() {
        if(mOwner) {
            mOwner->remove(this);
            mOwner = nullptr;
        }
    }

protected:
    Registerable *mNext;
    Owner *mOwner;
    std::type_info *mEventTypeId;

    friend class RegistrationList;
    friend class EventBroker;
};

/** An event handler serves as the targetnullptr for an event callback. It can be
 * derived and the functor overridden to define custom event handlers.
 * Alternatively, consider using EventHandlerFunction and a lambda.
 */
template <typename T>
struct EventHandler : public Registerable {
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
    EventHandlerFunction() {}

    EventHandlerFunction(std::function<void(T &)> f) : mFunc(f) {}

    void setFunction(std::function<void(T &)> f) {
        mFunc = f;
    }

    void operator()(T &event) override { mFunc(event); }

private:
    std::function<void(T &)> mFunc;
};

struct RegistrationList : public Registerable::Owner {
    struct iter {
        iter(Registerable *p) : mItem(p) {}

        Registerable & operator*() {
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
        Registerable *mItem;
    };

    RegistrationList() : mTop(nullptr) {}

    iter begin() { return iter(mTop); }
    iter end() { return iter(nullptr); }

    void push_back(Registerable *r) {
        r->mNext = nullptr;

        if(mTop == nullptr) {
            mTop = r;
        } else {
            Registerable *p = mTop;
            while(p->mNext != nullptr) {
                p = p->mNext;
            }
            p->mNext = r;
        }
    }

    void remove(Registerable *r) {
        if(mTop == r) {
            mTop = r->mNext;
        } else {
            Registerable *p = mTop;
            while(p->mNext != r && p->mNext != nullptr) {
                p = p->mNext;
            }
            if(p->mNext == r) {
                p->mNext = r->mNext;
            }
        }
    }

private:
    Registerable *mTop;
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
                static_cast<EventHandler<Event>*>(&handler)->dispatch(event);
            }
        }
    }

    template<typename T>
    void registerHandler(EventHandler<T> *handler) {
        handler->mOwner = &mEvents;
        handler->mEventTypeId = const_cast<std::type_info*>(&typeid(T));
        mEvents.push_back(handler);
    }

    void unregisterHandler(Registerable *reg) {
        mEvents.remove(reg);
    }

private:
    RegistrationList mEvents;
};

} // namespace EventEx