#include "artemis_devices.h"
#include "artemisbeacons.h"
#include "channels/artemis_channels.h"
#include "helpers.h"
#include "tests/tests.h"
#include <Arduino.h>
#include <USBHost_t36.h>
#include <pdu.h>
#include <support/configCosmosKernel.h>
#include <vector>

// For setting Teensy Clock Frequency (only for Teensy 4.0 and 4.1)
#if defined(__IMXRT1062__)
extern "C" uint32_t set_arm_clock(uint32_t frequency);
#endif

void run_tests();
void beacon_if_deployed();
void route_packets();
void setup_devices();
void setup_threads();

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
  int32_t                     iretn = 0;

  // Deployment variables
  elapsedMillis               deploymentbeacon;
  // const unsigned long readInterval = 300000; // Flight
  const unsigned long         readInterval = 20000; // Testing
} // namespace

void setup() {
  Helpers::connect_serial_debug(115200);

#if defined(__IMXRT1062__)
  set_arm_clock(
      450000000); // Allowed Frequencies (MHz): 24, 150, 396, 450, 528, 600
#endif

  usb.begin();
  pinMode(RPI_ENABLE, OUTPUT);
  delay(3000);
  setup_devices();
  setup_threads();
  threads.delay(5000);
  Helpers::print_debug(Helpers::MAIN, "Teensy Flight Software Setup Complete");
}

void loop() {
  pinMode(UART6_RX, INPUT);
  run_tests();
  beacon_if_deployed();
  route_packets();
  gps.update();
  threads.delay(100);
}

void setup_devices() {
  iretn = magnetometer.setup();
  iretn = imu.setup();
  current_sensors.setup();
  gps.setup();
}

void setup_threads() {
  threads.setSliceMillis(10);

  // Threads
  thread_list.push_back(
      {threads.addThread(Channels::RFM23::rfm23_channel, 0, 4096),
       Channels::Channel_ID::RFM23_CHANNEL});
  thread_list.push_back({threads.addThread(Channels::PDU::pdu_channel, 0, 8192),
                         Channels::Channel_ID::PDU_CHANNEL});

  // Only uncomment these when testing and you want to force the RPi to turn on
  // thread_list.push_back({threads.addThread(Channels::RPI::rpi_channel),
  // Channels::Channel_ID::RPI_CHANNEL}); pinMode(RPI_ENABLE, HIGH);
}

void run_tests() {
#ifdef TESTS
  run_test();
#ifdef ENABLE_TEMPERATURESENSORS
  temperature_sensors.read(uptime);
#endif

#ifdef ENABLE_CURRENTSENSORS
  current_sensors.read(uptime);
#endif

#ifdef ENABLE_IMU
  imu.read(uptime);
#endif

#ifdef ENABLE_MAGNETOMETER
  magnetometer.read(uptime);
#endif

#ifdef ENABLE_GPS
  gps.read(uptime);
#endif
  threads.delay(10000);
#endif
}

void beacon_if_deployed() {
  // During deployment mode send beacons every 5 minutes for 2 weeks.
  if (deploymentmode) {
    // Check if it's time to read the sensors
    if (deploymentbeacon >= readInterval) {
      Helpers::print_debug(Helpers::MAIN, "Deployment beacons sending");
      temperature_sensors.read(uptime);
      current_sensors.read(uptime);
      imu.read(uptime);
      magnetometer.read(uptime);
      gps.read(uptime);

      // Get PDU Switches
      packet.header.type     = PacketComm::TypeId::CommandEpsSwitchStatus;
      packet.header.nodeorig = (uint8_t)NODES::GROUND_NODE_ID;
      packet.header.nodedest = (uint8_t)NODES::TEENSY_NODE_ID;
      packet.data.clear();
      packet.data.push_back((uint8_t)Artemis::Devices::PDU::PDU_SW::All);
      PushQueue(packet, pdu_queue, pdu_queue_mtx);

      // Reset the timer
      deploymentbeacon = 0;
    }
  }
}

