/**
 * @file temperature_sensors.cpp
 * @brief Definition of the Artemis TemperatureSensors class.
 *
 * This file defines the methods for the TemperatureSensors object.
 */
#include "artemis_devices.h"
#include "channels/artemis_channels.h"

namespace Artemis {
namespace Devices {
  /**
   * @brief Sets up the satellite's temperature sensors.
   *
   * This method of the TemperatureSensors class sets up the analog connection
   * to the satellite's temperature sensors.
   */
  void TemperatureSensors::setup(void) {
    for (auto &temperature_sensor : temp_sensors) {
      pinMode(temperature_sensor.second, INPUT);
    }
  }

  /**
   * @brief Reads the satellite's temperature sensors.
   *
   * This method of the TemperatureSensors class reads the temperature sensor
   * values, stores them in a temperaturebeacon, and transmits that beacon to
   * the ground.
   *
   * The temperature sensors are read as an analog voltage, then converted to
   * a temperature in Celsius. The Teensy's internal temperature sensor is also
   * read.
   *
   * @param uptime The time, in milliseconds, since the Teensy has been
   * powered on.
   */
  void TemperatureSensors::read(uint32_t uptime) {
    PacketComm        packet;
    temperaturebeacon beacon;
    beacon.deci = uptime;

    for (auto &it : temp_sensors) {
      const int   reading      = analogRead(it.second);
      float       voltage      = reading * MV_PER_ADC_UNIT;
      const float temperatureF = (voltage - OFFSET_F) / MV_PER_DEGREE_F;
      beacon.tmp36_tempC[std::distance(temp_sensors.begin(),
                                       temp_sensors.find(it.first))] =
          (temperatureF - 32) * 5 / 9;
    }

    beacon.teensy_tempC    = InternalTemperature.readTemperatureC();

    packet.header.nodeorig = (uint8_t)NODES::TEENSY_NODE_ID;
    packet.header.nodedest = (uint8_t)NODES::GROUND_NODE_ID;
    packet.header.type     = PacketComm::TypeId::DataObcBeacon;
    packet.data.resize(sizeof(beacon));
    memcpy(packet.data.data(), &beacon, sizeof(beacon));
    packet.header.chanin  = 0;
    packet.header.chanout = Artemis::Channels::Channel_ID::RFM23_CHANNEL;
    route_packet_to_rfm23(packet);
  }
}
}