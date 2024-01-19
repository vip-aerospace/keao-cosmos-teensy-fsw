/**
 * @file artemis_defs.cpp
 * @brief The Artemis definitions.
 *
 * This file defines global variables and functions used throughout the
 * satellite.
 */
#include "config/artemis_defs.h"

/**
 * @brief The list of active threads.
 *
 * This is a list of thread_structs that correspond to each active thread,
 * running a channel. There can be a maximum of 16 active threads.
 *
 */
vector<struct thread_struct> thread_list;

/** @brief Mapping between string names and NodeType. */
std::map<string, NODES>      NodeType = {
    {        "ground", NODES::GROUND_NODE_ID},
    {"artemis_teensy", NODES::TEENSY_NODE_ID},
    {   "artemis_rpi",    NODES::RPI_NODE_ID},
};

/** @brief The packet queue for the main channel. */
std::deque<PacketComm> main_queue;
/** @brief The packet queue for the RFM23 channel. */
std::deque<PacketComm> rfm23_queue;
/** @brief The packet queue for the PDU channel. */
std::deque<PacketComm> pdu_queue;
/** @brief The packet queue for the Raspberry Pi channel. */
std::deque<PacketComm> rpi_queue;

/** @brief The mutex for the main channel's packet queue. */
Threads::Mutex         main_queue_mtx;
/** @brief The mutex for the RFM23 channel's packet queue. */
Threads::Mutex         rfm23_queue_mtx;
/** @brief The mutex for the PDU channel's packet queue. */
Threads::Mutex         pdu_queue_mtx;
/** @brief The mutex for the Raspberry Pi channel's packet queue. */
Threads::Mutex         rpi_queue_mtx;

/** @brief The mutex for the SPI1 interface. */
Threads::Mutex         spi1_mtx;
/** @brief The mutex for the I2C1 interface. */
Threads::Mutex         i2c1_mtx;

/** @brief Whether the satellite is in deployment mode. */
bool                   deploymentmode = false;

/**
 * @brief Kill a running thread.
 *
 * @todo Check return types here.
 *
 * @param target_channel_id The Channel_ID of the channel to kill.
 * @return true The first channel that matches the target Channel_ID has been
 * killed.
 * @return false The target Channel_ID has not been found in the thread_list.
 */
bool                   kill_thread(uint8_t target_channel_id) {
  for (auto thread_list_iterator = thread_list.begin();
       thread_list_iterator != thread_list.end(); thread_list_iterator++) {
    if (thread_list_iterator->channel_id == target_channel_id) {
      threads.kill(thread_list_iterator->thread_id);
      thread_list.erase(thread_list_iterator);
      return true;
    }
  }
  return false;
}
/**
 * @brief Push a packet into a queue.
 *
 * This is a helper function to push a packet into a queue of packets. It will
 * push out the first packet in the queue if the queue is too large.
 *
 * @param packet The packet object that will be pushed into the queue.
 * @param queue The queue of packets to be pulled from.
 * @param mtx The mutex used to lock the queue.
 */
void PushQueue(PacketComm &packet, std::deque<PacketComm> &queue,
               Threads::Mutex &mtx) {
  Threads::Scope lock(mtx);
  if (queue.size() == MAXQUEUESIZE) {
    queue.pop_front();
  }
  queue.push_back(packet);
}
/**
 * @brief Pull a packet from a queue.
 *
 * This is a helper function to check a queue of packets for a packet.
 *
 * @param packet The packet object that will carry the pulled packet, if there
 * is one.
 * @param queue The queue of packets to be pulled from.
 * @param mtx The mutex used to lock the queue.
 * @return true A packet has been pulled from the queue. The passed-in packet
 * now contains its contents.
 * @return false The queue does not contain any packets.
 */
bool PullQueue(PacketComm &packet, std::deque<PacketComm> &queue,
               Threads::Mutex &mtx) {
  Threads::Scope lock(mtx);
  if (queue.size() > 0) {
    packet = queue.front();
    queue.pop_front();
    return true;
  }
  return false;
}

/** @brief Wrapper function to send a packet to the main channel. */
void route_packet_to_main(PacketComm packet) {
  PushQueue(packet, main_queue, main_queue_mtx);
}
/** @brief Wrapper function to send a packet to the RFM23. */
void route_packet_to_rfm23(PacketComm packet) {
  PushQueue(packet, rfm23_queue, rfm23_queue_mtx);
}
/** @brief Wrapper function to send a packet to the PDU. */
void route_packet_to_pdu(PacketComm packet) {
  PushQueue(packet, pdu_queue, pdu_queue_mtx);
}
/** @brief Wrapper function to send a packet to the Raspberry Pi. */
void route_packet_to_rpi(PacketComm packet) {
  PushQueue(packet, rpi_queue, rpi_queue_mtx);
}
