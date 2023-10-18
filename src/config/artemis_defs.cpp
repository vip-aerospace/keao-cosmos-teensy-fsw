#include "config/artemis_defs.h"

vector<struct thread_struct> thread_list;

std::map<string, NODES>      NodeType = {
    {        "ground", NODES::GROUND_NODE_ID},
    {"artemis_teensy", NODES::TEENSY_NODE_ID},
    {   "artemis_rpi",    NODES::RPI_NODE_ID},
};

// Mutex for Command Queues
Threads::Mutex         main_queue_mtx;
Threads::Mutex         rfm23_queue_mtx;
Threads::Mutex         pdu_queue_mtx;
Threads::Mutex         rpi_queue_mtx;

// Command Queues
std::deque<PacketComm> main_queue;
std::deque<PacketComm> rfm23_queue;
std::deque<PacketComm> pdu_queue;
std::deque<PacketComm> rpi_queue;

// Other Mutex
Threads::Mutex         spi1_mtx;
Threads::Mutex         i2c1_mtx;

bool                   deploymentmode = false;

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

void PushQueue(PacketComm &packet, std::deque<PacketComm> &queue,
               Threads::Mutex &mtx) {
  Threads::Scope lock(mtx);
  if (queue.size() == MAXQUEUESIZE) {
    queue.pop_front();
  }
  queue.push_back(packet);
}

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

void route_packet_to_pdu(PacketComm packet) {
  PushQueue(packet, pdu_queue, pdu_queue_mtx);
}

void route_packet_to_rpi(PacketComm packet) {
  PushQueue(packet, rpi_queue, rpi_queue_mtx);
}

void route_packet_to_rfm23(PacketComm packet) {
  PushQueue(packet, rfm23_queue, rfm23_queue_mtx);
}
