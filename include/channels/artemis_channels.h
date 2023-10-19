/**
 * @file artemis_channels.h
 * @brief Definitions for Artemis channels on the satellite.
 *
 * This file contains the definition of the Channels namespace and the
 * declaration for each channel's namespace.
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
      void loop();
      void handle_queue();
      void regulate_temperature();
      void update_watchdog_timer();
    } // namespace PDU

    namespace RPI {
      void rpi_channel();
      void setup();
      void loop();
      void handle_queue();
      void send_to_pi();
      void receive_from_pi();
    } // namespace RPI

  } // namespace Channels
} // namespace Artemis

#endif // _ARTEMIS_CHANNELS_H
