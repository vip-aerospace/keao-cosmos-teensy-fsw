/**
 * @file pdu.h
 * @brief The header file for the PDU class.
 *
 * This file contains declarations for the PDU class.
 */
#ifndef _PDU_H
#define _PDU_H

#include "helpers.h"
#include "support/configCosmosKernel.h"
#include <Arduino.h>
#include <TeensyThreads.h>
#include <stdint.h>

/** @brief The offset between PDU character values and ASCII values. */
#define PDU_CMD_OFFSET            48
/** @brief The number of switches on the PDU. */
#define NUMBER_OF_SWITCHES        12

/** @brief The time given to let the PDU warm up. */
#define PDU_WARMUP_TIME           5 * SECONDS
/** @brief The time given to let the PDU reply to a packet before retrying. */
#define PDU_RETRY_INTERVAL        1 * SECONDS
/** @brief The time given to keep the burn wire on. */
#define BURN_WIRE_ON_TIME         5 * SECONDS
/** @brief The time to wait before starting the deployment routine. */
#define DEPLOYMENT_DELAY          5 * SECONDS
/**
 * @brief The length of the deployment.
 *
 * Flight: two weeks in milliseconds = 14 * 24 * 60 * 60 * 1000;
 * Testing: set desired deployment length in seconds
 */
#define DEPLOYMENT_LENGTH         60 * SECONDS
/** @brief The time to wait between iterations of the deployment loop. */
#define DEPLOYMENT_LOOP_INTERVAL  10 * SECONDS
/** @brief The interval at which to check the satellite's temperature. */
#define HEATER_CHECK_INTERVAL     60 * SECONDS
/** @brief The maximum time given to send a packet to the PDU. */
#define PDU_COMMUNICATION_TIMEOUT 5000

namespace Artemis {
  namespace Devices {
    /** @brief The PDU class. */
    class PDU {
    public:
      /** @brief Enumeration of PDU packet types. */
      enum class PDU_Type : uint8_t {
        NOP,
        CommandPing,
        CommandSetSwitch,
        CommandGetSwitchStatus,
        DataPong,
        DataSwitchStatus,
        DataSwitchTelem,
      };
      /** @brief Enumeration of PDU switches. */
      enum class PDU_SW : uint8_t {
        None,
        All,
        SW_3V3_1,
        SW_3V3_2,
        SW_5V_1,
        SW_5V_2,
        SW_5V_3,
        SW_5V_4,
        SW_12V,
        VBATT,
        WDT,
        HBRIDGE1,
        HBRIDGE2,
        BURN,
        BURN1,
        BURN2,
        RPI,
      };
      /** @brief Enumeration of PDU switch state. */
      enum class PDU_SW_State : bool {
        SWITCH_OFF,
        SWITCH_ON,
      };
      /** @brief Mapping between PDU switches and their string names. */
      std::map<std::string, PDU_SW> PDU_SW_Type = {
          {     "all",      PDU_SW::All},
          {   "3v3_1", PDU_SW::SW_3V3_1},
          {   "3v3_2", PDU_SW::SW_3V3_2},
          {    "5v_1",  PDU_SW::SW_5V_1},
          {    "5v_2",  PDU_SW::SW_5V_2},
          {    "5v_3",  PDU_SW::SW_5V_3},
          {    "5v_4",  PDU_SW::SW_5V_4},
          {     "12v",   PDU_SW::SW_12V},
          {   "vbatt",    PDU_SW::VBATT},
          {     "wdt",      PDU_SW::WDT},
          {"hbridge1", PDU_SW::HBRIDGE1},
          {"hbridge2", PDU_SW::HBRIDGE2},
          {    "burn",     PDU_SW::BURN},
          {   "burn1",    PDU_SW::BURN1},
          {   "burn2",    PDU_SW::BURN2},
          {     "rpi",      PDU_SW::RPI},
      };

      /** @brief The PDU packet structure. */
      struct __attribute__((packed)) pdu_packet {
        PDU_Type type     = PDU_Type::NOP;
        PDU_SW   sw       = PDU_SW::None;
        uint8_t  sw_state = 0;
      };
      /** @brief The PDU telemetry packet structure. */
      struct __attribute__((packed)) pdu_telem {
        PDU_Type type = PDU_Type::DataSwitchTelem;
        uint8_t  sw_state[NUMBER_OF_SWITCHES];
      };

      PDU(HardwareSerial *hw_serial, int baud_rate);

      bool         ping();
      bool         set_switch(PDU_SW sw, PDU_SW_State state);
      bool         set_heater(PDU_SW_State state);
      bool         set_burn_wire(PDU_SW_State state);
      bool         refresh_switch_states();

      /**
       * @brief The status of each switch on the PDU.
       *
       * @todo Make this private.
       */
      PDU_SW_State switch_states[NUMBER_OF_SWITCHES];

    private:
      /** @brief The serial connection used to communicate with the PDU. */
      HardwareSerial *serial;

      bool            send(pdu_packet packet);
      bool            recv(pdu_packet *packet);
      bool            recv(pdu_telem *packet);
    };
  } // namespace Devices
} // namespace Artemis

#endif
