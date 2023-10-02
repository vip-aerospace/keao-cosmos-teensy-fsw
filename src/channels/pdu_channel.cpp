#include "artemis_devices.h"
#include "artemisbeacons.h"
#include "channels/artemis_channels.h"
#include <SD.h>
#include <pdu.h>

namespace Artemis {
  namespace Channels {
    namespace PDU {
      using Artemis::Devices::PDU;
      PDU           pdu(&Serial1, 115200);
      PacketComm    packet;
      unsigned long timeoutStart;
      elapsedMillis uptime;
      // Create a new elapsedMillis object for the heater
      elapsedMillis heaterinterval;

      void          pdu_channel() {
        setup();
        loop();
      }

      void setup() {
        while (!Serial1)
          ;
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

        // The radio is connected to the 3V3_2 switch however it is still
        // getting power from somewhere else. Uncommenting these two lines
        // crashes the flight software. TODO: troubleshoot harwdare & software.
        // pdu.set_switch(Artemis::Teensy::PDU::PDU_SW::SW_3V3_2, true);
        // threads.delay(1000);

        deploy();

        deploymentmode = false;
        print_debug(Helpers::PDU, "Satellite is now in passive state.");
      }

      void deploy() {
        if (SD.begin(BUILTIN_SDCARD)) {
          if (!SD.exists("/deployed.txt")) {
            deploymentmode = true;
            threads.delay(DEPLOYMENT_DELAY);

            // Enable burn wire
            print_debug(Helpers::PDU, "Starting Deployment Sequence");
            if (!pdu.set_burn_wire(PDU::PDU_SW_State::SWITCH_ON)) {
              print_debug(Helpers::PDU, "Failed to enable burn wire switch");
            }
            print_debug(Helpers::PDU, "Burn switch on");
            threads.delay(BURN_WIRE_ON_TIME);
            if (!pdu.set_burn_wire(PDU::PDU_SW_State::SWITCH_OFF)) {
              print_debug(Helpers::PDU, "Failed to disable burn wire switch");
            }
            print_debug(Helpers::PDU, "Burn switch off");

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
            // issue
            print_debug(Helpers::PDU, "Satellite was already deployed");
          }
        }
      }

      void loop() {
        while (true) {
          handle_queue();
          regulate_temperature();
          update_watchdog_timer();
          threads.delay(100);
        }
      }

      void handle_queue() {
        if (PullQueue(packet, pdu_queue, pdu_queue_mtx)) {
          switch (packet.header.type) {
            case PacketComm::TypeId::CommandEpsCommunicate: {
              timeoutStart = millis();
              while (!pdu.ping() &&
                     (millis() - timeoutStart) < PDU_COMMUNICATION_TIMEOUT) {
                threads.delay(PDU_RETRY_INTERVAL);
              }
              if ((millis() - timeoutStart) >= PDU_COMMUNICATION_TIMEOUT) {
                print_debug(Helpers::PDU, "Timed out trying to ping PDU");
              } else {
                packet.header.nodedest = packet.header.nodeorig;
                packet.header.nodeorig = (uint8_t)NODES::TEENSY_NODE_ID;
                PushQueue(packet, main_queue, main_queue_mtx);
              }
              break;
            }
            case PacketComm::TypeId::CommandEpsSwitchName: {
              PDU::PDU_SW       switchID    = (PDU::PDU_SW)packet.data[0];
              PDU::PDU_SW_State switchState = (PDU::PDU_SW_State)packet.data[1];

              timeoutStart                  = millis();
              while (!pdu.set_switch(switchID, switchState) &&
                     (millis() - timeoutStart) < PDU_COMMUNICATION_TIMEOUT) {
                threads.delay(PDU_RETRY_INTERVAL);
              }
              if ((millis() - timeoutStart) >= PDU_COMMUNICATION_TIMEOUT) {
                print_debug(Helpers::PDU, "Timed out trying to set switch");
              }
            }
            case PacketComm::TypeId::CommandEpsSwitchStatus: {
              timeoutStart = millis();
              while (!pdu.refresh_switch_states() &&
                     (millis() - timeoutStart) < PDU_COMMUNICATION_TIMEOUT) {
                threads.delay(PDU_RETRY_INTERVAL);
              }
              if ((millis() - timeoutStart) >= PDU_COMMUNICATION_TIMEOUT) {
                print_debug(Helpers::PDU,
                            "Timed out trying to refresh PDU switch states");
              } else {
                Devices::Switches::switchbeacon beacon;
                beacon.deci = uptime;
                for (int i = 0; i < NUMBER_OF_SWITCHES; i++) {
                  beacon.sw[i] = (uint8_t)pdu.switch_states[i];
                }
                beacon.sw[NUMBER_OF_SWITCHES] = digitalRead(UART6_TX);

                packet.header.type     = PacketComm::TypeId::DataObcBeacon;
                packet.header.nodeorig = (uint8_t)NODES::TEENSY_NODE_ID;
                packet.header.nodedest = (uint8_t)NODES::GROUND_NODE_ID;
                packet.data.resize(sizeof(beacon));
                memcpy(packet.data.data(), &beacon, sizeof(beacon));
                packet.header.chanin  = 0;
                packet.header.chanout = Channel_ID::RFM23_CHANNEL;
                PushQueue(packet, rfm23_queue, rfm23_queue_mtx);
              }
              break;
            }
            default:
              break;
          }
        }
      }

      void regulate_temperature() {
        if (heaterinterval > HEATER_CHECK_INTERVAL) {
          heaterinterval     = 0;
          int   reading      = analogRead(A6);
          float voltage      = reading * MV_PER_ADC_UNIT;
          float temperatureF = (voltage - OFFSET_F) / MV_PER_DEGREE_F;
          float temperatureC = (temperatureF - 32) * 5 / 9;
          // Turn heater on or off based on temperature
          if (temperatureC <= heater_threshold) {
            // turn heater on
            pdu.set_heater(PDU::PDU_SW_State::SWITCH_ON);
            print_debug(Helpers::PDU, "Heater turned on");
          } else {
            // turn heater off
            pdu.set_heater(PDU::PDU_SW_State::SWITCH_OFF);
            print_debug(Helpers::PDU, "Heater turned off");
          }
        }
      }

      void update_watchdog_timer() {
        // Update WDT
        // pdu.set_switch(Artemis::Teensy::PDU::PDU_SW::WDT, 1);
        // pdu.set_switch(Artemis::Teensy::PDU::PDU_SW::WDT, 0);
      }
    } // namespace PDU
  }   // namespace Channels
} // namespace Artemis
