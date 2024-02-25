/**
 * @file current_sensors.cpp
 * @brief Definition of the Artemis CurrentSensors class.
 *
 * This file defines the methods for the CurrentSensors object.
 */
#include "artemis_devices.h"
#include "channels/artemis_channels.h"

namespace Artemis {
namespace Devices {
  /**
   * @brief Sets up the satellite's current sensors.
   *
   * This method of the CurrentSensors class sets up the I2C connection to the
   * satellite's current sensors.
   *
   * @todo Go through library and see what we need to configure and calibrate
   *
   * @return true All current sensors in current_sensors have been connected to
   * over I2C.
   * @return false At least one current sensor in current_sensors failed to
   * initialize over I2C.
   */
  bool CurrentSensors::setup(void) {
    for (auto &current_sensor : current_sensors) {
      if (!current_sensor.second->begin(&Wire2)) {
        currentSetup = false;
        return currentSetup;
      }
    }
    currentSetup = true;
    return currentSetup;
  }

  /**
   * @brief Reads the satellite's current sensors.
   *
   * This method of the CurrentSensors class reads the current sensor values,
   * stores them in two beacons (currentbeacon1 and currentbeacon2), and
   * transmits those beacons to the ground.
   *
   * @param uptime The time, in milliseconds, since the Teensy has been
   * powered on.
   */
  void CurrentSensors::read(uint32_t uptime) {
    if (!currentSetup) {
      setup();
    }

    PacketComm     packet;
    currentbeacon1 beacon1;
    currentbeacon2 beacon2;
    packet.header.nodeorig = (uint8_t)NODES::TEENSY_NODE_ID;
    packet.header.nodedest = (uint8_t)NODES::GROUND_NODE_ID;
    packet.header.type     = PacketComm::TypeId::DataObcBeacon;

    for (auto &it : current_sensors) {
      const int i = std::distance(current_sensors.begin(),
                                  current_sensors.find(it.first));
      if (i < ARTEMIS_CURRENT_BEACON_1_COUNT) {
        beacon1.busvoltage[i] = (current_sensors[it.first]->getBusVoltage_V());
        beacon1.current[i]    = (current_sensors[it.first]->getCurrent_mA());
      } else {
        beacon2.busvoltage[i - ARTEMIS_CURRENT_BEACON_1_COUNT] =
            (current_sensors[it.first]->getBusVoltage_V());
        beacon2.current[i - ARTEMIS_CURRENT_BEACON_1_COUNT] =
            (current_sensors[it.first]->getCurrent_mA());
      }
    }

    beacon1.deci = uptime;
    packet.data.resize(sizeof(beacon1));
    memcpy(packet.data.data(), &beacon1, sizeof(beacon1));
    packet.header.chanin  = 0;
    packet.header.chanout = Artemis::Channels::Channel_ID::RFM23_CHANNEL;
    route_packet_to_rfm23(packet);

    beacon2.deci = uptime;
    packet.data.resize(sizeof(beacon2));
    memcpy(packet.data.data(), &beacon2, sizeof(beacon2));
    packet.header.chanin  = 0;
    packet.header.chanout = Artemis::Channels::Channel_ID::RFM23_CHANNEL;
    route_packet_to_rfm23(packet);
  }
}
}