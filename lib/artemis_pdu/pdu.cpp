#include <pdu.h>

namespace Artemis {
  namespace Devices {
    PDU::PDU(HardwareSerial *hw_serial, int baud_rate) {
      serial = hw_serial;
      serial->begin(baud_rate);
      serial->clear();
      serial->flush();
    }

    int32_t PDU::send(pdu_packet packet) {
      char *ptr = (char *)&packet;
      for (size_t i = 0; i < sizeof(packet); i++) {
        serial->print((char)(*(ptr + i) + PDU_CMD_OFFSET));
      }
      serial->write('\0');
      serial->print('\n');

      Helpers::print_hexdump(Helpers::PDU, "Sending to PDU: ", (uint8_t *)ptr,
                             sizeof(packet));
      threads.delay(100);
      return 0;
    }

    int32_t PDU::recv(std::string &response) {
      if (serial->available() > 0) {
        String UART1_RX = serial->readString();
        if (UART1_RX.length() > 0) {
          for (unsigned int i = 0; i < UART1_RX.length(); i++) {
            if (UART1_RX[i] != 0x20 && UART1_RX[i] != 0x0D &&
                UART1_RX[i] != 0x0A) {
              UART1_RX[i] = UART1_RX[i] - PDU_CMD_OFFSET;
            }
          }
          Helpers::print_hexdump(Helpers::PDU,
                                 "UART received: ", (uint8_t *)UART1_RX.c_str(),
                                 UART1_RX.length());
          response = UART1_RX.c_str();
          return UART1_RX.length();
        }
      }
      return -1;
    }

    int32_t PDU::set_switch(PDU_SW sw, uint8_t _enable) {
      std::string response;
      pdu_packet  packet;
      packet.type            = PDU_Type::CommandSetSwitch;
      packet.sw              = sw;
      packet.sw_state        = _enable > 0 ? 1 : 0;

      uint8_t       attempts = 1;
      elapsedMillis timeout;
      while (1) {
        send(packet);
        while (recv(response) < 0) {
          if (timeout > 5000) {
            Helpers::print_debug(
                Helpers::PDU,
                "Failed to send CMD to PDU. Attempt: ", (u_int32_t)attempts);
            timeout = 0;

            if (++attempts == 5) {
              return -1;
            }
            break;
          }
        }
        if ((response[1] == (uint8_t)sw) ||
            (sw == PDU_SW::All &&
             response[0] == (uint8_t)PDU::PDU_Type::DataSwitchTelem)) {
          break;
        }

        threads.delay(100);
      }
      Helpers::print_hexdump(Helpers::PDU,
                             "UART received: ", (uint8_t *)response.c_str(),
                             response.length());
      return 0;
    }

    int32_t PDU::get_switch(PDU_SW sw, string &ret) {
      std::string response;
      pdu_packet  packet;
      packet.type            = PDU_Type::CommandGetSwitchStatus;
      packet.sw              = sw;

      uint8_t       attempts = 1;
      elapsedMillis timeout;
      while (1) {
        send(packet);
        while (recv(response) < 0) {
          if (timeout > 5000) {
            Helpers::print_debug(
                Helpers::PDU,
                "Failed to send CMD to PDU. Attempt: ", (u_int32_t)attempts);
            timeout = 0;

            if (++attempts == 5) {
              return -1;
            }
            break;
          }
        }
        if ((response[1] == (uint8_t)sw) ||
            (sw == PDU_SW::All &&
             response[0] == (uint8_t)PDU::PDU_Type::DataSwitchTelem)) {
          break;
        }
        threads.delay(100);
      }
      Helpers::print_hexdump(Helpers::PDU,
                             "UART received: ", (uint8_t *)response.c_str(),
                             response.length());
      ret = response;

      if (sw == PDU_SW::All) {
        for (size_t i = 2; i < response.length(); i++) {
          if (response[i] == '0') {
            return 0;
          }
        }
        return 1;
      }

      return response[3] - PDU_CMD_OFFSET;
    }
  } // namespace Devices
} // namespace Artemis