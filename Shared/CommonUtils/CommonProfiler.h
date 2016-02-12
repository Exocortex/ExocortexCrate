#ifndef __COMMON_PROFILER_H
#define __COMMON_PROFILER_H

#ifndef __COMMON_ALEMBIC_H
#error "Must include CommonAlembic.h before CommonProfiler.h"
#endif

#ifdef _MSC_VER

#include <windows.h>
#include <algorithm>

class HighResolutionTimer {
 public:
  // ctor
  HighResolutionTimer()
  {
    start_time.QuadPart = 0;
    frequency.QuadPart = 0;

    if (!QueryPerformanceFrequency(&frequency))
      throw std::runtime_error("Couldn't acquire frequency");

    restart();
  }

  // restart timer
  void restart()
  {
    if (!QueryPerformanceCounter(&start_time))
      throw std::runtime_error("Couldn't initialize start_time");
  }

  // return elapsed time in seconds
  double elapsed() const
  {
    LARGE_INTEGER now;
    if (!QueryPerformanceCounter(&now))
      throw std::runtime_error("Couldn't get current time");

    // QueryPerformanceCounter() workaround
    // http://support.microsoft.com/default.aspx?scid=kb;EN-US;q274323
    double d1 = double(now.QuadPart - start_time.QuadPart) / frequency.QuadPart;
    return d1;
  }

  // return estimated maximum value for elapsed()
  double elapsed_max() const
  {
    return (double((std::numeric_limits<LONGLONG>::max)()) -
            double(start_time.QuadPart)) /
           double(frequency.QuadPart);
  }

  // return minimum value for elapsed()
  double elapsed_min() const { return 1.0 / frequency.QuadPart; }
 private:
  LARGE_INTEGER start_time;
  LARGE_INTEGER frequency;
};

struct empty_logging_policy {
  static void on_start(std::string){};
  static void on_resume(std::string){};
  static void on_pause(std::string, double){};
  static void on_stop(std::string, double, bool, bool){};
};

struct default_logging_policy {
  static void on_start(std::string name)
  {
    // cerr << "starting profile " << name << endl;
  }
  static void on_resume(std::string name)
  {
    //   cerr << "resuming profile " << name << endl;
  }
  static void on_pause(std::string name)
  {
    //   cerr << "pausing profile " << name << endl;
  }
  static void on_stop(std::string name, double sec, bool underflow,
                      bool overflow)
  {
    //   cerr << "stopping profile " << name;
    //   cerr << " time elapsed = " << sec;
    //    if (underflow) cerr << " underflow occurred";
    //    if (overflow) cerr << " overflow occurred";
    //    cerr << endl;
  }
};

struct counted_sum : std::pair<int, double> {
  counted_sum() : std::pair<int, double>(0, 0) {}
  counted_sum(int x, double y) : std::pair<int, double>(x, y) {}
  double last;

  void operator+=(double x)
  {
    first++;
    second += x;
    last = x;
  }
};

typedef std::map<std::string, counted_sum> stats_map;

struct empty_stats_policy {
  static void on_stop(std::string name, double sec, bool underflow,
                      bool overflow)
  {
  }
  static void on_report() {}
};

struct default_stats_policy {
  static stats_map stats;

  static void on_stop(std::string name, double sec, bool underflow,
                      bool overflow)
  {
    // underflow and overflow are sticky.
    if (underflow) {
      stats[name] = counted_sum(-1, -1);
    }
    else if (overflow) {
      stats[name] = counted_sum(-2, -2);
    }
    else {
      stats[name] += sec;
    }
  }
  static void on_report()
  {
    ESS_LOG_WARNING(
        "PROFILER REPORT "
        ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>"
        ">>>>>>>>>>>>>>>>");
    ESS_LOG_WARNING("profile name," << '\t' << "total elapsed," << '\t'
                                    << "entry count," << '\t' << "average");

    for (stats_map::iterator i = stats.begin(); i != stats.end(); i++) {
      std::string sName = i->first;
      int nCount = i->second.first;
      double dTotal = i->second.second;
      double dAvg = dTotal / nCount;
      ESS_LOG_WARNING(sName << ",\t" << dTotal << ",\t" << nCount << ",\t"
                            << dAvg);
    }

    ESS_LOG_WARNING(
        "PROFILER REPORT "
        "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<"
        "<<<<<<<<<<<<<<<<");
  }
};

#ifndef PROFILING_OFF
template <typename logging_policy, typename stats_policy, typename timer_type>
class BasicProfiler {
 private:
  bool underflow;
  bool overflow;
  bool timing;
  std::string name;
  double elapsed;
  timer_type t;

