#ifndef _ARTEMIS_DEVICES_H
#define _ARTEMIS_DEVICES_H

#include "artemisbeacons.h"
#include "config/artemis_defs.h"
#include <Adafruit_GPS.h>
#include <Adafruit_INA219.h>
#include <Adafruit_LIS3MDL.h>
#include <Adafruit_LSM6DSOX.h>
#include <Adafruit_Sensor.h>
#include <InternalTemperature.h>
#include <SD.h>
#include <support/configCosmosKernel.h>

namespace Artemis {
  namespace Devices {
    class Magnetometer {
    public:
      struct __attribute__((packed)) magbeacon {
        BeaconType type = BeaconType::MagnetometerBeacon;
        uint32_t   deci = 0;
        float      magx = 0;
        float      magy = 0;
        float      magz = 0;
      };
      Adafruit_LIS3MDL *magnetometer = new Adafruit_LIS3MDL();

      int32_t           setup(void);
      int32_t           read(uint32_t uptime);

    private:
      bool magnetometerSetup;
    };

    class IMU {
    public:
      struct __attribute__((packed)) imubeacon {
        BeaconType type    = BeaconType::IMUBeacon;
        uint32_t   deci    = 0;
        float      accelx  = 0;
        float      accely  = 0;
        float      accelz  = 0;
        float      gyrox   = 0;
        float      gyroy   = 0;
        float      gyroz   = 0;
        float      imutemp = 0;
      };
      Adafruit_LSM6DSOX *imu = new Adafruit_LSM6DSOX();

      int32_t            setup(void);
      int32_t            read(uint32_t uptime);

    private:
      bool imuSetup;
    };

    class GPS {
    public:
      struct __attribute__((packed)) gpsbeacon {
        BeaconType type       = BeaconType::GPSBeacon;
        uint32_t   deci       = 0;
        float      latitude   = 0;
        float      longitude  = 0;
        float      speed      = 0;
        float      angle      = 0;
        float      altitude   = 0;
        uint8_t    satellites = 0;
      };
      Adafruit_GPS *gps = new Adafruit_GPS(&Serial7);

      int32_t       setup(void);
      int32_t       update(void);
      int32_t       read(uint32_t uptime);

    private:
      bool gpsSetup;
    };

    class CurrentSensors {
    public:
      struct __attribute__((packed)) currentbeacon1 {
        BeaconType type = BeaconType::CurrentBeacon1;
        uint32_t   deci = 0;
        float      busvoltage[ARTEMIS_CURRENT_BEACON_1_COUNT];
        float      current[ARTEMIS_CURRENT_BEACON_1_COUNT];
      };
      struct __attribute__((packed)) currentbeacon2 {
        BeaconType type = BeaconType::CurrentBeacon2;
        uint32_t   deci = 0;
        float      busvoltage[ARTEMIS_CURRENT_SENSOR_COUNT -
                         ARTEMIS_CURRENT_BEACON_1_COUNT];
        float      current[ARTEMIS_CURRENT_SENSOR_COUNT -
                      ARTEMIS_CURRENT_BEACON_1_COUNT];
      };

      std::map<std::string, Adafruit_INA219 *> current_sensors = {
          {"solar_panel_1", new Adafruit_INA219(0x40)},
          {"solar_panel_2", new Adafruit_INA219(0x41)},
          {"solar_panel_3", new Adafruit_INA219(0x42)},
          {"solar_panel_4", new Adafruit_INA219(0x43)},
          {"battery_board", new Adafruit_INA219(0x44)},
      };

      int32_t setup(void);
      int32_t read(uint32_t uptime);

    private:
      bool currentSetup;
    };

    class TemperatureSensors {
    public:
      struct __attribute__((packed)) temperaturebeacon {

        BeaconType type = BeaconType::TemperatureBeacon;
        uint32_t   deci = 0;
        float      tmp36_tempC[ARTEMIS_TEMP_SENSOR_COUNT];
        float      teensy_tempC;
      };

      std::map<std::string, int> temp_sensors = {
          {          "obc",  A0},
          {          "pdu",  A1},
          {"battery_board",  A6},
          {"solar_panel_1",  A7},
          {"solar_panel_2",  A8},
          {"solar_panel_3",  A9},
          {"solar_panel_4", A17},
      };

      int32_t setup(void);
      int32_t read(uint32_t uptime);

    private:
      bool temperatureSetup;
    };

    class Switches {
    public:
      struct __attribute__((packed)) switchbeacon {
        BeaconType type = BeaconType::SwitchBeacon;
        uint32_t   deci = 0;
        uint8_t    sw[13];
      };
    };
  } // namespace Devices
} // namespace Artemis

#endif //_ARTEMIS_DEVICES_H
