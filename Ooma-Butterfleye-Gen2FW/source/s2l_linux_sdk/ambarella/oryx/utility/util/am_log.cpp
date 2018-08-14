/*******************************************************************************
 * am_log.cpp
 *
 * Histroy:
 *  2012-2-29 2012 - [ypchang] created file
 *
 * Copyright (c) 2016 Ambarella, Inc.
 *
 * This file and its contents ("Software") are protected by intellectual
 * property rights including, without limitation, U.S. and/or foreign
 * copyrights. This Software is also the confidential and proprietary
 * information of Ambarella, Inc. and its licensors. You may not use, reproduce,
 * disclose, distribute, modify, or otherwise prepare derivative works of this
 * Software or any portion thereof except pursuant to a signed license agreement
 * or nondisclosure agreement with Ambarella, Inc. or its authorized affiliates.
 * In the absence of such an agreement, you agree to promptly notify and return
 * this Software to Ambarella, Inc.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF NON-INFRINGEMENT,
 * MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL AMBARELLA, INC. OR ITS AFFILIATES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; COMPUTER FAILURE OR MALFUNCTION; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************/

#include "am_base_include.h"
#include "am_define.h"
#include "am_log.h"

#include <errno.h>
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>
#include <syslog.h>

#include <atomic>

#define empty_module_name(x)   (x ? x : "AMLog")
#define empty_level_str(x)     (x ? x : "error")
#define empty_target_str(x)    (x ? x : "stderr")

std::atomic_flag isInitialized = ATOMIC_FLAG_INIT;
std::atomic_flag lock = ATOMIC_FLAG_INIT;

static const int level_to_syslog[] =
{
 LOG_INFO,    /* PRINT  */
 LOG_ERR,     /* ERROR  */
 LOG_WARNING, /* WARN   */
 LOG_INFO,    /* STAT   */
 LOG_NOTICE,  /* NOTICE */
 LOG_INFO,    /* INFO   */
 LOG_DEBUG    /* DEBUG  */
};

class LogFile
{
  public:
    LogFile() :logfd(-1){}
    virtual ~LogFile() {set_log_fd(-1);}
    void set_log_fd(int fd) {
      if (fd >= 0) {
        if (logfd >= 0) {
          close(logfd);
        }
        logfd = fd;
      } else if (logfd >= 0) {
        close(logfd);
        logfd = -1;
        sync();
      }
    }
    int fd() {return logfd;}

  private:
    int logfd;
};

static LogFile       logfile;
static AM_LOG_LEVEL  logLevel;
static AM_LOG_TARGET logTarget;
static bool          logTimeStamp;
static bool          logClassName;

static inline void get_log_level()
{
  logLevel = AM_LOG_LEVEL_WARN;
  const char *levelstr = empty_level_str(secure_getenv(AM_LEVEL_ENV_VAR));
  int         level    = atoi(levelstr);

  switch(level) {
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6: logLevel = AM_LOG_LEVEL(level); break;
    case 0: {
      if (is_str_equal(levelstr, "print") || is_str_equal(levelstr, "0")) {
        logLevel = AM_LOG_LEVEL_PRINT;
      } else if (is_str_equal(levelstr, "error")) {
        logLevel = AM_LOG_LEVEL_ERROR;
      } else if (is_str_equal(levelstr, "warn")) {
        logLevel = AM_LOG_LEVEL_WARN;
      } else if (is_str_equal(levelstr, "stat")) {
        logLevel = AM_LOG_LEVEL_STAT;
      } else if (is_str_equal(levelstr, "notice")) {
        logLevel = AM_LOG_LEVEL_NOTICE;
      } else if (is_str_equal(levelstr, "info")) {
        logLevel = AM_LOG_LEVEL_INFO;
      } else if (is_str_equal(levelstr, "debug")) {
        logLevel = AM_LOG_LEVEL_DEBUG;
      }
    }break;
    default: break;
  }
}

