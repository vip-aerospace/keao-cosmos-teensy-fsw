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
  };

  void connect_serial_debug(long baud);
  void print_debug(Short_Name channel, const char *msg);
  void print_debug(Short_Name channel, const char *msg, u_int32_t iretn);
  void print_debug(Short_Name channel, const char *msg, int32_t iretn);
  void print_hexdump(Short_Name channel, const char *msg, uint8_t *src,
                     uint8_t size);
  void print_debug_rapid(Short_Name channel, const char *msg);
  void print_debug_rapid(Short_Name channel, const char *msg, u_int32_t iretn);

  void print_free_memory();
} // namespace Helpers

#endif // _HELPERS_H