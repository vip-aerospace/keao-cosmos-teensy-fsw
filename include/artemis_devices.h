#ifndef _ARTEMIS_DEVICES_H
#define _ARTEMIS_DEVICES_H

#include "artemisbeacons.h"
#include "config/artemis_defs.h"
#include "helpers.h"
#include "pdu.h"
#include <Adafruit_GPS.h>
#include <Adafruit_INA219.h>
#include <Adafruit_LIS3MDL.h>
#include <Adafruit_LSM6DSOX.h>
#include <Adafruit_Sensor.h>
#include <InternalTemperature.h>
#include <SD.h>
#include <support/configCosmosKernel.h>

namespace Artemis {
  /** @brief The devices and sensors in the satellite. */
  namespace Devices {
    /** @brief The satellite's magnetometer. */
    class Magnetometer {
    public:
      /** @brief The structure of a magnetometer beacon. */
      struct __attribute__((packed)) magbeacon {
        /** @brief The type of beacon. */
        BeaconType type = BeaconType::MagnetometerBeacon;
        /** @brief A decimal identifier for the beacon. */
        uint32_t   deci = 0;
        /** @brief The magnetometer reading for the x axis. */
        float      magx = 0;
        /** @brief The magnetometer reading for the y axis. */
        float      magy = 0;
        /** @brief The magnetometer reading for the z axis. */
        float      magz = 0;
      };
      /**<  A diagram of the struct is included below.
       *
       * @verbatim
 1 byte   4 bytes  4 bytes  4 bytes  4 bytes
+--------+--------+--------+--------+--------+
|  type  |  deci  |  magx  |  magy  |  magz  |
+--------+--------+--------+--------+--------+
         @endverbatim
      */

      /**
       * @brief The core sensor object.
       *
       * The Magnetometer class is a wrapper around the [Adafruit
       * LIS3MDL](https://github.com/adafruit/Adafruit_LIS3MDL) magnetometer
       * object.
       */
      Adafruit_LIS3MDL *magnetometer = new Adafruit_LIS3MDL();

      bool              setup(void);
      bool              read(uint32_t uptime);

    private:
      /** @brief Whether the I2C connection has been set up. */
      bool magnetometerSetup;
    };

    /** @brief The satellite's Inertial Measurement Unit (IMU). */
    class IMU {
    public:
      /** @brief The structure of an IMU beacon. */
      struct __attribute__((packed)) imubeacon {
        /** @brief The type of beacon. */
        BeaconType type    = BeaconType::IMUBeacon;
        /** @brief A decimal identifier for the beacon. */
        uint32_t   deci    = 0;
        /** @brief The accelerometer reading for the x axis. */
        float      accelx  = 0;
        /** @brief The accelerometer reading for the y axis. */
        float      accely  = 0;
        /** @brief The accelerometer reading for the z axis. */
        float      accelz  = 0;
        /** @brief The gyroscope reading for the x axis. */
        float      gyrox   = 0;
        /** @brief The gyroscope reading for the x axis. */
        float      gyroy   = 0;
        /** @brief The gyroscope reading for the y axis. */
        float      gyroz   = 0;
        /** @brief The temperature, in Celcius, of the IMU. */
        float      imutemp = 0;
      };
      /**<  A diagram of the struct is included below.
       *
       * @verbatim
 1 byte   4 bytes  4 bytes    4 bytes    4 bytes    4 bytes   4 bytes
+--------+--------+----------+----------+----------+---------+---------+
|  type  |  deci  |  accelx  |  accely  | accelz   |  gyrox  |  gyroy  |
+--------+--------+----------+----------+----------+---------+---------+
 4 bytes   4 bytes
+--------+---------+
| gyroz  | imutemp |
+--------+---------+
         @endverbatim
       */

      /**
       * @brief The core sensor object.
       *
       * The IMU class is a wrapper around the [Adafruit
       * LSM6DSOX](https://learn.adafruit.com/lsm6dsox-and-ism330dhc-6-dof-imu/)
       * Inertial Measurement Unit (IMU) object.
       */
      Adafruit_LSM6DSOX *imu = new Adafruit_LSM6DSOX();

      bool               setup(void);
      bool               read(uint32_t uptime);

    private:
      /** @brief Whether the I2C connection has been set up. */
      bool imuSetup;
    };

    /** @brief The current sensors on the satellite. */
    class CurrentSensors {
    public:
      /** @brief The first current beacon structure. */
      struct __attribute__((packed)) currentbeacon1 {
        /** @brief The type of the beacon. */
        BeaconType type = BeaconType::CurrentBeacon1;
        /** @brief A decimal identifier for the beacon. */
        uint32_t   deci = 0;
        /** @brief The voltage data. */
        float      busvoltage[ARTEMIS_CURRENT_BEACON_1_COUNT];
        /** @brief The current data. */
        float      current[ARTEMIS_CURRENT_BEACON_1_COUNT];
      };
      /**<  A diagram of the struct is included below.
       *
       * @verbatim
 1 byte 4 bytes 4*X bytes      4*X bytes
+------+-------+--------------+-----------+
| type | deci  | busvoltage[] | current[] |
+------+-------+--------------+-----------+
(Note: X = ARTEMIS_CURRENT_BEACON_1_COUNT)
        @endverbatim
      */

