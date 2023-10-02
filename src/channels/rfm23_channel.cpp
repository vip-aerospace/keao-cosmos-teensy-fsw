#include "channels/artemis_channels.h"
#include <rfm23.h>

namespace Artemis {
  namespace Channels {
    namespace RFM23 {
      using Artemis::Devices::RFM23;
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

      RFM23      radio(config.pins.cs, config.pins.nirq, hardware_spi1);
      PacketComm packet;

      void       rfm23_channel() {
        setup();
        loop();
      }

      void setup() {
        while (radio.init(config, &spi1_mtx) < 0)
          ;
      }

      void loop() {
        while (true) {
          handle_queue();
          receive_from_radio();
          threads.delay(10);
        }
      }

      void handle_queue() {
        if (PullQueue(packet, rfm23_queue, rfm23_queue_mtx)) {
          switch (packet.header.type) {
            case PacketComm::TypeId::DataObcBeacon:
            case PacketComm::TypeId::DataObcPong:
            case PacketComm::TypeId::DataEpsResponse:
            case PacketComm::TypeId::DataRadioResponse:
            case PacketComm::TypeId::DataAdcsResponse:
            case PacketComm::TypeId::DataObcResponse: {
              radio.send(packet);
              threads.delay(500);
              break;
            }
            default:
              break;
          }
        }
      }

      void receive_from_radio() {
        int32_t timeout = 5000 - rfm23_queue.size() * 1000;
        if (timeout < 100)
          timeout = 100;
        if (radio.recv(packet, (uint16_t)timeout) >= 0) {
          Helpers::print_hexdump(Helpers::RFM23,
                                 "Radio received :", &packet.wrapped[0],
                                 packet.wrapped.size());
          Helpers::print_debug(Helpers::RFM23, "Bytes received: ",
                               (int32_t)packet.wrapped.size());

          threads.delay(2000);
          PushQueue(packet, main_queue, main_queue_mtx);
        }
      }
    } // namespace RFM23
  }   // namespace Channels
} // namespace Artemis
