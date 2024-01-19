/**
 * @file test_channel.cpp
 * @brief The tests channel.
 *
 * The definition of the tests channel.
 */
#include "channels/artemis_channels.h"
#include <pdu.h>

namespace Artemis {
namespace Channels {
  /** @brief The tests channel. */
  namespace TEST {
    /** @brief The packet used throughout the tests. */
    PacketComm    packet;
    /** @brief The time since the Raspberry Pi has been turned on. */
    elapsedMillis piShutdownTimer = 0;
    /** @brief Whether the Raspberry Pi is off. */
    bool          piIsOff         = false;
    /** @brief The number of packets transmitted by the RFM23. */
    uint32_t      packet_count    = 0;

    /**
     * @brief The top-level channel definition.
     *
     * This is the function that defines the tests channel. Like an Arduino
     * script, it has a setup() function that is run once, then loop() runs
     * forever.
     */
    void          test_channel() {
      setup();
      loop();
    }

    /**
     * @brief The test setup function.
     *
     * This function is run once, when the channel is started. It connects to
     * the Raspberry Pi over a serial connection.
     */
    void setup() { print_debug(Helpers::TEST, "Test channel starting..."); }

    /**
     * @brief The test loop function.
     *
     * This function runs in an infinite loop after setup() completes. It routes
     * packets going to and coming from the Raspberry Pi.
     */
    void loop() {
      while (true) {
        report_threads_status();
        report_memory_usage();
        report_queue_size();

        turn_on_rpi();
        threads.delay(500);
        pdu_switch_all_on();
        threads.delay(500);
        pdu_switch_status();
        threads.delay(500);
        rfm23_transmit();
        threads.delay(500);
        rpi_take_picture_from_teensy();
        threads.delay(500);
        rpi_take_picture_from_ground();
        threads.delay(500);
        turn_off_rpi();
        threads.delay(500);
      }
    }

    /**
     * @brief Test turning on the Raspberry Pi.
     *
     * This function simulates a packet sent from the ground to the satellite
     * commanding the Teensy to turn on the Raspberry Pi.
     */
    void turn_on_rpi() {
      if (piIsOff) {
        packet.header.type     = PacketComm::TypeId::CommandEpsSwitchName;
        packet.header.nodeorig = (uint8_t)NODES::GROUND_NODE_ID;
        packet.header.nodedest = (uint8_t)NODES::TEENSY_NODE_ID;
        packet.data.resize(0);
        packet.data.push_back((uint8_t)Artemis::Devices::PDU::PDU_SW::RPI);
        packet.data.push_back(1);
        route_packet_to_main(packet);
        piIsOff         = false;
        piShutdownTimer = 0;
      }
    }

    /**
     * @brief Test turning off the Raspberry Pi.
     *
     * This function simulates a packet sent from the ground to the satellite
     * commanding the Teensy to turn off the Raspberry Pi. A minimum of 10
     * seconds must have passed since the Raspberry Pi was turned on.
     */
    void turn_off_rpi() {
      if (piShutdownTimer > (10 * SECONDS) && !piIsOff) {
        packet.header.type     = PacketComm::TypeId::CommandEpsSwitchName;
        packet.header.nodeorig = (uint8_t)NODES::GROUND_NODE_ID;
        packet.header.nodedest = (uint8_t)NODES::RPI_NODE_ID;
        packet.data.clear();
        packet.data.push_back((uint8_t)Artemis::Devices::PDU::PDU_SW::RPI);
        packet.data.push_back(0);
        route_packet_to_main(packet);
        piIsOff = true;
      }
    }

    /**
     * @brief Test taking a picture (from the Teensy).
     *
     * This function simulates a packet sent from the Teensy to the Raspberry
     * Pi commanding it to take a picture.
     */
    void rpi_take_picture_from_teensy() {
      if (!piIsOff) {
        packet.header.type     = (PacketComm::TypeId)0x800;
        packet.header.nodeorig = (uint8_t)NODES::TEENSY_NODE_ID;
        packet.header.nodedest = (uint8_t)NODES::RPI_NODE_ID;
        packet.data.resize(0);
        route_packet_to_rpi(packet);
      }
    }

    /**
     * @brief Test taking a picture (from the ground).
     *
     * This function simulates a packet sent from the ground to the satellite
     * commanding it to take a picture.
     */
    void rpi_take_picture_from_ground() {
      packet.header.type     = PacketComm::TypeId::CommandCameraCapture;
      packet.header.nodeorig = (uint8_t)NODES::GROUND_NODE_ID;
      packet.header.nodedest = (uint8_t)NODES::RPI_NODE_ID;
      packet.data.clear();
      route_packet_to_main(packet);
    }

