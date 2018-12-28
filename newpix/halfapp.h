#ifndef HALFAPP_H
#define HALFAPP_H

// STL
#include <queue>
#include <functional>

// PUT
#include <cxxutils/posix_helpers.h>
#include <specialized/mutex.h>
#include <specialized/eventbackend.h>


using vfunc = std::function<void()>;

class HalfApp
{
public:
  static void initialize(void) noexcept;

  static int exec(void) noexcept;
  static void process_events(milliseconds_t timeout = -1) noexcept;
  static void quit(int return_value = posix::success_response) noexcept;

private:
  static void step(void) noexcept;
  static void read(posix::fd_t fd, native_flags_t) noexcept;
  static posix::lockable<std::queue<vfunc>> ms_signal_queue;
  friend class Object;
};


#endif // HALFAPP_H
