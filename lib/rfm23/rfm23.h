/**
 * @file rfm23.h
 * @brief The header file for the RFM23 radio class.
 *
 * This file contains declarations for the RFM23 radio class.
 */
#ifndef _RFM23_H
#define _RFM23_H

#include "helpers.h"
#include <RHHardwareSPI1.h>
#include <RH_RF22.h>
#include <TeensyThreads.h>
#include <support/packetcomm.h>

#undef RH_RF22_MAX_MESSAGE_LEN
/** @brief Overrides the default maximum message length. */
#define RH_RF22_MAX_MESSAGE_LEN 50
/** @brief The minimum time to wait for a reply from the radio. */
#define MINIMUM_TIMEOUT         100

namespace Artemis {
  namespace Devices {
    /** @brief The RFM23 radio class. */
    class RFM23 {
    public:
      /** @brief The RFM23 radio configuration. */
      struct __attribute__((packed)) rfm23_config {
        /** @brief The receive/transmit center frequency. */
        uint16_t freq;
        /** @brief The transmit power, set as a macro. */
        uint8_t  tx_power;
        /** @brief The pins connecting the RFM23 and Teensy. */
        struct {
          /** @brief The MISO pin used on the SPI interface. */
          uint8_t spi_miso;
          /** @brief The MOSI pin used on the SPI interface. */
          uint8_t spi_mosi;
          /** @brief The clock pin used on the SPI interface. */
          uint8_t spi_sck;
          /** @brief The interrupt pin. */
          uint8_t nirq;
          /** @brief The chip select pin used on the SPI interface. */
          uint8_t cs;
          /** @brief The transmit enable pin. */
          uint8_t tx_on;
          /** @brief The receive enable pin. */
          uint8_t rx_on;
        } pins;
      };

      RFM23(uint8_t slaveSelectPin, uint8_t interruptPin,
            RHGenericSPI &spi = hardware_spi1);
      bool    init(rfm23_config cfg, Threads::Mutex *mtx);
      void    reset();
      bool    send(PacketComm &packet);
      int32_t recv(PacketComm &packet, uint16_t timeout);

    private:
      /**
       * @brief The core radio object.
       *
       * The RFM23 class is a wrapper around the [RadioHead
       * RH_RF22](http://www.airspayce.com/mikem/arduino/RadioHead/classRH__RF22.html)
       * object.
       */
      RH_RF22         rfm23;
      /** @brief The mutex used to lock the SPI interface to the radio. */
      Threads::Mutex *spi_mtx;
      /** @brief The configuration of the RFM23 class. */
      rfm23_config    config;
    };
  } // namespace Devices
} // namespace Artemis

#endif // _RFM23_H
