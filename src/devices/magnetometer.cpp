/**
 * @file magnetometer.cpp
 * @brief Definition of the Artemis Magnetometer object.
 *
 * This file defines the methods for the Magnetometer object.
 */
#include "artemis_devices.h"
#include "channels/artemis_channels.h"

namespace Artemis {
namespace Devices {
  /**
   * @brief Sets up the satellite's magnetometer.
   *
   * This method of the Magnetometer class sets up the I2C connection to the
   * satellite's magnetometer and applies settings to it.
   *
   * @return true The magnetometer has been successfully set up.
   * @return false The I2C connection to the magnetometer failed to start.
   */
  bool Magnetometer::setup(void) {
    if ((magnetometerSetup = magnetometer->begin_I2C())) {
      magnetometer->setPerformanceMode(LIS3MDL_LOWPOWERMODE);
      magnetometer->setDataRate(LIS3MDL_DATARATE_0_625_HZ);
      magnetometer->setRange(LIS3MDL_RANGE_16_GAUSS);
      magnetometer->setOperationMode(LIS3MDL_CONTINUOUSMODE);
    }
    return magnetometerSetup;
  }

  /**
   * @brief Reads the satellite's magnetometer.
   *
   * This method of the Magnetometer class reads the magnetometer's values,
   * stores it in a magbeacon, and transmits that beacon to the ground.
   *
   * @param uptime The time, in milliseconds, since the Teensy has been
   * powered on.
   * @return true The magnetometer has been successfully read and a packet
   * carrying the reading has been queued for transmission.
   * @return false The magnetometer could not be read or hasn't been set up.
   */
  bool Magnetometer::read(uint32_t uptime) {
    if (!magnetometerSetup) {
      setup();
    }

    PacketComm packet;
    magbeacon  beacon;
    beacon.deci = uptime;

    sensors_event_t event;
    if (!magnetometer->getEvent(&event) || !magnetometerSetup) {
      return false;
    }
    beacon.magx            = (event.magnetic.x);
    beacon.magy            = (event.magnetic.y);
    beacon.magz            = (event.magnetic.z);

    packet.header.nodeorig = (uint8_t)NODES::TEENSY_NODE_ID;
    packet.header.nodedest = (uint8_t)NODES::GROUND_NODE_ID;
    packet.header.type     = PacketComm::TypeId::DataObcBeacon;
    packet.data.resize(sizeof(beacon));
    memcpy(packet.data.data(), &beacon, sizeof(beacon));
    packet.header.chanin  = 0;
    packet.header.chanout = Artemis::Channels::Channel_ID::RFM23_CHANNEL;
    route_packet_to_rfm23(packet);

    return true;
  }
}
}