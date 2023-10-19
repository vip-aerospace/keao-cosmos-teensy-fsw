/**
 * @file artemis_defs.h
 * @brief The Artemis definitions.
 *
 * This file declares constants and functions used throughout the satellite.
 */
#ifndef _ARTEMIS_DEFS_H
#define _ARTEMIS_DEFS_H

#include <TeensyThreads.h>
#include <support/configCosmosKernel.h>
#include <support/packetcomm.h>

/** The number of current sensor readings in the first current beacon. */
#define ARTEMIS_CURRENT_BEACON_1_COUNT 2
/** @brief The number of current sensors in the satellite. */
#define ARTEMIS_CURRENT_SENSOR_COUNT   5

/** @brief The number of temperature sensors in the satellite. */
#define ARTEMIS_TEMP_SENSOR_COUNT      7
/**
 * @brief The conversion factor between temperature and voltage.
 *
 * This value represents the relationship between millivolts and degrees
 * Fahrenheit. It is currently 1 mV/°F.
 */
const float MV_PER_DEGREE_F  = 1.0;
/**
 * @brief The offset of the temperature sensors.
 *
 * This value represents the offset in the output voltage. It is given as either
 * degrees Fahrenheit or millivolts. It is currently 58 mV (58°F).
 */
const float OFFSET_F         = 58.0;
/**
 * @brief The conversion factor between ADC units and voltage.
 *
 * This value represents the ratio between the 10-bit value returned by the
 * Analog-To-Digital (ADC) converter to millivolts DC. It is based off of the
 * maximum value of 1024 representing 3.3VDC, and 0 representing 0.0VDC.
 */
const float MV_PER_ADC_UNIT  = 3300.0 / 1024.0;

/** @brief The activation temperature, in Celcius, of the heater. */
const float heater_threshold = -10.0;

/** @brief The maximum number of packets that a queue can hold. */
#define MAXQUEUESIZE 8

/** @brief Enumeration of Node ID. */
enum class NODES : uint8_t {
  GROUND_NODE_ID = 1,
  TEENSY_NODE_ID = 2,
  RPI_NODE_ID    = 3,
};

/**
 * @brief The structure of a thread.
 *
 * This structure describes identifying information about an active thread. It
 * correlates a running thread's ID with the type of channel it is running.
 */
struct thread_struct {
  int     thread_id;
  uint8_t channel_id;
};

/** @brief Enumeration of Teensy 4.1 pins, based on Artemis OBC v4.23.*/
enum TEENSY_PINS {
  UART4_RXD,
  UART4_TXD,
  T_GPIO2,
  T_GPIO3,
  T_GPIO4,
  T_GPIO5,
  T_GPIO6,
  UART6_RX,
  UART6_TX,
  T_CS1,
  T_CS,
  SPI0_MOSI,
  SPI0_MISO,
  SPI0_SCLK,
  AIN0,
  AIN1,
  SCL1_I2C,
  SDA1_I2C,
  I2C2_SDA,
  I2C2_SCL,
  AIN3,
  AIN4,
  AIN5,
  AIN6,
  I2C1_SCL,
  I2C1_SDA,
  SPI1_D1,
  SPI1_SCLK,
  UART5_TXD,
  UART5_RXD,
  RX_ON,
  TX_ON,
  RADIO_RESET,
  GPS_RSTN,
  UART2_RXD,
  UART2_TXD,
  RPI_ENABLE,
  SDN,
  SPI1_CS1,
  SPI1_D0,
  NIRQ,
  AIN2
};

extern vector<struct thread_struct> thread_list;

extern std::map<string, NODES>      NodeType;

extern std::deque<PacketComm>       main_queue;
extern std::deque<PacketComm>       rfm23_queue;
extern std::deque<PacketComm>       pdu_queue;
extern std::deque<PacketComm>       rpi_queue;

extern Threads::Mutex               main_queue_mtx;
extern Threads::Mutex               rfm23_queue_mtx;
extern Threads::Mutex               pdu_queue_mtx;
extern Threads::Mutex               rpi_queue_mtx;

extern Threads::Mutex               spi1_mtx;
extern Threads::Mutex               i2c1_mtx;

extern bool                         deploymentmode;

bool                                kill_thread(uint8_t channel_id);
void PushQueue(PacketComm &packet, std::deque<PacketComm> &queue,
               Threads::Mutex &mtx);
bool PullQueue(PacketComm &packet, std::deque<PacketComm> &queue,
               Threads::Mutex &mtx);

void route_packet_to_main(PacketComm packet);
void route_packet_to_rfm23(PacketComm packet);
void route_packet_to_pdu(PacketComm packet);
void route_packet_to_rpi(PacketComm packet);

#endif // _ARTEMIS_DEFS_H