static inline void get_log_target()
{
  logTarget = AM_LOG_TARGET_NULL;
  const char *target = empty_target_str(secure_getenv(AM_TARGET_ENV_VAR));

  if (is_str_equal(target, "stderr")) {
    logTarget = AM_LOG_TARGET_STDERR;
  } else if (is_str_equal(target, "syslog")) {
    logTarget = AM_LOG_TARGET_SYSLOG;
  } else if (is_str_start_with(target, "file:")) {
    int fd = -1;
    char logname[512] = {0};
    time_t current = time(nullptr);
    struct tm* tmstruct = gmtime(&current);
    const char *filename = strstr(target, ":");
    const char* ext = strrchr(&filename[1], '.');
    if (ext) {
      snprintf(logname, (ext - &filename[1]) + 1, "%s", &filename[1]);
      sprintf(logname + strlen(logname), "-%04d%02d%02d%02d%02d%02d",
              tmstruct->tm_year + 1900, tmstruct->tm_mon + 1,
              tmstruct->tm_mday, tmstruct->tm_hour,
              tmstruct->tm_min,  tmstruct->tm_sec);
      sprintf(logname + strlen(logname), "%s", ext);
    } else {
      sprintf(logname, "%s-%04d%02d%02d%02d%02d%02d.log",
              &filename[1],
              tmstruct->tm_year + 1900,
              tmstruct->tm_mon + 1,
              tmstruct->tm_mday,
              tmstruct->tm_hour,
              tmstruct->tm_min,
              tmstruct->tm_sec);
    }
    if ((fd = open(logname,
                   O_WRONLY|O_TRUNC|O_CREAT,
                   S_IRUSR|S_IWUSR)) >= 0) {
      logfile.set_log_fd(fd);
      logTarget = AM_LOG_TARGET_FILE;
    } else {
      fprintf(stderr,
              "Failed to open file %s: %s(Reset log target to stderr)",
              &filename[1], strerror(errno));
      logTarget = AM_LOG_TARGET_STDERR;
    }
  } else if (is_str_equal(target, "null")) {
    logTarget = AM_LOG_TARGET_NULL;
  }
}

static inline void get_boolean_setting(const char *env,
                                       bool &result,
                                       bool def = false)
{
  const char *target = secure_getenv(env);

  result = target ? (is_str_equal(target, "Yes")  ||
                     is_str_equal(target, "True") ||
                     is_str_equal(target, "1")    ||
                     is_str_equal(target, "On")) : def;
}

static inline void init()
{
  if (AM_UNLIKELY(!isInitialized.test_and_set())) {
      get_log_level();
      get_log_target();
      get_boolean_setting(AM_TIMESTAMP_ENV_VAR, logTimeStamp, true);
      get_boolean_setting(AM_CLASS_ENV_VAR, logClassName, false);
  }
}

static inline const char *get_timestamp_ms()
{
  char date_fmt[20] = {0};
  char date_ms[8] = {0};
  struct timeval tv;
  struct tm now_tm;
  int now_us;
  time_t now_sec;
  static char timestring[128] = {0};
  gettimeofday(&tv, nullptr);
  now_sec = tv.tv_sec;
  now_us = tv.tv_usec;
  localtime_r(&now_sec, &now_tm);

  strftime(date_fmt, sizeof(date_fmt), "%Y-%m-%d %H:%M:%S", &now_tm);
  snprintf(date_ms, sizeof(date_ms), "%06d", now_us);
  int ret = snprintf(timestring, sizeof(timestring), "%s.%s",
                     date_fmt, date_ms);
  if (ret < (int)sizeof(timestring)) {
    char *end = strstr(timestring, "\n");
    timestring[ret] = '\0';
    if (end) {
      *end = '\0';
    }
  } else {
    timestring[sizeof(timestring) - 1] = '\0';
  }
  return timestring;
}

