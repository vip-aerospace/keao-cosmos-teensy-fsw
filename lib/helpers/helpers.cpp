#include <helpers.h>

namespace Helpers {
  void connect_serial_debug(long baud) {
#if defined(DEBUG_PRINT) || defined(DEBUG_PRINT_RAPID) ||                      \
    defined(DEBUG_PRINT_HEXDUMP)
    Serial.begin(baud);
    print_debug(MAIN, "Connected to Serial Console.");
#endif
  }

  void print_hexdump(Short_Name channel, const char *msg, uint8_t *src,
                     uint8_t size) {
#ifdef DEBUG_PRINT_HEXDUMP
    std::ostringstream oss;
    oss << msg;
    for (uint8_t i = 0; i < size; i++) {
      if (channel == Short_Name::PDU &&
          (src[i] == 0x20 || src[i] == 0x0D || src[i] == 0x0A)) {
        continue;
      }
      oss << std::hex << std::setw(2) << std::setfill('0')
          << static_cast<int>(src[i]) << " ";
    }
    print_debug(channel, oss.str().c_str());
#endif
  }
} // namespace Helpers