    /**
     * @brief Test enabling all PDU switches.
     *
     * This function simulates a packet sent from the ground to the satellite
     * commanding it to enable all PDU switches.
     */
    void pdu_switch_all_on() {
      packet.header.type     = PacketComm::TypeId::CommandEpsSwitchName;
      packet.header.nodeorig = (uint8_t)NODES::GROUND_NODE_ID;
      packet.header.nodedest = (uint8_t)NODES::TEENSY_NODE_ID;
      packet.data.resize(0);
      packet.data.push_back((uint8_t)Artemis::Devices::PDU::PDU_SW::All);
      packet.data.push_back(1);
      route_packet_to_main(packet);
    }

    /**
     * @brief Test requesting status of all PDU switches.
     *
     * This function simulates a packet sent from the ground to the satellite
     * commanding it to report the status of all PDU switches.
     */
    void pdu_switch_status() {
      packet.header.type     = PacketComm::TypeId::CommandEpsSwitchStatus;
      packet.header.nodeorig = (uint8_t)NODES::GROUND_NODE_ID;
      packet.header.nodedest = (uint8_t)NODES::TEENSY_NODE_ID;
      packet.data.clear();
      packet.data.push_back((uint8_t)Artemis::Devices::PDU::PDU_SW::All);
      route_packet_to_pdu(packet);
    }

    /**
     * @brief Test transmitting from the RFM23.
     *
     * This function simulates a packet sent from the Teensy to the RFM23
     * commanding it to send a packet of data to the ground.
     */
    void rfm23_transmit() {
      packet.header.type     = PacketComm::TypeId::DataObcResponse;
      packet.header.nodeorig = (uint8_t)NODES::TEENSY_NODE_ID;
      packet.header.nodedest = (uint8_t)NODES::GROUND_NODE_ID;
      packet.header.chanin   = 0;
      packet.header.chanout  = Artemis::Channels::Channel_ID::RFM23_CHANNEL;

      packet.data.resize(0);
      String data_str = "Hello World!" + String(packet_count);
      Helpers::print_hexdump(Helpers::TEST,
                             "data_str: ", (uint8_t *)data_str.c_str(),
                             data_str.length());
      for (size_t i = 0; i < data_str.length(); i++) {
        packet.data.push_back(data_str[i] - '0');
      }
      packet_count++;

      route_packet_to_main(packet);
    }

    /** @brief Report on the status of all currently running threads. */
    void report_threads_status() {
      for (auto &t : thread_list) {
        Helpers::print_debug(Helpers::TEST, "thread_id:", t.thread_id,
                             " channel_id:", (int)t.channel_id,
                             " state:", threads.getState(t.thread_id));
      }
    }

    /** @brief Report on the current memory utilization. */
    void report_memory_usage() {
      long  totalMemory = &_heap_end - &_heap_start;
      long  freeMemory  = &_heap_end - (unsigned long *)__brkval;
      long  usedMemory  = totalMemory - freeMemory;
      float memoryUtilization =
          ((float)(usedMemory) / (float)totalMemory) * 100.0;
      Helpers::print_debug(Helpers::TEST, "Memory usage: ", usedMemory, "/",
                           totalMemory, " bytes (", memoryUtilization,
                           "% utilization)");
    }

    /** @brief Report on the size of each of the queues. */
    void report_queue_size() {
      Helpers::print_debug(Helpers::TEST, "main_queue contains ",
                           main_queue.size(), " packets, with a size of ",
                           sizeof(PacketComm) * main_queue.size(), " bytes");
      Helpers::print_debug(Helpers::TEST, "rfm23_queue contains ",
                           rfm23_queue.size(), " packets, with a size of ",
                           sizeof(PacketComm) * rfm23_queue.size(), " bytes");
      Helpers::print_debug(Helpers::TEST, "pdu_queue contains ",
                           pdu_queue.size(), " packets, with a size of ",
                           sizeof(PacketComm) * pdu_queue.size(), " bytes");
      Helpers::print_debug(Helpers::TEST, "rpi_queue contains ",
                           rpi_queue.size(), " packets, with a size of ",
                           sizeof(PacketComm) * rpi_queue.size(), " bytes");
      Helpers::print_debug(Helpers::TEST, "Packets in queues are taking up ",
                           sizeof(PacketComm) *
                               (main_queue.size() + rfm23_queue.size() +
                                pdu_queue.size() + rpi_queue.size()),
                           " bytes");
    }
  } // namespace TEST
} // namespace Channels
} // namespace Artemis
