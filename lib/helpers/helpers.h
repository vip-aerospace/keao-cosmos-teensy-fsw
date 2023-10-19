#ifndef _HELPERS_H
#define _HELPERS_H

#include "support/configCosmosKernel.h"
#include <Arduino.h>
#include <stdint.h>

extern unsigned long _heap_start;
extern unsigned long _heap_end;
extern char         *__brkval;

namespace Helpers {
  enum Short_Name : uint8_t {
    RFM23 = 1,
    PDU,
    RPI,
    MAIN,
    TEST,
  };

  void                         connect_serial_debug(long baud);
  template <typename Arg> void print_args(std::ostream &oss, const Arg &arg) {
#ifdef DEBUG_PRINT
    oss << arg;
#endif
  }

  template <typename Arg, typename... Args>
  void print_args(std::ostream &oss, const Arg &arg, const Args &...args) {
    oss << arg;
    print_args(oss, args...);
  }

  template <typename... Args>
  void print_debug(Short_Name channel, const Args &...args) {
#ifdef DEBUG_PRINT
    std::ostringstream oss;

    switch (channel) {
      case PDU:
        oss << "[PDU ] ";
        break;
      case RFM23:
        oss << "[RM23] ";
        break;
      case RPI:
        oss << "[R-PI] ";
        break;
      case MAIN:
        oss << "[MAIN] ";
        break;
      case TEST:
        oss << "[TEST] ";
        break;
      default:
        oss << "[????] ";
        break;
    }

    print_args(oss, args...);

    Serial.println(oss.str().c_str());
#endif
  }

  template <typename... Args>
  void print_debug_rapid(Short_Name channel, const Args &...args) {
#ifdef DEBUG_PRINT_RAPID
    print_debug(channel, args...);
#endif
  }

  void print_hexdump(Short_Name channel, const char *msg, uint8_t *src,
                     uint8_t size);

  void print_free_memory();
} // namespace Helpers

#endif // _HELPERS_H