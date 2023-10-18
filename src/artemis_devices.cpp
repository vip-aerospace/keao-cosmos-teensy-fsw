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
      if (!magnetometer->begin_I2C()) {
        return false;
      }
      magnetometer->setPerformanceMode(LIS3MDL_LOWPOWERMODE);
      magnetometer->setDataRate(LIS3MDL_DATARATE_0_625_HZ);
      magnetometer->setRange(LIS3MDL_RANGE_16_GAUSS);
      magnetometer->setOperationMode(LIS3MDL_CONTINUOUSMODE);
      return true;
    }

    /**
     * @brief Reads the satellite's magnetometer.
     *
     * This method of the Magnetometer class reads the magnetometer's values,
     * stores it in a magbeacon, and transmits that beacon to the ground.
     *
     * @param uptime The time, in milliseconds, since the Teensy has been
     * powered on.
     * @return true The magnetometer has been sucessfully read and a packet
     * carrying the reading has been queued for transmission.
     * @return false The magnetometer could not be read.
     */
    bool Magnetometer::read(uint32_t uptime) {
      PacketComm packet;
      magbeacon  beacon;
      beacon.deci = uptime;

      sensors_event_t event;
      if (!magnetometer->getEvent(&event)) {
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
      PushQueue(packet, rfm23_queue, rfm23_queue_mtx);

      return true;
    }

    /**
     * @brief Sets up the satellite's IMU.
     *
     * This method of the IMU class sets up the I2C connection to the
     * satellite's IMU and applies settings to it.
     *
     * @return true The IMU has been successfully set up.
     * @return false The I2C connection to the IMU failed to start.
     */
    bool IMU::setup(void) {
      if (!imu->begin_I2C()) {
        return false;
      }
      imu->setAccelRange(LSM6DS_ACCEL_RANGE_16_G);
      imu->setGyroRange(LSM6DS_GYRO_RANGE_2000_DPS);
      imu->setAccelDataRate(LSM6DS_RATE_6_66K_HZ);
      imu->setGyroDataRate(LSM6DS_RATE_6_66K_HZ);

      return true;
    }

    /**
     * @brief Reads the satellite's IMU.
     *
     * This method of the IMU class reads the IMU's values, stores it in an
     * imubeacon, and transmits that beacon to the ground.
     *
     * @param uptime The time, in milliseconds, since the Teensy has been
     * powered on.
     * @return true The IMU has been sucessfully read and a packet carrying the
     * reading has been queued for transmission.
     * @return false The IMU could not be read.
     */
    bool IMU::read(uint32_t uptime) {
      PacketComm packet;
      imubeacon  beacon;
      beacon.deci = uptime;

      sensors_event_t accel;
      sensors_event_t gyro;
      sensors_event_t temp;
      if (!imu->getEvent(&accel, &gyro, &temp)) {
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
      PushQueue(packet, rfm23_queue, rfm23_queue_mtx);

      return true;
    }

    /**
     * @brief Sets up the satellite's current sensors.
     *
     * This method of the CurrentSensors class sets up the I2C connection to the
     * satellite's current sensors.
     *
     * @todo Go through library and see what we need to configure and callibrate
     *
     * @return true All current sensors in current_sensors have been connected
     * to over I2C.
     * @return false At least one current sensor in current_sensors failed to
     * initialize over I2C.
     */
    bool CurrentSensors::setup(void) {
      for (auto &current_sensor : current_sensors) {
        if (!current_sensor.second->begin(&Wire2)) {
          return false;
        }
      }

      return true;
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
          beacon1.busvoltage[i] =
              (current_sensors[it.first]->getBusVoltage_V());
          beacon1.current[i] = (current_sensors[it.first]->getCurrent_mA());
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
      PushQueue(packet, rfm23_queue, rfm23_queue_mtx);

      beacon2.deci = uptime;
      packet.data.resize(sizeof(beacon2));
      memcpy(packet.data.data(), &beacon2, sizeof(beacon2));
      packet.header.chanin  = 0;
      packet.header.chanout = Artemis::Channels::Channel_ID::RFM23_CHANNEL;
      PushQueue(packet, rfm23_queue, rfm23_queue_mtx);
    }
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
     * a temperature in Celcius. The Teensy's internal temperature sensor is
     * also read.
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
      PushQueue(packet, rfm23_queue, rfm23_queue_mtx);
    }

    /**
     * @brief Sets up the satellite's GPS.
     *
     * This method of the GPS class sets up the serial connection to the
     * satellite's GPS and applies settings to it.
     *
     * @return true The GPS has been sucessfully set up.
     * @return false The serial connection to the GPS failed to start.
     */
    bool GPS::setup(void) {
      if (!gps->begin(9600)) {
        return false;
      }
      threads.delay(100);
      gps->sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
      threads.delay(100);
      gps->sendCommand(PMTK_SET_NMEA_UPDATE_1HZ); // 1 Hz update rate
      threads.delay(100);

      return true;
    }

    /**
     * @brief Update the satellite's GPS.
     *
     * This method of the GPS class checks the serial connection to see if there
     * is more data to be read in. If there is, it is read in and parsed.
     */
    void GPS::update(void) {
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
        beacon.latitude        = 0;
        beacon.longitude       = 0;
        beacon.speed           = 0;
        beacon.angle           = 0;
        beacon.altitude        = 0;
        beacon.satellites      = 0;

        packet.header.nodeorig = (uint8_t)NODES::TEENSY_NODE_ID;
        packet.header.nodedest = (uint8_t)NODES::GROUND_NODE_ID;
        packet.header.type     = PacketComm::TypeId::DataObcBeacon;
        packet.data.resize(sizeof(beacon));
        memcpy(packet.data.data(), &beacon, sizeof(beacon));
        packet.header.chanin  = 0;
        packet.header.chanout = Artemis::Channels::Channel_ID::RFM23_CHANNEL;
        PushQueue(packet, rfm23_queue, rfm23_queue_mtx);
      }
    }
  } // namespace Devices

} // namespace Artemis