static inline void am_level_logv(const char  *module,
                                 AM_LOG_LEVEL level,
                                 const char  *format,
                                 va_list      vlist)
{
  char text[2*1024] = {0};
  char str[4*1024] = {0};
  int len = vsnprintf(text, sizeof(text), format, vlist);
  int end = ((uint32_t)len < sizeof(text)) ? len : (sizeof(text) - 1);
  text[end] = '\0';

  while(lock.test_and_set(std::memory_order_acquire)); /* Spin lock */
  logTimeStamp ?
      snprintf(str, sizeof(str) - 1, "[" B_BLUE("%s") "]%s\n",
               get_timestamp_ms(), text) :
      snprintf(str, sizeof(str) - 1, "%s\n", text);

  switch(logTarget) {
    case AM_LOG_TARGET_STDERR: {
      fprintf(stderr, "%s", str);
    }break;
    case AM_LOG_TARGET_SYSLOG: {
      openlog(empty_module_name(module), LOG_PID, LOG_USER);
      syslog(level_to_syslog[level], "%s\n", text);
    }break;
    case AM_LOG_TARGET_FILE: {
      if ((write(logfile.fd(), str, strlen(str)) < 0)) {
        fprintf(stderr, "%s: %s: %d\n",
                B_RED("Failed writing logs to file. Redirect log to console"),
                strerror(errno), logfile.fd());
        logfile.set_log_fd(-1);
        logTarget = AM_LOG_TARGET_STDERR;
        fprintf(stderr, "%s", str);
      }
    }break;
    case AM_LOG_TARGET_NULL:
    default: break;
  }
  lock.clear(std::memory_order_release);
}

static inline void class_name(char *name, const char *pretty_func)
{
  char temp[strlen(pretty_func) + 1];
  char *end = nullptr;
  snprintf(temp, sizeof(temp) - 1, "%s", pretty_func);
  temp[sizeof(temp) - 1] = '\0';
  end = strstr(temp, "::");
  if (AM_LIKELY(end)) { /* Found */
    char *classname = nullptr;
    *end = '\0';
    classname = strrchr(temp, (int)' ');
    sprintf(name, "%s", classname ? &classname[1] : temp);
  } else { /* Not Found */
    sprintf(name, "Function");
  }
}

static inline void function_name(char *name, const char *pretty_func)
{
  char buff[strlen(pretty_func) + 1];
  char    *start = nullptr;
  char      *end = nullptr;
  snprintf(buff, sizeof(buff) - 1, "%s", pretty_func);
  buff[sizeof(buff) - 1] = '\0';
  start = strstr(buff, "::");
  end = strstr(buff, "(");
  if (AM_LIKELY(end && (end <= (buff + sizeof(buff) - 3)))) {
    end[1] = ')';
    end[2] = '\0';
  }
  sprintf(name, "%s", start ? &start[2] : buff);
}

static inline void file_name(char *name, const char *file)
{
  char temp[strlen(file) + 1];
  char *start = nullptr;
  snprintf(temp, sizeof(temp), "%s", file);
  start = strrchr(temp, (int)'/');
  sprintf(name, "%s", start ? &start[1] : temp);
}

bool set_log_level(const char *level, bool change)
{
  return (0 == setenv(AM_LEVEL_ENV_VAR, level, change ? 1 : 0));
}

bool set_log_target(const char *target, bool change)
{
  return (0 == setenv(AM_TARGET_ENV_VAR, target, change ? 1 : 0));
}

bool set_timestamp_enabled(bool enable)
{
  return (0 == setenv(AM_TIMESTAMP_ENV_VAR, enable ? "Yes" : "No", 1));
}

void close_log_file()
{
  logfile.set_log_fd(-1);
}

