/**
 * @file main.cpp
 * @brief The main operating loop for the Teensy.
 *
 * This file contains the main oprerating loop that runs on the Teensy. Like an
 * Arduino sketch, it consists of a setup and loop function.
 */
#include "artemis_devices.h"
#include "artemisbeacons.h"
#include "channels/artemis_channels.h"
#include "helpers.h"
#include <Arduino.h>
#include <USBHost_t36.h>
#include <pdu.h>
#include <support/configCosmosKernel.h>
#include <vector>

// For setting Teensy Clock Frequency (only for Teensy 4.0 and 4.1)
#if defined(__IMXRT1062__)
extern "C" uint32_t set_arm_clock(uint32_t frequency);
#endif

void setup_connections();
void setup_devices();
void setup_threads();

void beacon_artemis_devices();
void beacon_if_deployed();
void route_packets();

void route_packet_to_ground();
void ensure_rpi_is_powered();
void send_pong_reply();
void enable_rpi();
void report_rpi_enabled();
void update_pdu_switches();

namespace {
  using namespace Artemis;
  Devices::IMU                imu;
  Devices::Magnetometer       magnetometer;
  Devices::CurrentSensors     current_sensors;
  Devices::GPS                gps;
  Devices::TemperatureSensors temperature_sensors;
  PacketComm                  packet;
  USBHost                     usb;
  elapsedMillis               uptime;

  // Deployment variables
  elapsedMillis               deploymentbeacon;
  // const unsigned long readInterval = 300 * SECONDS; // Flight
  const unsigned long         readInterval = 20 * SECONDS; // Testing
} // namespace

/**
 * @brief Main setup function.
 *
 * This function is run once, when the Teensy is started. It initializes the
 * various sensors and connections on the Teensy.
 *
 * The frequency of the Teensy's processor is also set here. Allowed
 * frequencies in MHz are: 24, 150, 396, 450, 528, 600.
 */
void setup() {
#if defined(__IMXRT1062__)
  set_arm_clock(450000000);
#endif
  setup_connections();
  delay(3 * SECONDS);
  setup_devices();
  setup_threads();
  threads.delay(5 * SECONDS);
  Helpers::print_debug(Helpers::MAIN, "Teensy Flight Software Setup Complete");
}

/**
 * @brief Main loop function.
 *
 * This function runs in an infinite loop after setup() completes. It routes
 * packets among the various channels and periodically creates beacon packets
 * when in deployment mode. It also runs tests if they are enabled.
 */
void loop() {
  beacon_if_deployed();
  route_packets();
  gps.update();
  threads.delay(100);
}

/** @brief Helper function to set up connections on the Teensy. */
void setup_connections() {
  Helpers::connect_serial_debug(115200);
  usb.begin();
  pinMode(RPI_ENABLE, OUTPUT);
  pinMode(UART6_TX, INPUT);
  pinMode(UART6_RX, INPUT);
}

/** @brief Helper function to set up devices on the Teensy. */
void setup_devices() {
  if (!magnetometer.setup()) {
    print_debug(Helpers::MAIN, "Failed to setup magnetometer");
  }
  if (!imu.setup()) {
    print_debug(Helpers::MAIN, "Failed to setup IMU");
  }
  if (!current_sensors.setup()) {
    print_debug(Helpers::MAIN, "Failed to setup at least one current sensor");
  }
  if (!gps.setup()) {
    print_debug(Helpers::MAIN, "Failed to setup GPS");
  }
}

/** @brief Helper function to set up threads on the Teensy. */
void setup_threads() {
  if (threads.setSliceMillis(10) != 1) {
    print_debug(Helpers::MAIN,
                "Failed to assign computing time to all threads");
  }

  int thread_id = 0;
  if ((thread_id =
           threads.addThread(Channels::RFM23::rfm23_channel, 0, 4096)) == -1) {
    print_debug(Helpers::MAIN, "Failed to start rfm23_channel");
  } else {
    thread_list.push_back({thread_id, Channels::Channel_ID::RFM23_CHANNEL});
  }

  if ((thread_id = threads.addThread(Channels::PDU::pdu_channel, 0, 8192)) ==
      -1) {
    print_debug(Helpers::MAIN, "Failed to start pdu_channel");
  } else {
    thread_list.push_back({thread_id, Channels::Channel_ID::PDU_CHANNEL});
  }
#ifdef TESTS
  if ((thread_id = threads.addThread(Channels::TEST::test_channel, 0, 4096)) ==
      -1) {
    print_debug(Helpers::MAIN, "Failed to start test_channel");
  } else {
    thread_list.push_back({thread_id, Channels::Channel_ID::TEST_CHANNEL});
  }
#endif

  // Only uncomment these when testing and you want to force the RPi to turn on
  // thread_list.push_back({threads.addThread(Channels::RPI::rpi_channel),
  // Channels::Channel_ID::RPI_CHANNEL}); pinMode(RPI_ENABLE, HIGH);
}

/** @brief Helper function to poll Artemis devices for their readings. */
void beacon_artemis_devices() {
#ifdef ENABLE_TEMPERATURESENSORS
  temperature_sensors.read(uptime);
#endif

#ifdef ENABLE_CURRENTSENSORS
  current_sensors.read(uptime);
#endif

#ifdef ENABLE_IMU
  if (!imu.read(uptime)) {
    print_debug(Helpers::MAIN, "Failed to read IMU");
  }
#endif

#ifdef ENABLE_MAGNETOMETER
  if (!magnetometer.read(uptime)) {
    print_debug(Helpers::MAIN, "Failed to read magnetometer");
  }
#endif

#ifdef ENABLE_GPS
  gps.read(uptime);
#endif
}

