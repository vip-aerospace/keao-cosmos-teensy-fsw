#include "tests/tests.h"
#include <pdu.h>

using namespace Artemis;

namespace {
  elapsedMillis piShutdownTimer = 0;
  bool          isoff           = false;
  uint32_t      packet_count    = 0;
} // namespace

void run_test() {
#ifdef TESTS
  Cosmos::Support::PacketComm packet;

#ifdef ENABLE_RASPBERRYPI
  packet.header.type     = PacketComm::TypeId::CommandEpsSwitchName;
  packet.header.nodeorig = (uint8_t)NODES::GROUND_NODE_ID;
  packet.header.nodedest = (uint8_t)NODES::TEENSY_NODE_ID;
  packet.data.resize(0);
  packet.data.push_back((uint8_t)Artemis::Teensy::PDU::PDU_SW::RPI);
  packet.data.push_back(1);
  PushQueue(packet, main_queue, main_queue_mtx);
#endif

#ifdef ENABLE_RASPBERRYPI
  packet.header.type     = (PacketComm::TypeId)0x800;
  packet.header.nodeorig = (uint8_t)NODES::TEENSY_NODE_ID;
  packet.header.nodedest = (uint8_t)NODES::RPI_NODE_ID;
  packet.data.resize(0);
  PushQueue(packet, rpi_queue, rpi_queue_mtx);
#endif

#ifdef ENABLE_PDU
  packet.header.type     = PacketComm::TypeId::CommandEpsSwitchName;
  packet.header.nodeorig = (uint8_t)NODES::GROUND_NODE_ID;
  packet.header.nodedest = (uint8_t)NODES::TEENSY_NODE_ID;
  packet.data.resize(0);
  packet.data.push_back((uint8_t)Artemis::Teensy::PDU::PDU_SW::All);
  packet.data.push_back(1);
  PushQueue(packet, main_queue, main_queue_mtx);
#endif

#ifdef ENABLE_RASPBERRYPI
  packet.header.type     = PacketComm::TypeId::CommandCameraCapture;
  packet.header.nodeorig = (uint8_t)NODES::GROUND_NODE_ID;
  packet.header.nodedest = (uint8_t)NODES::RPI_NODE_ID;
  packet.data.clear();
  PushQueue(packet, main_queue, main_queue_mtx);
#endif

#ifdef ENABLE_RASPBERRYPI
  if (piShutdownTimer > 10000 && !isoff) {
    packet.header.type     = PacketComm::TypeId::CommandEpsSwitchName;
    packet.header.nodeorig = (uint8_t)NODES::GROUND_NODE_ID;
    packet.header.nodedest = (uint8_t)NODES::RPI_NODE_ID;
    packet.data.clear();
    packet.data.push_back((uint8_t)Artemis::Teensy::PDU::PDU_SW::RPI);
    packet.data.push_back(0);
    PushQueue(packet, main_queue, main_queue_mtx);
  }
#endif

#ifdef ENABLE_RFM23
  packet.header.type     = PacketComm::TypeId::DataObcResponse;
  packet.header.nodeorig = (uint8_t)NODES::TEENSY_NODE_ID;
  packet.header.nodedest = (uint8_t)NODES::GROUND_NODE_ID;
  packet.header.chanin   = 0;
  packet.header.chanout  = Artemis::Channels::Channel_ID::RFM23_CHANNEL;

  packet.data.resize(0);
  String data_str = String(packet_count);
  for (size_t i = 0; i < data_str.length(); i++) {
    Serial.print((unsigned)(data_str[i] - '0'));
    packet.data.push_back(data_str[i] - '0');
  }
  Serial.println();
  packet_count++;

  PushQueue(packet, main_queue, main_queue_mtx);
#endif

  for (auto &t : thread_list) {
    Serial.print("Thread ");
    Serial.print(t.thread_id);
    Serial.print(": ");
    Serial.println(threads.getState(t.thread_id));
  }

#ifdef ENABLE_TEMPERATURESENSORS
  devices.read_temperature(uptime);
#endif

#ifdef ENABLE_CURRENTSENSORS
  devices.read_current(uptime);
#endif

#ifdef ENABLE_IMU
  devices.read_imu(uptime);
#endif

#ifdef ENABLE_MAGNETOMETER
  devices.read_mag(uptime);
#endif

#ifdef ENABLE_GPS
  devices.read_gps(uptime);
#endif

// PDU Switch Status
#ifdef ENABLE_PDU
  packet.header.type     = PacketComm::TypeId::CommandEpsSwitchStatus;
  packet.header.nodeorig = (uint8_t)NODES::GROUND_NODE_ID;
  packet.header.nodedest = (uint8_t)NODES::TEENSY_NODE_ID;
  packet.data.clear();
  packet.data.push_back((uint8_t)Artemis::Teensy::PDU::PDU_SW::All);
  PushQueue(packet, pdu_queue, pdu_queue_mtx);
#endif

#endif
}