/**
 * @file artemis_channels.h
 * @brief Declarations of Artemis channels on the satellite.
 *
 * This file contains the definition of the Channels namespace and the
 * declarations of each channel's namespace.
 */
#ifndef _ARTEMIS_CHANNELS_H
#define _ARTEMIS_CHANNELS_H

#include "config/artemis_defs.h"

namespace Artemis {
  /**
   * @brief The channels on the satellite.
   *
   * This namespace defines all the channels on the satellite. Each channel is
   * run in parallel as a thread by quickly switching between them.
   */
  namespace Channels {
    /** @brief Enumeration of channel ID. */
    enum Channel_ID : uint8_t {
      RFM23_CHANNEL = 1,
      PDU_CHANNEL,
      RPI_CHANNEL,
      TEST_CHANNEL,
    };

    namespace RFM23 {
      void rfm23_channel();
      void setup();
      void loop();
      void handle_queue();
      void receive_from_radio();
    } // namespace RFM23

    namespace PDU {
      void pdu_channel();
      void setup();
      void enableRFM23Radio();
      void deploy();
      void deploy_burn_wire();
      void loop();
      void handle_queue();
      void test_communicating_with_pdu();
      void set_switch_on_pdu();
      void regulate_temperature();
      void update_watchdog_timer();
    } // namespace PDU

    namespace RPI {
      void rpi_channel();
      void setup();
      void loop();
      void handle_queue();
      void shut_down_pi();
      void send_to_pi();
      void receive_from_pi();
    } // namespace RPI

    namespace TEST {
      void test_channel();
      void setup();
      void loop();
      void turn_on_rpi();
      void turn_off_rpi();
      void rpi_take_picture_from_teensy();
      void rpi_take_picture_from_ground();
      void pdu_switch_all_on();
      void pdu_switch_status();
      void rfm23_transmit();
      void report_threads_status();
      void report_memory_usage();
      void report_queue_size();
    } // namespace TEST

  } // namespace Channels
} // namespace Artemis

#endif // _ARTEMIS_CHANNELS_H
