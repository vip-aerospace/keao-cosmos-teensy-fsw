#ifndef _ARTEMISCHANNELS_H
#define _ARTEMISCHANNELS_H

#include <artemis_defs.h>
#include <TeensyThreads.h>
#include <rfm23.h>
#include <Wire.h>
#include <Adafruit_LIS3MDL.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_LSM6DSOX.h>
#include <Adafruit_INA219.h>
#include <pdu.h>
#include <support/packetcomm.h>

namespace Artemis
{
    namespace Teensy
    {
        namespace Channels
        {
            void rfm23_channel();
            void pdu_channel();
        }
    }
}

#endif
