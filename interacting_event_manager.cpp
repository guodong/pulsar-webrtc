#include <pthread.h>
#include "interacting_event_manager.h"
#include "webrtc/system_wrappers/include/logging.h"
#include <X11/extensions/Xdamage.h>

namespace pulsar {
InteractingEventManager::InteractingEventManager()
{
    display = XOpenDisplay(NULL);
    if (!XDamageQueryExtension(display, &damage_event_base_,
        &damage_error_base_))
    {
        LOG(LS_INFO) << "X server does not support XDamage.";
        return;
    }
    root_window_ = RootWindow(display, DefaultScreen(display));
    damage_handle_ = XDamageCreate(display, root_window_, XDamageReportNonEmpty);
    if (!damage_handle_)
    {
        LOG(LS_ERROR) << "Unable to initialize XDamage.";
        return;
    }
    XSetWindowAttributes attr;
    attr.event_mask = SubstructureNotifyMask | PropertyChangeMask;
    XChangeWindowAttributes(display, root_window_, CWEventMask, &attr);
}

InteractingEventManager::~InteractingEventManager()
{

}

void InteractingEventManager::RegisterObserver(EventObserver *observer)
{
    observer_ = observer;
}

int InteractingEventManager::GetDamageEventBase()
{
    return damage_event_base_;
}

Damage InteractingEventManager::GetDamageHandle()
{
    return damage_handle_;
}

Display* InteractingEventManager::GetDisplay()
{
    return display;
}

Window InteractingEventManager::GetRootWindow()
{
    return root_window_;
}

void *InteractingEventManager::ethread(void *data)
{

    XInitThreads();
    XEvent xevt;

	while (1)
	{
	    XNextEvent(display, &xevt);

	    observer_->OnEvent(xevt);
	}
}

void *InteractingEventManager::thread_helper(void *data)
{
    return ((InteractingEventManager *)data)->ethread(NULL);
}

/** open event capture thread **/
void InteractingEventManager::Start()
{
    pthread_t tid;
    pthread_create(&tid, NULL, &InteractingEventManager::thread_helper, this);
}

}
