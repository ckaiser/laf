#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_timer.h>

#include "os/sdl/event_queue.h"

namespace os {

void EventQueueSDL::queueEvent(const Event& ev) {
  // Transform to SDL event & push to SDL queue.
};

void EventQueueSDL::getEvent(Event& ev, double timeout)
{
  ASSERT(timeout == kWithoutTimeout); // Unsupported yet.

  SDL_Event sdlEvent;
  SDL_WaitEvent(&sdlEvent);

  switch (sdlEvent.type) {
    case SDL_EVENT_QUIT:                   ev.setType(Event::CloseApp); break;
    case SDL_EVENT_WINDOW_CLOSE_REQUESTED: ev.setType(Event::CloseWindow); break;
    case SDL_EVENT_WINDOW_RESIZED:         ev.setType(Event::ResizeWindow); break;
    default:                               ev.setType(Event::None);
  }
};

void EventQueueSDL::clearEvents()
{
  SDL_FlushEvents(SDL_EVENT_FIRST, SDL_EVENT_LAST);
};

} // namespace os