void am_debug(const char *pretty_func, const char *_file,
              int                line, const char *_format, ...)
{
  init();
  if (AM_UNLIKELY(AM_LOG_LEVEL_DEBUG <= logLevel)) {
    char text[4*1024]   = {0};
    char format[2*1024] = {0};
    char module[strlen(pretty_func) + 1];
    char func[strlen(pretty_func) + 1];
    char file[strlen(_file) + 1];
    va_list vlist;

    function_name(func, pretty_func);
    class_name(module, pretty_func);
    file_name(file, _file);
    snprintf(format, sizeof(format), "%s", _format);
    if (format[strlen(format) - 1] == '\n') {
      //Eat trailing \n, \n will be added automatically!
      format[strlen(format) - 1] = '\0';
    }
    switch(logTarget) {
      case AM_LOG_TARGET_SYSLOG: {
        sprintf(text, "[%6s][%4d: %s: %s]: %s",
                "DEBUG", line, file, func, format);
      }break;
      case AM_LOG_TARGET_FILE: {
        if (logClassName) {
          sprintf(text, "[%28s][%6s][%4d: %s: %s]: %s",
                  module, "DEBUG", line, file, func, format);
        } else {
          sprintf(text, "[%6s][%4d: %s: %s]: %s",
                  "DEBUG", line, file, func, format);
        }
      }break;
      default: {
        if (logClassName) {
          sprintf(text, "[" B_GREEN("%28s") "][" B_BLUE("%6s") "]" \
                  CYAN("[")B_WHITE("%4d")CYAN(": %s: %s]: ")BLUE("%s"),
                  module, "DEBUG", line, file, func, format);
        } else {
          sprintf(text, "[" B_BLUE("%6s") "]" \
                  CYAN("[")B_WHITE("%4d")CYAN(": %s: %s]: ")BLUE("%s"),
                  "DEBUG", line, file, func, format);
        }
      }break;
    }
    va_start(vlist, _format);
    am_level_logv(module, AM_LOG_LEVEL_DEBUG, text, vlist);
    va_end(vlist);
  }
}

void am_info(const char *pretty_func, const char *_file,
             int                line, const char *_format, ...)
{
  init();
  if (AM_UNLIKELY(AM_LOG_LEVEL_INFO <= logLevel)) {
    char text[4*1024]   = {0};
    char format[2*1024] = {0};
    char module[strlen(pretty_func) + 1];
    char func[strlen(pretty_func) + 1];
    char file[strlen(_file) + 1];
    va_list vlist;

    function_name(func, pretty_func);
    class_name(module, pretty_func);
    file_name(file, _file);
    snprintf(format, sizeof(format), "%s", _format);
    if (format[strlen(format) - 1] == '\n') {
      //Eat trailing \n, \n will be added automatically!
      format[strlen(format) - 1] = '\0';
    }
    switch(logTarget) {
      case AM_LOG_TARGET_SYSLOG: {
        sprintf(text, "[%6s][%4d: %s: %s]: %s",
                "INFO", line, file, func, format);
      }break;
      case AM_LOG_TARGET_FILE: {
        if (logClassName) {
          sprintf(text, "[%28s][%6s][%4d: %s: %s]: %s",
                  module, "INFO", line, file, func, format);
        } else {
          sprintf(text, "[%6s][%4d: %s: %s]: %s",
                  "INFO", line, file, func, format);
        }
      }break;
      default: {
        if (logClassName) {
          sprintf(text, "[" B_GREEN("%28s") "][" B_GREEN("%6s") "]" \
                  CYAN("[")B_WHITE("%4d")CYAN(": %s: %s]: ")GREEN("%s"),
                  module, "INFO", line, file, func, format);
        } else {
          sprintf(text, "[" B_GREEN("%6s") "]" \
                  CYAN("[")B_WHITE("%4d")CYAN(": %s: %s]: ")GREEN("%s"),
                  "INFO", line, file, func, format);
        }
      }break;
    }
    va_start(vlist, _format);
    am_level_logv(module, AM_LOG_LEVEL_INFO, text, vlist);
    va_end(vlist);
  }
}

