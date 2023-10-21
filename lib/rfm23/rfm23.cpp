/**
 * @file rfm23.cpp
 * @brief The RFM23 radio class.
 *
 * This file contains definitions for the RFM23 radio class.
 */
#include <rfm23.h>

namespace Artemis {
  namespace Devices {
    /**
     * @brief Construct a new RFM23 object. Wraps the RH_RFM23 constructor.
     *
     * @param slaveSelectPin The chip select pin unique to this instance.
     * @param interruptPin The interrupt pin unique to this instance.
     * @param spi The SPI interface connecting the Teensy to the radio.
     */
    RFM23::RFM23(uint8_t slaveSelectPin, uint8_t interruptPin,
                 RHGenericSPI &spi)
        : rfm23(slaveSelectPin, interruptPin, spi) {}

    /**
     * @brief Initialize the RFM23 radio.
     *
     * @param cfg The radio's configuration struct.
     * @param mtx The mutex for the radio's SPI interface.
     * @return true The radio could be connected to and configured properly.
     * @return false The radio could not be connected to or configured.
     *
     * @todo The function puts the rfm23 into sleep mode, then idle mode. Is
     * this intended? idle overrides sleep.
     */
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
      if (!rfm23.sleep()) {
        print_debug(Helpers::RFM23, "Failed to set radio in sleep mode");
        return false;
      }

      print_debug(Helpers::RFM23, "Radio initialized");
      rfm23.setModeIdle();
      return true;
    }

    /** @brief Resets the radio. */
    void RFM23::reset() {
      Threads::Scope lock(*spi_mtx);
      rfm23.reset();
    }

    /**
     * @brief Sends a packet through the radio.
     *
     * @param packet The packet to be sent.
     * @return true The packet was successfully sent.
     * @return false There was an error sending the packet.
     *
     * @todo The function puts the rfm23 into sleep mode, then idle mode. Is
     * this intended? idle overrides sleep.
     *
     * @todo The transmit and receive pins are set low and high respectively.
     * check if this is intended behavior. either change the pin definitions or
     * use setGpioReversed().
     */
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

      rfm23.sleep();
      rfm23.setModeIdle();
      return true;
    }

    /**
     * @brief Receive a packet from the radio.
     *
     * @param packet The packet that will hold the received data.
     * @param timeout The time to wait for a packet from the radio.
     * @return int32_t The size of the received packet, or -1 if failed to
     * receive.
     *
     * @todo The transmit and receive pins are set high and low respectively.
     * check if this is intended behavior. either change the pin definitions or
     * use setGpioReversed().
     */
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