void route_packets() {
  if (PullQueue(packet, main_queue, main_queue_mtx)) {
    if (packet.header.nodedest == (uint8_t)NODES::GROUND_NODE_ID) {
      switch (packet.header.chanout) {
        case Channels::Channel_ID::RFM23_CHANNEL:
          PushQueue(packet, rfm23_queue, rfm23_queue_mtx);
          break;
        default:
          break;
      }
    } else if (packet.header.nodedest == (uint8_t)NODES::RPI_NODE_ID) {
      if (!digitalRead(UART6_RX)) {
        float curr_V =
            current_sensors.current_sensors["battery_board"]->getBusVoltage_V();
        if ((curr_V >= 7.0) || 1) {
          Helpers::print_debug(Helpers::MAIN, "Turning on RPi");
          digitalWrite(RPI_ENABLE, HIGH);
          thread_list.push_back(
              {threads.addThread(Channels::RPI::rpi_channel, 9000),
               Channels::Channel_ID::RPI_CHANNEL});
          threads.delay(5000);
        } else {
          packet.header.type     = PacketComm::TypeId::CommandEpsSwitchStatus;
          packet.header.nodeorig = (uint8_t)NODES::GROUND_NODE_ID;
          packet.header.nodedest = (uint8_t)NODES::TEENSY_NODE_ID;
          packet.data.clear();
          packet.data.push_back((uint8_t)Artemis::Devices::PDU::PDU_SW::All);
          PushQueue(packet, pdu_queue, pdu_queue_mtx);
        }
      }
      PushQueue(packet, rpi_queue, rpi_queue_mtx);
    } else if (packet.header.nodedest == (uint8_t)NODES::TEENSY_NODE_ID) {
      switch (packet.header.type) {
        case PacketComm::TypeId::CommandObcPing: {
          packet.header.nodedest = packet.header.nodeorig;
          packet.header.nodeorig = (uint8_t)NODES::TEENSY_NODE_ID;
          packet.header.type     = PacketComm::TypeId::DataObcPong;
          packet.data.resize(0);
          const char *data = "Pong";
          for (size_t i = 0; i < strlen(data); i++) {
            packet.data.push_back(data[i]);
          }
          switch (packet.header.chanout) {
            case Channels::Channel_ID::RFM23_CHANNEL:
              PushQueue(packet, rfm23_queue, rfm23_queue_mtx);
              break;
            default:
              break;
          }
          break;
        }
        case PacketComm::TypeId::CommandEpsCommunicate: {
          PushQueue(packet, pdu_queue, pdu_queue_mtx);
        } break;
        case PacketComm::TypeId::CommandEpsSwitchName: {
          Devices::PDU::PDU_SW switchid = (Devices::PDU::PDU_SW)packet.data[0];
          switch (switchid) {
            case Devices::PDU::PDU_SW::RPI: {
              float curr_V = current_sensors.current_sensors["battery_board"]
                                 ->getBusVoltage_V();
              if ((packet.data[1] == 1 && curr_V >= 7.0) ||
                  (packet.data[1] == 1 && packet.data[2] == 1)) {
                digitalWrite(RPI_ENABLE, packet.data[1]);
                thread_list.push_back(
                    {threads.addThread(Channels::RPI::rpi_channel),
                     Channels::Channel_ID::RPI_CHANNEL});
                threads.delay(5000);
              } else if (packet.data[1] == 0) {
                PushQueue(packet, rpi_queue, rpi_queue_mtx);
              } else {
                packet.header.type = PacketComm::TypeId::CommandEpsSwitchStatus;
                packet.header.nodeorig = (uint8_t)NODES::GROUND_NODE_ID;
                packet.header.nodedest = (uint8_t)NODES::TEENSY_NODE_ID;
                packet.data.clear();
                packet.data.push_back(
                    (uint8_t)Artemis::Devices::PDU::PDU_SW::All);
                PushQueue(packet, pdu_queue, pdu_queue_mtx);
              }
              break;
            }
            default:
              PushQueue(packet, pdu_queue, pdu_queue_mtx);
              break;
          }
          break;
        }
        case PacketComm::TypeId::CommandEpsSwitchStatus: {
          Devices::PDU::PDU_SW switchid = (Devices::PDU::PDU_SW)packet.data[0];
          switch (switchid) {
            case Devices::PDU::PDU_SW::RPI: {
              packet.data.resize(1);
              packet.data.push_back(digitalRead(RPI_ENABLE));
              packet.header.type     = PacketComm::TypeId::DataEpsResponse;
              packet.header.nodedest = packet.header.nodeorig;
              packet.header.nodeorig = (uint8_t)NODES::TEENSY_NODE_ID;
              PushQueue(packet, rfm23_queue, rfm23_queue_mtx);
            } break;
            default:
              PushQueue(packet, pdu_queue, pdu_queue_mtx);
              break;
          }
        } break;
        case PacketComm::TypeId::CommandObcSendBeacon: {
          temperature_sensors.read(uptime);
          current_sensors.read(uptime);
          imu.read(uptime);
          magnetometer.read(uptime);
          gps.read(uptime);

          // Get PDU Switches
          packet.header.type     = PacketComm::TypeId::CommandEpsSwitchStatus;
          packet.header.nodeorig = (uint8_t)NODES::GROUND_NODE_ID;
          packet.header.nodedest = (uint8_t)NODES::TEENSY_NODE_ID;
          packet.data.clear();
          packet.data.push_back((uint8_t)Artemis::Devices::PDU::PDU_SW::All);
          PushQueue(packet, pdu_queue, pdu_queue_mtx);
        }
        default:
          break;
      }
    }
  }
}