void am_notice(const char *pretty_func, const char *_file,
               int                line, const char *_format, ...)
{
  init();
  if (AM_UNLIKELY(AM_LOG_LEVEL_NOTICE <= logLevel)) {
    char text[2*1024]   = {0};
    char format[2*1024] = {0};
    char module[strlen(pretty_func) + 1];
    char func[strlen(pretty_func) + 1];
    char file[strlen(_file) + 1];
    va_list vlist;

    function_name(func, pretty_func);
    class_name(module, pretty_func);
    file_name(file, _file);
    snprintf(format, sizeof(format), "%s", _format);
    if (format[strlen(format) - 1] == '\n') {
      //Eat trailing \n, \n will be added automatically!
      format[strlen(format) - 1] = '\0';
    }
    switch(logTarget) {
      case AM_LOG_TARGET_SYSLOG: {
        sprintf(text, "[%6s][%4d: %s: %s]: %s",
                "NOTICE", line, file, func, format);
      }break;
      case AM_LOG_TARGET_FILE: {
        if (logClassName) {
          sprintf(text, "[%28s][%6s][%4d: %s: %s]: %s",
                  module, "NOTICE", line, file, func, format);
        } else {
          sprintf(text, "[%6s][%4d: %s: %s]: %s",
                  "NOTICE", line, file, func, format);
        }
      }break;
      default: {
        if (logClassName) {
          sprintf(text, "[" B_GREEN("%28s") "][" B_MAGENTA("%6s") "]" \
                  CYAN("[")B_WHITE("%4d")CYAN(": %s: %s]: ")MAGENTA("%s"),
                  module, "NOTICE", line, file, func, format);
        } else {
          sprintf(text, "[" B_MAGENTA("%6s") "]" \
                  CYAN("[")B_WHITE("%4d")CYAN(": %s: %s]: ")MAGENTA("%s"),
                  "NOTICE", line, file, func, format);
        }
      }break;
    }
    va_start(vlist, _format);
    am_level_logv(module, AM_LOG_LEVEL_NOTICE, text, vlist);
    va_end(vlist);
  }
}

void am_stat(const char *pretty_func, const char *_file,
             int                line, const char *_format, ...)
{
  init();
  if (AM_UNLIKELY(AM_LOG_LEVEL_STAT <= logLevel)) {
    char text[2*1024]   = {0};
    char format[2*1024] = {0};
    char module[strlen(pretty_func) + 1];
    char func[strlen(pretty_func) + 1];
    char file[strlen(_file) + 1];
    va_list vlist;

    function_name(func, pretty_func);
    class_name(module, pretty_func);
    file_name(file, _file);
    snprintf(format, sizeof(format), "%s", _format);
    if (format[strlen(format) - 1] == '\n') {
      //Eat trailing \n, \n will be added automatically!
      format[strlen(format) - 1] = '\0';
    }

    switch(logTarget) {
      case AM_LOG_TARGET_SYSLOG: {
        sprintf(text, "[%6s][%4d: %s: %s]: %s",
                "STAT", line, file, func, format);
      }break;
      case AM_LOG_TARGET_FILE: {
        if (logClassName) {
          sprintf(text, "[%28s][%6s][%4d: %s: %s]: %s",
                  module, "STAT", line, file, func, format);
        } else {
          sprintf(text, "[%6s][%4d: %s: %s]: %s",
                  "STAT", line, file, func, format);
        }
      }break;
      default: {
        if (logClassName) {
          sprintf(text, "[" B_GREEN("%28s") "][" B_CYAN("%6s") "]" \
                  CYAN("[")B_WHITE("%4d")CYAN(": %s: %s]: ")B_WHITE("%s"),
                  module, "STAT", line, file, func, format);
        } else {
          sprintf(text, "[" B_CYAN("%6s") "]" \
                  CYAN("[")B_WHITE("%4d")CYAN(": %s: %s]: ")B_WHITE("%s"),
                  "STAT", line, file, func, format);
        }
      }break;
    }
    va_start(vlist, _format);
    am_level_logv(module, AM_LOG_LEVEL_STAT, text, vlist);
    va_end(vlist);
  }
}

