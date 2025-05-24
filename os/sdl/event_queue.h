#ifndef OS_SDL_EVENT_QUEUE_INCLUDED
#define OS_SDL_EVENT_QUEUE_INCLUDED
#pragma once

#include "os/event.h"
#include "os/event_queue.h"

namespace os {

class EventQueueSDL final : public EventQueue {
public:
  void queueEvent(const Event& ev) override;
  void getEvent(Event& ev, double timeout) override;
  void clearEvents() override;
};

using EventQueueImpl = EventQueueSDL;

} // namespace os

#endif
