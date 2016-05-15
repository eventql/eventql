#include <eventql/util/MonotonicClock.h>
#include <eventql/util/duration.h>
#include <eventql/util/exception.h>
#include <eventql/util/defines.h>
#include <eventql/util/sysconfig.h>

#if defined(STX_OS_DARWIN)
#include <mach/mach.h>
#include <mach/mach_time.h>
#endif

#if defined(STX_OS_DARWIN)
mach_timebase_info_data_t timebaseInfo;

STX_INIT static void tbi_initialize() {
  mach_timebase_info(&timebaseInfo);
}
#endif

MonotonicTime MonotonicClock::now() {
#if defined(HAVE_CLOCK_GETTIME) // POSIX realtime API
  timespec ts;
  int rv = clock_gettime(CLOCK_MONOTONIC, &ts);

  if (rv < 0)
    RAISE_ERRNO(kRuntimeError, "Could not retrieve clock.");

  uint64_t nanos =
      Duration::fromSeconds(ts.tv_sec).microseconds() * 1000 +
      ts.tv_nsec;

  return MonotonicTime(nanos);
#elif defined(STX_OS_DARWIN)
  uint64_t machTimeUnits = mach_absolute_time();
  uint64_t nanos = machTimeUnits * timebaseInfo.numer / timebaseInfo.denom;

  return MonotonicTime(nanos);
#else
  // TODO
  #error "MonotonicClock: Your platform is not implemented."
#endif
}
