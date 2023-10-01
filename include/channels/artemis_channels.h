#ifndef _ARTEMIS_CHANNELS_H
#define _ARTEMIS_CHANNELS_H

#include "config/artemis_defs.h"

namespace Artemis {
  namespace Channels {
    // Channel IDs
    enum Channel_ID : uint8_t {
      RFM23_CHANNEL = 1,
      PDU_CHANNEL,
      RPI_CHANNEL,
    };

    namespace RFM23 {
      void rfm23_channel();
      void setup();
      void loop();
    } // namespace RFM23

    namespace PDU {
      void pdu_channel();
      void setup();
      void deploy();
      void loop();
      void handle_pdu_queue();
      void regulate_temperature();
      void update_watchdog_timer();
    } // namespace PDU

    namespace RPI {
      void rpi_channel();
      void setup();
      void loop();
      void sendToPi();
    } // namespace RPI

  } // namespace Channels
} // namespace Artemis

#endif // _ARTEMIS_CHANNELS_H
