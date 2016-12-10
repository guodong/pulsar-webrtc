#ifndef INTERACTING_EVENT_MANAGER_H_INCLUDED
#define INTERACTING_EVENT_MANAGER_H_INCLUDED

#include <X11/extensions/Xdamage.h>
#include "webrtc/base/copyonwritebuffer.h"

namespace pulsar
{
class EventObserver
{
public:
    virtual void OnEvent(const XEvent &event) = 0;

protected:
    virtual ~EventObserver() {}
};

class InteractingEventManager
{
public:
    InteractingEventManager();
    void RegisterObserver(EventObserver *observer);
    void Start();
    int GetDamageEventBase();
    Damage GetDamageHandle();
    Display* GetDisplay();
    Window GetRootWindow();
    void *ethread(void *data);
    static void *thread_helper(void *data);

protected:
    ~InteractingEventManager();

private:
    EventObserver *observer_;
    Display *display;
	int damage_event_base_ = -1;
	int damage_error_base_ = -1;
    Damage damage_handle_;
    Window root_window_;

};

}

#endif // INTERACTING_EVENT_MANAGER_H_INCLUDED