/** @brief Helper function to beacon Artemis devices if in deployment mode. */
void beacon_if_deployed() {
  // During deployment mode send beacons every 5 minutes for 2 weeks.
  if (deploymentmode) {
    // Check if it's time to read the sensors
    if (deploymentbeacon >= readInterval) {
      Helpers::print_debug(Helpers::MAIN, "Deployment beacons sending");
      beacon_artemis_devices();
      update_pdu_switches();
      // Reset the timer
      deploymentbeacon = 0;
    }
  }
}

/** @brief Helper function to route packets. */
void route_packets() {
  if (PullQueue(packet, main_queue, main_queue_mtx)) {
    if (packet.header.nodedest == (uint8_t)NODES::GROUND_NODE_ID) {
      route_packet_to_ground();
    } else if (packet.header.nodedest == (uint8_t)NODES::RPI_NODE_ID) {
      ensure_rpi_is_powered();
      route_packet_to_rpi(packet);
    } else if (packet.header.nodedest == (uint8_t)NODES::TEENSY_NODE_ID) {
      switch (packet.header.type) {
        case PacketComm::TypeId::CommandObcPing: {
          send_pong_reply();
          break;
        }
        case PacketComm::TypeId::CommandEpsCommunicate: {
          route_packet_to_pdu(packet);
          break;
        }
        case PacketComm::TypeId::CommandEpsSwitchName: {
          Devices::PDU::PDU_SW switchid = (Devices::PDU::PDU_SW)packet.data[0];
          switch (switchid) {
            case Devices::PDU::PDU_SW::RPI: {
              if (packet.data[1] == 0) {
                route_packet_to_rpi(packet);
              } else if (packet.data[2] == 1) {
                enable_rpi();
                threads.delay(5 * SECONDS);
              } else {
                ensure_rpi_is_powered();
              }
              break;
            }
            default: {
              route_packet_to_pdu(packet);
              break;
            }
          }
          break;
        }
        case PacketComm::TypeId::CommandEpsSwitchStatus: {
          Devices::PDU::PDU_SW switchid = (Devices::PDU::PDU_SW)packet.data[0];
          switch (switchid) {
            case Devices::PDU::PDU_SW::RPI: {
              report_rpi_enabled();
              break;
            }
            default: {
              route_packet_to_pdu(packet);
              break;
            }
          }
          break;
        }
        case PacketComm::TypeId::CommandObcSendBeacon: {
          beacon_artemis_devices();
          update_pdu_switches();
          break;
        }
        default: {
          break;
        }
      }
    }
  }
}

/** @brief Helper function to route packets to ground. */
void route_packet_to_ground() {
  switch (packet.header.chanout) {
    case Channels::Channel_ID::RFM23_CHANNEL: {
      route_packet_to_rfm23(packet);
      break;
    }
    default: {
      break;
    }
  }
}

/** @brief Helper function to ensure the Raspberry Pi is powered. */
void ensure_rpi_is_powered() {
  if (!digitalRead(UART6_RX)) {
    float curr_V =
        current_sensors.current_sensors["battery_board"]->getBusVoltage_V();
    if (curr_V >= 7.0) {
      enable_rpi();
      threads.delay(5 * SECONDS);
    } else {
      update_pdu_switches();
    }
  }
}

/** @brief Helper function to send a pong reply. */
void send_pong_reply() {
  packet.header.nodedest = packet.header.nodeorig;
  packet.header.nodeorig = (uint8_t)NODES::TEENSY_NODE_ID;
  packet.header.type     = PacketComm::TypeId::DataObcPong;
  packet.data.resize(0);
  const char *data = "Pong";
  for (size_t i = 0; i < strlen(data); i++) {
    packet.data.push_back(data[i]);
  }
  route_packet_to_ground();
}

/** @brief Helper function to enable the Raspberry Pi. */
void enable_rpi() {
  Helpers::print_debug(Helpers::MAIN, "Turning on RPi");
  digitalWrite(RPI_ENABLE, HIGH);
  int thread_id = 0;
  if ((thread_id = threads.addThread(Channels::RPI::rpi_channel)) == -1) {
    print_debug(Helpers::MAIN, "Failed to start rpi_channel");
  } else {
    thread_list.push_back({thread_id, Channels::Channel_ID::RPI_CHANNEL});
  }
}

/** @brief Helper function to report if the Raspberry Pi is enabled. */
void report_rpi_enabled() {
  packet.data.resize(1);
  packet.data.push_back(digitalRead(RPI_ENABLE));
  packet.header.type     = PacketComm::TypeId::DataEpsResponse;
  packet.header.nodedest = packet.header.nodeorig;
  packet.header.nodeorig = (uint8_t)NODES::TEENSY_NODE_ID;
  route_packet_to_rfm23(packet);
}

/** @brief Helper function to request PDU switch state update. */
void update_pdu_switches() {
  packet.header.type     = PacketComm::TypeId::CommandEpsSwitchStatus;
  packet.header.nodeorig = (uint8_t)NODES::GROUND_NODE_ID;
  packet.header.nodedest = (uint8_t)NODES::TEENSY_NODE_ID;
  packet.data.clear();
  packet.data.push_back((uint8_t)Artemis::Devices::PDU::PDU_SW::All);
  route_packet_to_pdu(packet);
}
