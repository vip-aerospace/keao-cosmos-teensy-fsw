/**
 * @file rpi_channel.cpp
 * @brief The Raspberry Pi channel.
 *
 * The definition of the Raspberry Pi channel.
 */
#include "channels/artemis_channels.h"
#include <pdu.h>

namespace Artemis {
namespace Channels {
  /** @brief The Raspberry Pi channel. */
  namespace RPI {
    using Artemis::Devices::PDU;
    /** @brief The packet used throughout the channel. */
    PacketComm packet;
    /** @brief Whether the Raspberry Pi is on and active. */
    bool       piIsOn = false;

    /**
     * @brief The top-level channel definition.
     *
     * This is the function that defines the Raspberry Pi channel. Like an
     * Arduino script, it has a setup() function that is run once, then loop()
     * runs forever.
     */
    void       rpi_channel() {
      setup();
      loop();
    }

    /**
     * @brief The Raspberry Pi setup function.
     *
     * This function is run once, when the channel is started. It connects to
     * the Raspberry Pi over a serial connection.
     *
     * @todo Ensure the Pi is on before completing setup.
     */
    void setup() {
      print_debug(Helpers::RPI, "RPI channel starting...");
      Serial2.begin(9600);
      while (!Serial2) {
      }
      // Try pinging the RPi to ensure we can communicate with it.
      // Retry and wait until a ping is successful before continuing.
      // Set the piIsOn variable when successful.
    }

    /**
     * @brief The Raspberry Pi loop function.
     *
     * This function runs in an infinite loop after setup() completes. It routes
     * packets going to and coming from the Raspberry Pi.
     *
     * @todo Handle the case where the Pi is not on.
     */
    void loop() {
      while (true) {
        if (!piIsOn) {
          // Try pinging the RPi to ensure we can communicate with it.
          // Retry and wait until a ping is successful before continuing.
          // Set the piIsOn variable when successful.
        }
        receive_from_pi();
        handle_queue();
        threads.delay(100);
      }
    }

    /**
     * @brief Helper function to receive a packet from the Raspberry Pi.
     * 
     * It seems that the micro-cosmos implementation of PacketComm/SLIPLib does 
     * not provide a convenient get_slip() method. This means that we need to 
     * manually parse the Serial stream for something that *looks like* SLIP
     * packets. From there, we use the standard SLIPUnPacketize() method to turn
     * it back into a regular packet for routing.
     * 
     * This method is temporarily extra-documented, to explain how this 
     * searching works.
     * 
     * @todo See if there's a better way of doing this.
     */
    void receive_from_pi() {
      // While there are bytes available on the serial connection to the RPi
      while(Serial2.available()){
        // Read a character from that connection.
        uint8_t inChar = Serial2.read();
        // If it is our magic character,
        if(inChar == 0xC0){
          // Create a large buffer for the incoming packet.
          uint8_t readBuffer[2048];
          // Clear the buffer.
          memset(readBuffer, 0, sizeof(readBuffer));
          // Read the packet's bytes in from the serial connection.
          // Note that this reads everything between the start and end flags 
          // (each of which are 0xC0), but does not include those flags.
          // Therefore, write these incoming bytes to the buffer starting from 
          // the *second* position in the array (index 1), and cap the maximum
          // number of bytes to read to be the size of the buffer plus both 
          // flags.
          size_t readBytes = Serial2.readBytesUntil((SLIP_FEND), &readBuffer[1], (size_t)2046);
          // Resize the packetized copy of the packet to be the number of bytes 
          // read in, plus the start and end flags.
          packet.packetized.resize(readBytes + 2);
          // Manually add the start...
          readBuffer[0] = 0xC0;
          // ...and end flags to the buffer at the appropriate places, "capping
          // off" the received data.
          readBuffer[readBytes + 1] = 0xC0;
          // Copy the packet in the buffer to the actual packet, in the 
          // packetized vector for further processing.
          memcpy(packet.packetized.data(), &readBuffer[0], readBytes + 2);
          
          // Invoke the micro-cosmos SLIPUnPacketize. This assumes that the 
          // packet's packetized vector has the start and end flags, as well as 
          // a CRC checksum at the end.
          if(!packet.SLIPUnPacketize()){
            print_debug(Helpers::RPI, "Failed to SLIP unpacketize incoming packet");
            return;
          }

          print_debug(Helpers::RPI, "Pushing packet of type ",
                    (uint16_t)packet.header.type, " to main queue.");
          
          // If the un-packetizing is successful, pass the packet to be routed 
          // in the main queue.
          PushQueue(packet, main_queue, main_queue_mtx);
        }
        // Note that, implicitly, this discards anything outside the bounds of
        // the first packet. This means that the last few bytes of the first 
        // partial packet will be discarded. This is considered expected 
        // behavior, since there's nothing you can do to get that first packet's
        // bytes anyways.
      }
    }

    /**
     * @brief Helper function to handle packet queue.
     *
     * This is a helper function called in loop() that checks for packets and
     * routes them to the Raspberry Pi.
     */
    void handle_queue() {
      if (PullQueue(packet, rpi_queue, rpi_queue_mtx)) {
        print_debug(Helpers::RPI, "Pulled packet of type ",
                    (uint16_t)packet.header.type, " from Raspberry Pi queue.");
        switch (packet.header.type) {
          case PacketComm::TypeId::CommandEpsSwitchName: {
            if ((PDU::PDU_SW)packet.data[0] == PDU::PDU_SW::RPI &&
                packet.data[1] == 0) {
              shut_down_pi();
            }
            break;
          }
          default: {
            send_to_pi();
            break;
          }
        }
      }
    }

    /** @brief Shuts down the Raspberry Pi and kills the channel.*/
    void shut_down_pi() {
      packet.header.type = PacketComm::TypeId::CommandObcHalt;
      send_to_pi();
      // Wait 20s to give the rpi time to turn off
      threads.delay(20 * SECONDS);
      digitalWrite(RPI_ENABLE, LOW);

      // Empty RPI Queue
      while (!rpi_queue.empty()) {
        rpi_queue.pop_front();
      }

      piIsOn = false;
    }

    /** @brief Helper function to send a packet to the Raspberry Pi. */
    void send_to_pi() {
      if (!packet.SLIPPacketize()) {
        print_debug(Helpers::RPI, "Failed to wrap and SLIP packetize");
      }
      print_hexdump(Helpers::RPI, "Forwarding to RPi: ", &packet.packetized[0],
                    packet.packetized.size());
      for (size_t i = 0; i < packet.packetized.size(); i++) {
        if (Serial2.write(packet.packetized[i]) != 1) {
          print_debug(Helpers::RPI,
                      "Failed to send byte to RPi: ", (u_int32_t)i);
          piIsOn = false;
        }
      }
    }
  } // namespace RPI
} // namespace Channels
} // namespace Artemis
