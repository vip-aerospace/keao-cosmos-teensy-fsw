/**
 * @file helpers.h
 * @brief The header file for helper functions.
 *
 * This file contains declarations and definitions of helper functions used for
 * debugging the flight software.
 */
#ifndef _HELPERS_H
#define _HELPERS_H

/** @brief The number of milliseconds in a second. */
#define SECONDS 1000

#include "support/configCosmosKernel.h"
#include <Arduino.h>
#include <stdint.h>

extern unsigned long _heap_start;
extern unsigned long _heap_end;
extern char         *__brkval;

namespace Helpers {
  /** @brief Enumeration of channels calling helper functions. */
  enum Short_Name : uint8_t {
    RFM23 = 1,
    PDU,
    RPI,
    MAIN,
    TEST,
  };

  void connect_serial_debug(long baud);
  void print_hexdump(Short_Name channel, const char *msg, uint8_t *src,
                     uint8_t size);

  /**
   * @brief Base case of the recursive debug print.
   *
   * @tparam Arg The generic type of the argument to be appended to the stream.
   * @param oss The output stream containing the string to be printed.
   * @param arg The argument to be added to the ouput string.
   */
  template <typename Arg> void print_args(std::ostream &oss, const Arg &arg) {
#ifdef DEBUG_PRINT
    oss << arg;
#endif
  }

  /**
   * @brief Recursive case of the recursive debug print.
   *
   * This function, in combination with its base case version, recursively goes
   * through each argument passed into it, appends the first argument to the
   * output stream, then passes the remaining arguments to the next iteration of
   * the function. This repeats until the base case is reached and the complete
   * output stream is built.
   *
   * Practically, this function can be called with an arbirary number of
   * arguments. The arguments are appended to each other as if they were each
   * passed into std::ostream.
   *
   * For example,
   * `print_args("Hello World! int=", int);`
   * and
   * `print_args("This ", "is ", "a ", "test ");
   * are both valid uses of this function.
   *
   * @tparam Arg The generic type of the argument to be appended to the stream.
   * @tparam Args The generic type of the remaining arguments.
   * @param oss The output stream containing the string to be printed.
   * @param arg The argument to be added to the ouput string.
   * @param args The remaining arguments.
   */
  template <typename Arg, typename... Args>
  void print_args(std::ostream &oss, const Arg &arg, const Args &...args) {
    oss << arg;
    print_args(oss, args...);
  }

  /**
   * @brief The helper function used to print debug messages.
   *
   * @tparam Args The generic type of arguments to be appended to the stream.
   * @param channel The Short_Name of the channel calling this function.
   * @param args The arguments to be added to the output string.
   */
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

  /**
   * @brief Helper function used to print debug messages quickly.
   *
   * @tparam Args The generic type of arguments to be appended to the stream.
   * @param channel The Short_Name of the channel calling this function.
   * @param args The arguments to be added to the output string.
   */
  template <typename... Args>
  void print_debug_rapid(Short_Name channel, const Args &...args) {
#ifdef DEBUG_PRINT_RAPID
    print_debug(channel, args...);
#endif
  }
} // namespace Helpers

#endif // _HELPERS_H