void am_warn(const char *pretty_func, const char *_file,
             int                line, const char *_format, ...)
{
  init();
  if (AM_UNLIKELY(AM_LOG_LEVEL_WARN <= logLevel)) {
    char text[2*1024]   = {0};
    char format[2*1024] = {0};
    char module[strlen(pretty_func) + 1];
    char func[strlen(pretty_func) + 1];
    char file[strlen(_file) + 1];
    va_list vlist;

    function_name(func, pretty_func);
    class_name(module, pretty_func);
    file_name(file, _file);
    snprintf(format, sizeof(format), "%s", _format);
    if (format[strlen(format) - 1] == '\n') {
      //Eat trailing \n, \n will be added automatically!
      format[strlen(format) - 1] = '\0';
    }
    switch(logTarget) {
      case AM_LOG_TARGET_SYSLOG: {
        sprintf(text, "[%6s][%4d: %s: %s]: %s",
                "WARN", line, file, func, format);
      }break;
      case AM_LOG_TARGET_FILE: {
        if (logClassName) {
          sprintf(text, "[%28s][%6s][%4d: %s: %s]: %s",
                  module, "WARN", line, file, func, format);
        } else {
          sprintf(text, "[%6s][%4d: %s: %s]: %s",
                  "WARN", line, file, func, format);
        }
      }break;
      default:{
        if (logClassName) {
          sprintf(text, "[" B_GREEN("%28s") "][" B_YELLOW("%6s") "]" \
                  CYAN("[")B_WHITE("%4d")CYAN(": %s: %s]: ")YELLOW("%s"),
                  module, "WARN", line, file, func, format);
        } else {
          sprintf(text, "[" B_YELLOW("%6s") "]" \
                  CYAN("[")B_WHITE("%4d")CYAN(": %s: %s]: ")YELLOW("%s"),
                  "WARN", line, file, func, format);
        }
      }break;
    }
    va_start(vlist, _format);
    am_level_logv(module, AM_LOG_LEVEL_WARN, text, vlist);
    va_end(vlist);
  }
}

void am_error(const char *pretty_func, const char *_file,
              int                line, const char *_format, ...)
{
  init();
  if (AM_UNLIKELY(AM_LOG_LEVEL_ERROR <= logLevel)) {
    char text[2*1024]   = {0};
    char format[2*1024] = {0};
    char module[strlen(pretty_func) + 1];
    char func[strlen(pretty_func) + 1];
    char file[strlen(_file) + 1];
    va_list vlist;

    function_name(func, pretty_func);
    class_name(module, pretty_func);
    file_name(file, _file);
    snprintf(format, sizeof(format), "%s", _format);
    if (format[strlen(format) - 1] == '\n') {
      //Eat trailing \n, \n will be added automatically!
      format[strlen(format) - 1] = '\0';
    }
    switch(logTarget) {
      case AM_LOG_TARGET_SYSLOG: {
        sprintf(text, "[%6s][%4d: %s: %s]: %s",
                "ERROR", line, file, func, format);
      }break;
      case AM_LOG_TARGET_FILE: {
        if (logClassName) {
          sprintf(text, "[%28s][%6s][%4d: %s: %s]: %s",
                  module, "ERROR", line, file, func, format);
        } else {
          sprintf(text, "[%6s][%4d: %s: %s]: %s",
                  "ERROR", line, file, func, format);
        }
      }break;
      default: {
        if (logClassName) {
          sprintf(text, "[" B_GREEN("%28s") "][" B_RED("%6s") "]" \
                  CYAN("[")B_WHITE("%4d")CYAN(": %s: %s]: ")RED("%s"),
                  module, "ERROR", line, file, func, format);
        } else {
          sprintf(text, "[" B_RED("%6s") "]" \
                  CYAN("[")B_WHITE("%4d")CYAN(": %s: %s]: ")RED("%s"),
                  "ERROR", line, file, func, format);
        }
      }break;
    }
    va_start(vlist, _format);
    am_level_logv(module, AM_LOG_LEVEL_ERROR, text, vlist);
    va_end(vlist);
  }
}

void am_print(const char *_format, ...)
{
  init();
  if (AM_UNLIKELY(AM_LOG_LEVEL_PRINT <= logLevel)) {
    char text[2*1024]   = {0};
    char format[2*1024] = {0};
    va_list vlist;

    snprintf(format, sizeof(format), "%s", _format);
    if (format[strlen(format) - 1] == '\n') {
      //Eat trailing \n, \n will be added automatically!
      format[strlen(format) - 1] = '\0';
    }
    switch(logTarget) {
      case AM_LOG_TARGET_SYSLOG: {
        sprintf(text, "%s", format);
      }break;
      case AM_LOG_TARGET_FILE: {
        sprintf(text, "%s", format);
      }break;
      default:{
        sprintf(text, B_WHITE("%s"), format);
      }break;
    }
    va_start(vlist, _format);
    am_level_logv(nullptr, AM_LOG_LEVEL_PRINT, text, vlist);
    va_end(vlist);
  }
}
