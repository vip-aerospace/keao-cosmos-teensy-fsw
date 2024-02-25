/**
 * @file gps.cpp
 * @brief Definition of the Artemis GPS class.
 *
 * This file defines the methods for the GPS object.
 */
#include "artemis_devices.h"
#include "channels/artemis_channels.h"

namespace Artemis {
namespace Devices {
  /**
   * @brief Sets up the satellite's GPS.
   *
   * This method of the GPS class sets up the serial connection to the
   * satellite's GPS and applies settings to it.
   *
   * @return true The GPS has been successfully set up.
   * @return false The serial connection to the GPS failed to start.
   */
  bool GPS::setup(void) {
    if ((gpsSetup = gps->begin(9600))) {
      threads.delay(100);
      gps->sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
      threads.delay(100);
      gps->sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);
      threads.delay(100);
    }
    return gpsSetup;
  }

  /**
   * @brief Update the satellite's GPS.
   *
   * This method of the GPS class checks the serial connection to see if there
   * is more data to be read in. If there is, it is read in and parsed.
   */
  void GPS::update(void) {
    if (!gpsSetup) {
      return;
    }
    if (gps->available()) {
      while (gps->read()) // Clear any data from the GPS module
        ;
    }

    if (gps->newNMEAreceived()) // Check to see if a new NMEA line has been
                                // received
    {
      if (gps->parse(gps->lastNMEA())) // A successful message was parsed
      {
        Helpers::print_debug(Helpers::MAIN, "Parsed new NMEA sentence");
      }
    }
  }
  /**
   * @brief Reads the satellite's GPS data.
   *
   * This method of the GPS class reads the last known GPS data, stores it in
   * a gpsbeacon, and transmits that beacon to ground.
   *
   * @param uptime The time, in milliseconds, since the Teensy has been
   * powered on.
   */
  void GPS::read(uint32_t uptime) {
    if (!gpsSetup) {
      setup();
    }

    PacketComm packet;
    gpsbeacon  beacon;
    beacon.deci = uptime;

    if (gps->fix) {
      // beacon.hour = gps->hour;
      // beacon.minute = gps->minute;
      // beacon.seconds = gps->seconds;
      // beacon.milliseconds = gps->milliseconds;
      // beacon.day = gps->day;
      // beacon.month = gps->month;
      // beacon.year = gps->year;
      beacon.latitude   = gps->latitude;
      beacon.longitude  = gps->longitude;
      beacon.speed      = gps->speed;
      beacon.angle      = gps->angle;
      beacon.altitude   = gps->altitude;
      beacon.satellites = gps->satellites;
    } else {
      // beacon.hour = 0;
      // beacon.minute = 0;
      // beacon.seconds = 0;
      // beacon.milliseconds = 0;
      // beacon.day = 0;
      // beacon.month = 0;
      // beacon.year = 0;
      beacon.latitude   = 0;
      beacon.longitude  = 0;
      beacon.speed      = 0;
      beacon.angle      = 0;
      beacon.altitude   = 0;
      beacon.satellites = 0;
    }
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