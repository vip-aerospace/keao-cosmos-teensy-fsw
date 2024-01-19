/**
 * @file artemisbeacons.h
 * @brief Definition of Artemis beacon types.
 *
 * This file defines the types of beacons used throughout the satellite.
 */
#ifndef _ARTEMIS_BEACONS_H
#define _ARTEMIS_BEACONS_H

#include "config/artemis_defs.h"
#include <cstdint>

namespace Artemis {
  namespace Devices {
    /** @brief Enumeration of beacon types. */
    enum class BeaconType : uint8_t {
      None,
      TemperatureBeacon,
      CurrentBeacon1,
      CurrentBeacon2,
      IMUBeacon,
      MagnetometerBeacon,
      GPSBeacon,
      SwitchBeacon,
    };
  } // namespace Devices
} // namespace Artemis

#endif // _ARTEMIS_BEACONS_H
