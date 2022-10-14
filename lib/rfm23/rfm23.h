#include <SPI.h>
#include <RH_RF22.h>
#include <RHHardwareSPI1.h>

/* RFM23 FREQUENCY CONFIG */
#define RFM23_FREQ   434.0

/* RFM23 PIN CONFIG */
#define RFM23_CS_PIN  38
#define RFM23_INT_PIN 8

/* SPI MISO/MOSI/SCK CONFIG */
#define RFM23_SPI_MISO 39
#define RFM23_SPI_MOSI 26
#define RFM23_SPI_SCK 27

/* RFM23 Power Level */
#define RFM23_TX_POWER 20
#define RX_ON 30
#define TX_ON 31

namespace Artemis {
    namespace Teensy {
        namespace Radio {
            class RFM23 {
                private:
                    RH_RF22 rfm23;
                    uint8_t RFM23_SEND_BUF[62];
                    uint8_t RFM23_SEND_LEN = 0;
                    uint8_t RFM23_RECV_BUF[RH_RF22_MAX_MESSAGE_LEN];
                    uint8_t RFM23_RECV_LEN = sizeof(RFM23_RECV_BUF);
                public:
                    RFM23(uint8_t slaveSelectPin = RFM23_CS_PIN, uint8_t interruptPin = RFM23_INT_PIN, RHGenericSPI& spi = hardware_spi1);
                    void RFM23_RESET();
                    void RFM23_INIT();
                    void RFM23_SEND(const char *input);
                    void RFM23_RECV();
            };
        }
    }
}


