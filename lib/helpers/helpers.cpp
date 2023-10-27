/**
 * @file helpers.cpp
 * @brief The helper functions.
 *
 * This file contains definitions of helper functions used for debugging the
 * flight software.
 */
#include <helpers.h>

namespace Helpers {
  /**
   * @brief Connects to a computer over USB Serial for debugging.
   *
   * @param baud The baud rate of the Serial connection to the host computer.
   */
  void connect_serial_debug(long baud) {
#if defined(DEBUG_PRINT) || defined(DEBUG_PRINT_RAPID) ||                      \
    defined(DEBUG_PRINT_HEXDUMP)
    Serial.begin(baud);
    print_debug(MAIN, "Connected to Serial Console.");
#endif
  }

  /**
   * @brief Helper function to print the hexdump of a region of memory.
   *
   * @param channel The Short_Name of the channel calling this function.
   * @param msg A message to accompany the hexdump.
   * @param src A pointer to the start of the region to be printed.
   * @param size The number of bytes to be printed.
   */
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
