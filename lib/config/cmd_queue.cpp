#include <cmd_queue.h>

// Mutex for Command Queues
Threads::Mutex main_queue_mtx;
Threads::Mutex rfm23_queue_mtx;
Threads::Mutex pdu_queue_mtx;

// Command Queues
std::queue<Cosmos::Support::PacketComm> main_queue;
std::queue<Cosmos::Support::PacketComm> rfm23_queue;
std::queue<Cosmos::Support::PacketComm> pdu_queue;

// Other Mutex
Threads::Mutex spi_mtx;
Threads::Mutex spi1_mtx;