      /** @brief The second current beacon structure. */
      struct __attribute__((packed)) currentbeacon2 {
        /** @brief The type of the beacon. */
        BeaconType type = BeaconType::CurrentBeacon2;
        /** @brief A decimal identifier for the beacon. */
        uint32_t   deci = 0;
        /** @brief The voltage data. */
        float      busvoltage[ARTEMIS_CURRENT_SENSOR_COUNT -
                         ARTEMIS_CURRENT_BEACON_1_COUNT];
        /** @brief The current data. */
        float      current[ARTEMIS_CURRENT_SENSOR_COUNT -
                      ARTEMIS_CURRENT_BEACON_1_COUNT];
      };
      /**<  A diagram of the struct is included below.
       *
       * @verbatim
 1 byte 4 bytes 4*X bytes      4*X bytes
+------+-------+--------------+-----------+
| type | deci  | busvoltage[] | current[] |
+------+-------+--------------+-----------+
(Note: X = ARTEMIS_CURRENT_SENSOR_COUNT - ARTEMIS_CURRENT_BEACON_1_COUNT)
@endverbatim
       */

      /**
       * @brief Instantiation and mapping of core sensor objects.
       *
       * The CurrentSensors class is a wrapper around the [Adafruit
       * INA219
       * ](https://learn.adafruit.com/adafruit-ina219-current-sensor-breakout)
       * current sensor object.
       */
      std::map<std::string, Adafruit_INA219 *> current_sensors = {
          {"solar_panel_1", new Adafruit_INA219(0x40)},
          {"solar_panel_2", new Adafruit_INA219(0x41)},
          {"solar_panel_3", new Adafruit_INA219(0x42)},
          {"solar_panel_4", new Adafruit_INA219(0x43)},
          {"battery_board", new Adafruit_INA219(0x44)},
      };

      bool setup(void);
      void read(uint32_t uptime);

    private:
      /**
       * @brief Whether the I2C connection has been set up.
       *
       * @todo check if this is necessary
       */
      bool currentSetup;
    };

    /** @brief The temperature sensors on the satellite. */
    class TemperatureSensors {
    public:
      /** @brief The temperature beacon structure. */
      struct __attribute__((packed)) temperaturebeacon {
        /** @brief The type of the beacon. */
        BeaconType type = BeaconType::TemperatureBeacon;
        /** @brief A decimal identifier for the beacon. */
        uint32_t   deci = 0;
        /** @brief The temperature data for each TMP36 sensor. */
        float      tmp36_tempC[ARTEMIS_TEMP_SENSOR_COUNT];
        /** @brief The temperature of the Teensy's processor. */
        float      teensy_tempC;
      };
      /**<  A diagram of the struct is included below.
       *
       * @verbatim
 1 byte  4 bytes 4*X bytes              4 bytes
+-------+-------+----------------------+---------------------+
| type  | deci  | tmp36_temperatureC[] | teensy_temperatureC |
+-------+-------+----------------------+---------------------+
(Note: X = ARTEMIS_TEMP_SENSOR_COUNT)
      @endverbatim
      */

      /** @brief Mapping of temperature sensor names and analog pins. */
      std::map<std::string, int> temp_sensors = {
          {          "obc",  A0},
          {          "pdu",  A1},
          {"battery_board",  A6},
          {"solar_panel_1",  A7},
          {"solar_panel_2",  A8},
          {"solar_panel_3",  A9},
          {"solar_panel_4", A17},
      };

      void setup(void);
      void read(uint32_t uptime);
    };

    /** @brief The satellite's Global Positioning System (GPS). */
    class GPS {
    public:
      /** @brief The GPS beacon structure. */
      struct __attribute__((packed)) gpsbeacon {
        /** @brief The type of the beacon. */
        BeaconType type       = BeaconType::GPSBeacon;
        /** @brief A decimal identifier for the beacon. */
        uint32_t   deci       = 0;
        /** @brief The latitude reading in decimal degrees. */
        float      latitude   = 0;
        /** @brief The longitude reading in decimal degrees. */
        float      longitude  = 0;
        /** @brief The ground speed reading, in knots. */
        float      speed      = 0;
        /** @brief The course from true north in degrees. */
        float      angle      = 0;
        /** @brief The altitude reading in meters above Mean Sea Level. */
        float      altitude   = 0;
        /** @brief The number of satellites in use. */
        uint8_t    satellites = 0;
      };
      /**<  A diagram of the struct is included below.
       *
       * @verbatim
 1 byte 4 bytes 4 bytes    4 bytes     4 bytes 4 bytes 4 bytes    1 byte
+------+-------+----------+-----------+-------+-------+----------+------------+
| type | deci  | latitude | longitude | speed | angle | altitude | satellites |
+------+-------+----------+-----------+-------+-------+----------+------------+
      @endverbatim
      */

      /**
       * @brief The core sensor object.
       *
       * The Magnetometer class is a wrapper around the [Adafruit
       * GPS](https://learn.adafruit.com/adafruit-ultimate-gps) object.
       */
      Adafruit_GPS *gps = new Adafruit_GPS(&Serial7);

      bool          setup(void);
      void          update(void);
      void          read(uint32_t uptime);

    private:
      /** @brief Whether the serial connection has been set up. */
      bool gpsSetup;
    };

    /** @brief The switches on the PDU of the satellite. */
    class Switches {
    public:
      /**
       * @brief The PDU switches beacon structure.
       */
      struct __attribute__((packed)) switchbeacon {
        /** @brief The type of the beacon. */
        BeaconType type = BeaconType::SwitchBeacon;
        /** @brief A decimal identifier for the beacon. */
        uint32_t   deci = 0;
        /** @brief The switch states.*/
        uint8_t    sw[NUMBER_OF_SWITCHES + 1];
      };
      /**<  A diagram of the struct is included below.
 *
 * @verbatim
 1 byte  4 bytes 4*X bytes
+-------+-------+------+
| type  | deci  | sw[] |
+-------+-------+------+
(Note: X = NUMBER_OF_SWITCHES + 1)
@endverbatim
*/
    };
  } // namespace Devices
} // namespace Artemis

#endif //_ARTEMIS_DEVICES_H
