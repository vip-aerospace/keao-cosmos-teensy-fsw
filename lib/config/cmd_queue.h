#ifndef _CMD_QUEUE_H
#define _CMD_QUEUE_H

#include <queue>
#include <TeensyThreads.h>
#include "support/packetcomm.h"

// Mutex for Command Queues
extern Threads::Mutex main_queue_mtx;
extern Threads::Mutex rfm23_queue_mtx;
extern Threads::Mutex pdu_queue_mtx;

// Command Queues
extern std::queue<Cosmos::Support::PacketComm> main_queue;
extern std::queue<Cosmos::Support::PacketComm> rfm23_queue;
extern std::queue<Cosmos::Support::PacketComm> pdu_queue;

// Other Mutex
extern Threads::Mutex spi_mtx;
extern Threads::Mutex spi1_mtx;

#endif // _CMD_QUEUE_H