 public:
  BasicProfiler(char const* s = "", bool autoStart = true)
      : underflow(false),
        overflow(false),
        timing(autoStart),
        name(s),
        elapsed(0.0)
  {
    logging_policy::on_start(name);
    if (autoStart) {
      t.restart();
    }
  }
  ~BasicProfiler()
  {
    if (timing) {
      stop();
    }
  }
  bool isTiming() const { return timing; }
  void stop()
  {
    if (timing) {
      double tmp = t.elapsed();
      if (tmp <= t.elapsed_min()) {
        underflow = true;
      }
      if (tmp >= t.elapsed_max()) {
        overflow = true;
      }
      tmp += elapsed;
      elapsed = 0.0;
      timing = false;
      logging_policy::on_stop(name, tmp, underflow, overflow);
      stats_policy::on_stop(name, tmp, underflow, overflow);
    }
  }
  void restart()
  {
    timing = true;
    elapsed = 0.0;
    // logging_policy::on_restart(name);
    t.restart();
  }
  void resume()
  {
    timing = true;
    // logging_policy::on_resume(name);
    t.restart();
  }
  void pause()
  {
    double tmp = t.elapsed();
    if (tmp <= t.elapsed_min()) {
      underflow = true;
    }
    if (tmp >= t.elapsed_max()) {
      overflow = true;
    }
    elapsed += tmp;
    timing = false;
    logging_policy::on_pause(name);
    t.pause();
  }
  static void generate_report() { stats_policy::on_report(); }
};
#else
template <typename logging_policy, typename stats_policy, typename timer_t>
class BasicProfiler {
 public:
  BasicProfiler(char const* s = "") {}
  void stop() {}
  void restart() {}
  void resume() {}
  void pause() {}
  static void generate_report() {}
};
#endif

struct sortableStatRecord {
  std::string sName;
  int nCount;
  double dTotal;
  double dAvg;
  double last;
};

class logging_stats_policy {
 public:
  stats_map stats;

  static void on_stop(std::string name, double sec, bool underflow,
                      bool overflow)
  {
    // underflow and overflow are sticky.
    if (underflow) {
      default_stats_policy::stats[name] = counted_sum(-1, -1);
    }
    else if (overflow) {
      default_stats_policy::stats[name] = counted_sum(-2, -2);
    }
    else {
      // Intel Parallel Studio says that the following line causes a memory leak
      default_stats_policy::stats[name] += sec;
    }
  }
  static void on_report() { generateReport(); }
  static bool statSortFunction(const sortableStatRecord& r1,
                               const sortableStatRecord& r2)
  {
    return r1.dTotal > r2.dTotal;
  }

  static void generateReport()
  {
    int padding = 14;
    ESS_LOG_WARNING(
        "PROFILER REPORT "
        ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>"
        ">>>>>>>>>>>>>>>>");
    std::stringstream strstream;
    strstream << std::setw(padding + 6) << std::setiosflags(std::ios::left)
              << "profile name," << std::setw(padding)
              << std::setiosflags(std::ios::right) << "average,"
              << std::setw(padding) << "last," << std::setw(padding)
              << "total elapsed," << std::setw(padding) << "entry count"
              << std::setw(padding);

    ESS_LOG_WARNING(strstream.str().c_str());

    std::vector<sortableStatRecord> records;
    records.reserve(default_stats_policy::stats.size());

    for (stats_map::iterator i = default_stats_policy::stats.begin();
         i != default_stats_policy::stats.end(); i++) {
      sortableStatRecord rec;
      rec.sName = i->first;
      rec.nCount = i->second.first;
      rec.dTotal = i->second.second;
      rec.dAvg = rec.dTotal / rec.nCount;
      rec.last = i->second.last;

      records.push_back(rec);
    }

    std::sort(records.begin(), records.end(), statSortFunction);

    for (int i = 0; i < records.size(); i++) {
      std::stringstream strstream2;
      strstream2 << std::setw(padding + 6) << std::setiosflags(std::ios::left)
                 << records[i].sName << "," << std::setw(50)
                 << std::setiosflags(std::ios::right) << records[i].dAvg << ","
                 << std::setw(padding) << records[i].last << ","
                 << std::setw(padding) << records[i].dTotal << ","
                 << std::setw(padding) << records[i].nCount
                 << std::setw(padding);

      ESS_LOG_WARNING(strstream2.str().c_str());
    }
    ESS_LOG_WARNING(
        "PROFILER REPORT "
        "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<"
        "<<<<<<<<<<<<<<<<");
    default_stats_policy::stats.clear();
  }
};

typedef BasicProfiler<empty_logging_policy,  // don't log events
                      logging_stats_policy, HighResolutionTimer>
    Profiler;

#define ESS_PROFILING

#endif  // _MSC_VER

#ifdef ESS_PROFILING
#pragma message("EXOCORTEX: ESS_PROFILING defined, profiling enabled.")
#define ESS_PROFILE_SCOPE(a) Profiler profiler(a);
#define ESS_PROFILE_REPORT() logging_stats_policy::generateReport();
#else
#pragma message("EXOCORTEX: ESS_PROFILING not defined, profiling disabled.")
#define ESS_PROFILE_SCOPE(a)
#define ESS_PROFILE_REPORT()
#endif

#define ESS_PROFILE_FUNC() ESS_PROFILE_SCOPE(__FUNCTION__)

#endif  // __COMMON_PROFILER_H