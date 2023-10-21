/**
 * @file pdu_channel.cpp
 * @brief The PDU channel.
 *
 * The definition of the PDU channel.
 */
#include "artemis_devices.h"
#include "artemisbeacons.h"
#include "channels/artemis_channels.h"
#include <SD.h>
#include <pdu.h>

namespace Artemis {
  namespace Channels {
    /** @brief The PDU channel. */
    namespace PDU {
      using Artemis::Devices::PDU;
      /** @brief The packet used throughout the channel. */
      PacketComm    packet;
      /** @brief The PDU object used throughout the channel. */
      PDU           pdu(&Serial1, 115200);
      /** @brief The time at which an action has started.*/
      unsigned long startTime;
      /** @brief The time in milliseconds since the channel was started.*/
      elapsedMillis uptime;
      /** @brief The time in milliseconds since the temperature was checked. */
      elapsedMillis heaterinterval;

      /**
       * @brief The top-level channel definition.
       *
       * This is the function that defines the PDU channel. Like an Arduino
       * script, it has a setup() function that is run once, then loop() runs
       * forever.
       */
      void          pdu_channel() {
        setup();
        loop();
      }

      /**
       * @brief The PDU setup function.
       *
       * This function is run once, when the channel is started. It connects to
       * the PDU over a serial connection, tests the connection, then sets the
       * PDU's switch states.
       */
      void setup() {
        print_debug(Helpers::PDU, "PDU channel starting...");
        while (!Serial1) {
        }
        // Give the PDU some time to warm up...
        threads.delay(PDU_WARMUP_TIME);

        // Ensure PDU is communicating with Teensy
        while (!pdu.ping()) {
          print_debug(Helpers::PDU, "Unable to ping PDU");
          threads.delay(PDU_RETRY_INTERVAL);
        }
        print_debug(Helpers::PDU, "PDU connection established");

        while (!pdu.refresh_switch_states()) {
          print_debug(Helpers::PDU, "Unable to refresh PDU switch states");
          threads.delay(PDU_RETRY_INTERVAL);
        }
        print_debug(Helpers::PDU, "PDU switch states refreshed");
        threads.delay(100);

        enableRFM23Radio();

        deploy();

        print_debug(Helpers::PDU, "Satellite is now in passive state.");
      }

      /**
       * @brief Provides power to the RFM23 radio.
       *
       * @todo The radio is connected to the 3V3_2 switch however it is still
       * getting power from somewhere else. Uncommenting these two lines crashes
       * the flight software. troubleshoot harwdare & software.
       */
      void enableRFM23Radio() {
        // pdu.set_switch(Artemis::Teensy::PDU::PDU_SW::SW_3V3_2, true);
        // threads.delay(1000);
      }

      /**
       * @brief Deployment sequence.
       *
       * @todo SD is begun twice. is this needed?
       *
       * @todo try storing burnwire complete and deployment seperately.
       * deployed.txt should only be written after DEPLOYMENT_LENGTH.
       */
      void deploy() {
        if (SD.begin(BUILTIN_SDCARD)) {
          if (!SD.exists("/deployed.txt")) {
            deploymentmode = true;
            threads.delay(DEPLOYMENT_DELAY);

            print_debug(Helpers::PDU, "Starting Deployment Sequence");
            deploy_burn_wire();

            SD.begin(BUILTIN_SDCARD);
            File file = SD.open("/deployed.txt", FILE_WRITE);
            if (file) {
              file.close();
              print_debug(Helpers::PDU, "Deployment recorded on SD card.");
            } else {
              print_debug(Helpers::PDU, "Error opening file");
              return;
            }

            elapsedMillis timeElapsed;
            while (timeElapsed <= DEPLOYMENT_LENGTH) {
              handle_queue();
              regulate_temperature();
              threads.delay(DEPLOYMENT_LOOP_INTERVAL);
            }
          } else {
            print_debug(Helpers::PDU, "Satellite was already deployed");
          }
        }
        deploymentmode = false;
      }

      /** @brief Deploys the burn wire. */
      void deploy_burn_wire() {
        if (!pdu.set_burn_wire(PDU::PDU_SW_State::SWITCH_ON)) {
          print_debug(Helpers::PDU, "Failed to enable burn wire switch");
        }
        print_debug(Helpers::PDU, "Burn switch on");
        threads.delay(BURN_WIRE_ON_TIME);
        if (!pdu.set_burn_wire(PDU::PDU_SW_State::SWITCH_OFF)) {
          print_debug(Helpers::PDU, "Failed to disable burn wire switch");
        }
        print_debug(Helpers::PDU, "Burn switch off");
      }

      /**
       * @brief The PDU loop function.
       *
       * This function runs in an infinite loop after setup() completes. It
       * routes packets going to and coming from the PDU.
       */
      void loop() {
        while (true) {
          handle_queue();
          regulate_temperature();
          update_watchdog_timer();
          threads.delay(100);
        }
      }

