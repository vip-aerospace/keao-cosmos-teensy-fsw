#include "channels/artemis_channels.h"
#include "artemisbeacons.h"
#include <pdu.h>
#include <SD.h>

namespace
{
    Artemis::Teensy::PDU pdu(&Serial1, 115200);
    PacketComm packet;
    Artemis::Teensy::PDU::pdu_packet pdu_packet;
    std::string response;
    unsigned long timeoutStart;
    elapsedMillis uptime;
    elapsedMillis heaterinterval; // Create a new elapsedMillis object for the heater
    int checkinterval = 60000;    // Time in milliseconds. Set this to the desired time between code runs.
}

void Artemis::Teensy::Channels::pdu_channel()
{
    while (!Serial1)
        ;
    threads.delay(5000); // Give the PDU some time to warm up...

    // Ensure PDU is communicating with Teensy
    pdu_packet.type = PDU::PDU_Type::CommandPing;
    while (1)
    {
        pdu.send(pdu_packet);
        pdu.recv(response);
        if (response[0] == (uint8_t)PDU::PDU_Type::DataPong + PDU_CMD_OFFSET)
        {
            Serial.println("PDU connection established");
            break;
        }
        threads.delay(100);
    }

    if (SD.begin(BUILTIN_SDCARD))
    {
        if (!SD.exists("/deployed.txt"))
        {
            deploymentmode = true;
            threads.delay(5000); // Deployment delay (set to desired delay length)

            // Enable burn wire
            Serial.println("Starting Deployment Sequence");
            pdu.set_switch(Artemis::Teensy::PDU::PDU_SW::BURN1, true);
            Serial.println("Burn switch on");
            threads.delay(5000); // burn wire on time
            pdu.set_switch(Artemis::Teensy::PDU::PDU_SW::BURN1, false);
            Serial.println("Burn switch off");

            SD.begin(BUILTIN_SDCARD);
            File file = SD.open("/deployed.txt", FILE_WRITE);
            if (file)
            {
                file.close();
                Serial.println("Deployment recorded on SD card.");
            }
            else
            {
                Serial.println("Error closing file");
                return;
            }

            // Define elapsedMillis variable
            elapsedMillis timeElapsed;
            // unsigned long twoWeeksMillis = 14 * 24 * 60 * 60 * 1000;  // Flight: two weeks in milliseconds
            unsigned long twoWeeksMillis = 60000; // Testing: set desired deployment length in milliseconds

            elapsedMillis heatertimer;

            while (timeElapsed <= twoWeeksMillis)
            {
                handle_pdu_queue();
                if ((heatertimer >= 60000)) // check every minute
                {
                    int reading = analogRead(A6);
                    float voltage = reading * MV_PER_ADC_UNIT;
                    float temperatureF = (voltage - OFFSET_F) / MV_PER_DEGREE_F;
                    float temperatureC = (temperatureF - 32) * 5 / 9;

                    // Turn heater on or off based on temperature
                    if (temperatureC <= heater_threshold)
                    {
                        pdu.set_switch(Artemis::Teensy::PDU::PDU_SW::SW_5V_2, true); // turn heater on
                    }
                    else
                    {
                        pdu.set_switch(Artemis::Teensy::PDU::PDU_SW::SW_5V_2, false); // turn heater off
                    }
                    heatertimer = 0;
                }
                threads.delay(10000);
            }
        }
        else
        {
            Serial.println("Satellite was already deployed"); // issue
        }
    }

    deploymentmode = false;
    Serial.println("Satellite is now in passive state.");
    while (true)
    {
        handle_pdu_queue();
        if (heaterinterval > static_cast<unsigned long>(checkinterval))
        {
            heaterinterval = 0;
            int reading = analogRead(A6);
            float voltage = reading * MV_PER_ADC_UNIT;
            float temperatureF = (voltage - OFFSET_F) / MV_PER_DEGREE_F;
            float temperatureC = (temperatureF - 32) * 5 / 9;
            // Turn heater on or off based on temperature
            if (temperatureC <= heater_threshold)
            {
                pdu.set_switch(Artemis::Teensy::PDU::PDU_SW::SW_5V_2, true); // turn heater on
                Serial.println("Heater turned on");
            }
            else
            {
                pdu.set_switch(Artemis::Teensy::PDU::PDU_SW::SW_5V_2, false); // turn heater off
                Serial.println("Heater turned off");
            }
        }

        // Update WDT
        // pdu.set_switch(Artemis::Teensy::PDU::PDU_SW::WDT, 1);
        // pdu.set_switch(Artemis::Teensy::PDU::PDU_SW::WDT, 0);

        threads.delay(100);
    }
}

void Artemis::Teensy::Channels::handle_pdu_queue()
{
    if (PullQueue(packet, pdu_queue, pdu_queue_mtx))
    {
        switch (packet.header.type)
        {
        case PacketComm::TypeId::CommandEpsCommunicate:
        {
            pdu_packet.type = PDU::PDU_Type::CommandPing;

            timeoutStart = millis();
            while (1)
            {
                pdu.send(pdu_packet);
                pdu.recv(response);
                if (response[0] == (uint8_t)PDU::PDU_Type::DataPong + PDU_CMD_OFFSET)
                {
                    pdu_packet.type = PDU::PDU_Type::DataPong;
                    packet.header.nodedest = packet.header.nodeorig;
                    packet.header.nodeorig = (uint8_t)NODES::TEENSY_NODE_ID;
                    PushQueue(packet, main_queue, main_queue_mtx);
                    break;
                }

                if (millis() - timeoutStart > 5000)
                {
                    Serial.println("Unable to Ping PDU");
                    break;
                }

                threads.delay(100);
            }
            break;
        }
        case PacketComm::TypeId::CommandEpsSwitchName:
        {
            Artemis::Teensy::PDU::PDU_SW switchid = (Artemis::Teensy::PDU::PDU_SW)packet.data[0];
            pdu.set_switch(switchid, packet.data[1]);
            break;
        }
        case PacketComm::TypeId::CommandEpsSwitchStatus:
        {
            string response;
            pdu.get_switch(Artemis::Teensy::PDU::PDU_SW::All, response);
            switchbeacon beacon;
            beacon.deci = uptime;
            for (size_t i = 1; i < response.length() - 2; i++)
            {
                beacon.sw[i - 1] = response[i] - PDU_CMD_OFFSET;
            }
            beacon.sw[12] = digitalRead(UART6_TX);

            packet.header.type = PacketComm::TypeId::DataObcBeacon;
            packet.header.nodeorig = (uint8_t)NODES::TEENSY_NODE_ID;
            packet.header.nodedest = (uint8_t)NODES::GROUND_NODE_ID;
            packet.data.resize(sizeof(beacon));
            memcpy(packet.data.data(), &beacon, sizeof(beacon));
            packet.header.chanin = 0;
            packet.header.chanout = Artemis::Teensy::Channels::Channel_ID::RFM23_CHANNEL;
            PushQueue(packet, rfm23_queue, rfm23_queue_mtx);
            break;
        }
        default:
            break;
        }
    }
}