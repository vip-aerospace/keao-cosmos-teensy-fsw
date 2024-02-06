/**
 * @file imu.cpp
 * @brief Definition of the Artemis IMU class.
 *
 * This file defines the methods for the IMU object.
 */
#include "artemis_devices.h"
#include "channels/artemis_channels.h"

namespace Artemis {
namespace Devices {
  /**
   * @brief Sets up the satellite's IMU.
   *
   * This method of the IMU class sets up the I2C connection to the satellite's
   * IMU and applies settings to it.
   *
   * @return true The IMU has been successfully set up.
   * @return false The I2C connection to the IMU failed to start.
   */
  bool IMU::setup(void) {
    if ((imuSetup = imu->begin_I2C())) {
      imu->setAccelRange(LSM6DS_ACCEL_RANGE_16_G);
      imu->setGyroRange(LSM6DS_GYRO_RANGE_2000_DPS);
      imu->setAccelDataRate(LSM6DS_RATE_6_66K_HZ);
      imu->setGyroDataRate(LSM6DS_RATE_6_66K_HZ);
    }
    return imuSetup;
  }

  /**
   * @brief Reads the satellite's IMU.
   *
   * This method of the IMU class reads the IMU's values, stores it in an
   * imubeacon, and transmits that beacon to the ground.
   *
   * @param uptime The time, in milliseconds, since the Teensy has been
   * powered on.
   * @return true The IMU has been successfully read and a packet carrying the
   * reading has been queued for transmission.
   * @return false The IMU could not be read.
   */
  bool IMU::read(uint32_t uptime) {
    if (!imuSetup) {
      setup();
    }

    PacketComm packet;
    imubeacon  beacon;
    beacon.deci = uptime;

    sensors_event_t accel;
    sensors_event_t gyro;
    sensors_event_t temp;
    if (!imu->getEvent(&accel, &gyro, &temp) || !imuSetup) {
      return false;
    }

    beacon.accelx          = (accel.acceleration.x);
    beacon.accely          = (accel.acceleration.y);
    beacon.accelz          = (accel.acceleration.z);
    beacon.gyrox           = (gyro.gyro.x);
    beacon.gyroy           = (gyro.gyro.y);
    beacon.gyroz           = (gyro.gyro.z);
    beacon.imutemp         = (temp.temperature);

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