      /**
       * @brief Helper function to handle packet queue.
       *
       * This is a helper function called in loop() that checks for packets and
       * routes them to the PDU.
       */
      void handle_queue() {
        if (PullQueue(packet, pdu_queue, pdu_queue_mtx)) {
          print_debug(Helpers::PDU, "Pulled packet of type ",
                      (uint16_t)packet.header.type, " from queue.");
          switch (packet.header.type) {
            case PacketComm::TypeId::CommandEpsCommunicate: {
              test_communicating_with_pdu();
              break;
            }
            case PacketComm::TypeId::CommandEpsSwitchName: {
              set_switch_on_pdu();
            }
            case PacketComm::TypeId::CommandEpsSwitchStatus: {
              report_pdu_switch_status();
              break;
            }
            default:
              break;
          }
        }
      }

      /** @brief Helper function to ping the PDU to test communications. */
      void test_communicating_with_pdu() {
        startTime = millis();
        while (!pdu.ping() &&
               (millis() - startTime) < PDU_COMMUNICATION_TIMEOUT) {
          print_debug_rapid(Helpers::PDU,
                            "Failed to ping PDU. Waiting to retry.");
          threads.delay(PDU_RETRY_INTERVAL);
        }
        if ((millis() - startTime) >= PDU_COMMUNICATION_TIMEOUT) {
          print_debug(Helpers::PDU, "Timed out trying to ping PDU");
        } else {
          packet.header.nodedest = packet.header.nodeorig;
          packet.header.nodeorig = (uint8_t)NODES::TEENSY_NODE_ID;
          route_packet_to_main(packet);
        }
      }

      /** @brief Helper function to set a switch on the PDU. */
      void set_switch_on_pdu() {
        PDU::PDU_SW       switchID    = (PDU::PDU_SW)packet.data[0];
        PDU::PDU_SW_State switchState = (PDU::PDU_SW_State)packet.data[1];

        startTime                     = millis();
        while (!pdu.set_switch(switchID, switchState) &&
               (millis() - startTime) < PDU_COMMUNICATION_TIMEOUT) {
          print_debug_rapid(Helpers::PDU,
                            "Failed to set switch on PDU. Waiting to retry.");
          threads.delay(PDU_RETRY_INTERVAL);
        }
        if ((millis() - startTime) >= PDU_COMMUNICATION_TIMEOUT) {
          print_debug(Helpers::PDU, "Timed out trying to set switch");
        }
      }

      /** @brief Helper function to report status of all switches on PDU. */
      void report_pdu_switch_status() {
        startTime = millis();
        while (!pdu.refresh_switch_states() &&
               (millis() - startTime) < PDU_COMMUNICATION_TIMEOUT) {
          print_debug_rapid(
              Helpers::PDU,
              "Failed to refresh PDU switch states. Waiting to retry.");
          threads.delay(PDU_RETRY_INTERVAL);
        }
        if ((millis() - startTime) >= PDU_COMMUNICATION_TIMEOUT) {
          print_debug(Helpers::PDU,
                      "Timed out trying to refresh PDU switch states");
        } else {
          Devices::Switches::switchbeacon beacon;
          beacon.deci = uptime;
          for (int i = 0; i < NUMBER_OF_SWITCHES; i++) {
            beacon.sw[i] = (uint8_t)pdu.switch_states[i];
          }
          beacon.sw[NUMBER_OF_SWITCHES] = digitalRead(UART6_TX);

          packet.header.type            = PacketComm::TypeId::DataObcBeacon;
          packet.header.nodeorig        = (uint8_t)NODES::TEENSY_NODE_ID;
          packet.header.nodedest        = (uint8_t)NODES::GROUND_NODE_ID;
          packet.data.resize(sizeof(beacon));
          memcpy(packet.data.data(), &beacon, sizeof(beacon));
          packet.header.chanin  = 0;
          packet.header.chanout = Channel_ID::RFM23_CHANNEL;
          route_packet_to_rfm23(packet);
        }
      }

      /** @brief Helper function to regulate the satellite's temperature. */
      void regulate_temperature() {
        if (heaterinterval > HEATER_CHECK_INTERVAL) {
          heaterinterval     = 0;
          int   reading      = analogRead(A6);
          float voltage      = reading * MV_PER_ADC_UNIT;
          float temperatureF = (voltage - OFFSET_F) / MV_PER_DEGREE_F;
          float temperatureC = (temperatureF - 32) * 5 / 9;
          if (temperatureC <= heater_threshold) {
            pdu.set_heater(PDU::PDU_SW_State::SWITCH_ON);
            print_debug(Helpers::PDU, "Heater turned on");
          } else {
            pdu.set_heater(PDU::PDU_SW_State::SWITCH_OFF);
            print_debug(Helpers::PDU, "Heater turned off");
          }
        }
      }

      /** @brief Helper function to feed the PDU's watchdog. */
      void update_watchdog_timer() {
        // pdu.set_switch(Artemis::Teensy::PDU::PDU_SW::WDT, 1);
        // pdu.set_switch(Artemis::Teensy::PDU::PDU_SW::WDT, 0);
      }
    } // namespace PDU
  }   // namespace Channels
} // namespace Artemis
