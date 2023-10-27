/**
 * @file rfm23_channel.cpp
 * @brief The RFM23 channel.
 *
 * The definition of the RFM23 channel.
 */
#include "channels/artemis_channels.h"
#include <rfm23.h>

namespace Artemis {
  namespace Channels {
    /** @brief The RFM23 channel. */
    namespace RFM23 {
      using Artemis::Devices::RFM23;
      /** @brief The packet used throughout the channel. */
      PacketComm          packet;
      /** @brief The radio's configuration used throughout the channel. */
      RFM23::rfm23_config config = {
          .freq     = 433,
          .tx_power = RH_RF22_RF23BP_TXPOW_30DBM,
          .pins =
              {
                     .spi_miso = SPI1_D0,
                     .spi_mosi = SPI1_D1,
                     .spi_sck  = SPI1_SCLK,
                     .nirq     = NIRQ,
                     .cs       = SPI1_CS1,
                     .tx_on    = TX_ON,
                     .rx_on    = RX_ON,
                     },
      };
      /** @brief The radio object used throughout the channel. */
      RFM23 radio(config.pins.cs, config.pins.nirq, hardware_spi1);

      /**
       * @brief The top-level channel definition.
       *
       * This is the function that defines the RFM23 channel. Like an Arduino
       * script, it has a setup() function that is run once, then loop() runs
       * forever.
       */
      void  rfm23_channel() {
        setup();
        loop();
      }

      /**
       * @brief The RFM23 setup function.
       *
       * This function is run once, when the channel is started. It connects to
       * the RFM23 over a SPI connection.
       */
      void setup() {
        print_debug(Helpers::RFM23, "RFM23 channel starting...");
        while (!radio.init(config, &spi1_mtx)) {
        }
      }

      /**
       * @brief The RFM23 loop function.
       *
       * This function runs in an infinite loop after setup() completes. It
       * routes packets going to and coming from the RFM23 radio.
       */
      void loop() {
        while (true) {
          receive_from_radio();
          handle_queue();
          threads.delay(10);
        }
      }

      /** @brief Helper function to receive a packet from the RFM23 radio. */
      void receive_from_radio() {
        int32_t timeout = (5 - rfm23_queue.size()) * SECONDS;
        if (timeout < MINIMUM_TIMEOUT) {
          timeout = MINIMUM_TIMEOUT;
        }
        if (radio.recv(packet, (uint16_t)timeout) >= 0) {
          print_debug(Helpers::RFM23, "Received ",
                      (int32_t)packet.wrapped.size(), " bytes from radio.");
          print_hexdump(Helpers::RFM23, "Raw bytes: ", &packet.wrapped[0],
                        packet.wrapped.size());
          threads.delay(2 * SECONDS);
          route_packet_to_main(packet);
        }
      }

      /**
       * @brief Helper function to handle packet queue.
       *
       * This is a helper function called in loop() that checks for packets and
       * routes them to the RFM23 radio.
       */
      void handle_queue() {
        if (PullQueue(packet, rfm23_queue, rfm23_queue_mtx)) {
          switch (packet.header.type) {
            print_debug(Helpers::RFM23, "Pulled packet of type ",
                        (uint16_t)packet.header.type, " from queue.");
            case PacketComm::TypeId::DataObcBeacon:
            case PacketComm::TypeId::DataObcPong:
            case PacketComm::TypeId::DataEpsResponse:
            case PacketComm::TypeId::DataRadioResponse:
            case PacketComm::TypeId::DataAdcsResponse:
            case PacketComm::TypeId::DataObcResponse: {
              if (!radio.send(packet)) {
                print_debug(
                    Helpers::RFM23,
                    "Failed to send packet through RFM23. Dropping packet.");
              }
              threads.delay(500);
              break;
            }
            default: {
              print_debug(Helpers::RFM23,
                          "Type not yet handled. Dropping packet.");
              break;
            }
          }
        }
      }
    } // namespace RFM23
  }   // namespace Channels
} // namespace Artemis
