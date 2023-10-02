#include <rfm23.h>

namespace Artemis {
  namespace Devices {
    RFM23::RFM23(uint8_t slaveSelectPin, uint8_t interruptPin,
                 RHGenericSPI &spi)
        : rfm23(slaveSelectPin, interruptPin, spi) {}

    bool RFM23::init(rfm23_config cfg, Threads::Mutex *mtx) {
      config  = cfg;
      spi_mtx = mtx;

      Threads::Scope lock(*spi_mtx);
      SPI1.setMISO(config.pins.spi_miso);
      SPI1.setMOSI(config.pins.spi_mosi);
      SPI1.setSCK(config.pins.spi_sck);
      pinMode(config.pins.rx_on, OUTPUT);
      pinMode(config.pins.tx_on, OUTPUT);

      elapsedMillis timeout;
      while (!rfm23.init()) {
        if (timeout > 10000) {
          print_debug(Helpers::RFM23, "Radio failed to initialize");
          return false;
        }
      }
      if (!rfm23.setFrequency(config.freq)) {
        print_debug(Helpers::RFM23, "Failed to set config: frequency");
        return false;
      }

      rfm23.setTxPower(config.tx_power);

      timeout = 0;
      while (!rfm23.setModemConfig(RH_RF22::FSK_Rb2Fd5)) {
        if (timeout > 10000) {
          print_debug(Helpers::RFM23,
                      "Failed to set config: modem configuration");
          return false;
        }
      }
      // TODO: Assuming this is a *test* of whether we can go into sleep mode.
      if (!rfm23.sleep()) {
        print_debug(Helpers::RFM23, "Failed to set radio in sleep mode");
        return false;
      }

      print_debug(Helpers::RFM23, "Radio initialized");
      rfm23.setModeIdle();
      return true;
    }

    bool RFM23::reset() {
      Threads::Scope lock(*spi_mtx);
      rfm23.reset();
      return true;
    }

    bool RFM23::send(PacketComm &packet) {
      digitalWrite(config.pins.rx_on, HIGH);
      digitalWrite(config.pins.tx_on, LOW);

      packet.wrapped.resize(0);
      if (!packet.Wrap()) {
        print_debug(Helpers::RFM23, "Failed to wrap packet");
        return false;
      }
      if (packet.wrapped.size() > RH_RF22_MAX_MESSAGE_LEN) {
        print_debug(Helpers::RFM23, "Wrapped packet exceeds size limits");
        return false;
      }

      print_hexdump(Helpers::RFM23, "Radio Sending: ", packet.wrapped.data(),
                    packet.wrapped.size());
      Threads::Scope lock(*spi_mtx);
      if (!rfm23.send(packet.wrapped.data(), packet.wrapped.size())) {
        print_debug(Helpers::RFM23, "Failed to queue outgoing packet to radio");
        return false;
      }
      if (!rfm23.waitPacketSent(1000)) {
        print_debug(Helpers::RFM23,
                    "Timed out waiting for packet transmission");
        return false;
      }

      // TODO: Do we want to set the radio to sleep mode, or idle?
      rfm23.sleep();
      rfm23.setModeIdle();
      return true;
    }

    int32_t RFM23::recv(PacketComm &packet, uint16_t timeout) {
      digitalWrite(config.pins.rx_on, LOW);
      digitalWrite(config.pins.tx_on, HIGH);

      Threads::Scope lock(*spi_mtx);
      if (rfm23.waitAvailableTimeout(timeout)) {
        packet.wrapped.resize(0);
        packet.wrapped.resize(RH_RF22_MAX_MESSAGE_LEN);
        uint8_t bytes_recieved = packet.wrapped.size();
        if (rfm23.recv(packet.wrapped.data(), &bytes_recieved)) {
          packet.wrapped.resize(bytes_recieved);
          if (packet.Unwrap() < 0) {
            print_debug(Helpers::RFM23,
                        "Data was received, but not in packetcomm format.");
            return -1;
          }
          rfm23.setModeIdle();
          return packet.wrapped.size();
        }
      }
      rfm23.setModeIdle();
      return -1;
    }
  } // namespace Devices
} // namespace Artemis