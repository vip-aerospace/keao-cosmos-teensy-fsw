#include <pdu.h>

namespace Artemis {
  namespace Devices {
    PDU::PDU(HardwareSerial *hw_serial, int baud_rate) {
      serial = hw_serial;
      serial->begin(baud_rate);
      serial->clear();
      serial->flush();
    }

    bool PDU::send(pdu_packet packet) {
      char *ptr = (char *)&packet;
      for (size_t i = 0; i < sizeof(packet); i++) {
        serial->print((char)(*(ptr + i) + PDU_CMD_OFFSET));
      }
      serial->write('\0');
      serial->print('\n');

      print_hexdump(Helpers::PDU, "Sending to PDU: ", (uint8_t *)ptr,
                    sizeof(packet));
      threads.delay(100);
      return true;
    }

    bool PDU::recv(pdu_packet packet) {
      if (serial->available() <= 0) {
        print_debug(Helpers::PDU, "Nothing in serial buffer to receive");
        return false;
      }
      String UART1_RX = serial->readString();
      if (UART1_RX.length() <= 0) {
        print_debug(Helpers::PDU, "Received serial string empty or corrupted");
        return false;
      }
      packet.type     = (PDU_Type)(UART1_RX[0] - PDU_CMD_OFFSET);
      packet.sw       = (PDU_SW)(UART1_RX[1] - PDU_CMD_OFFSET);
      packet.sw_state = (uint8_t)(UART1_RX[2] - PDU_CMD_OFFSET);

      print_hexdump(Helpers::PDU, "UART received: ", (uint8_t *)&packet,
                    sizeof(packet));
      return true;
    }

    bool PDU::recv(pdu_telem packet) {
      if (serial->available() <= 0) {
        print_debug(Helpers::PDU, "Nothing in serial buffer to receive");
        return false;
      }
      String UART1_RX = serial->readString();
      if (UART1_RX.length() <= 0) {
        print_debug(Helpers::PDU, "Received serial string empty or corrupted");
        return false;
      }
      packet.type = (PDU_Type)(UART1_RX[0] - PDU_CMD_OFFSET);
      for (int i = 0; i < NUMBER_OF_SWITCHES; i++) {
        packet.sw_state[i] = (uint8_t)(UART1_RX[i + 1] - PDU_CMD_OFFSET);
      }

      print_hexdump(Helpers::PDU, "UART received: ", (uint8_t *)&packet,
                    sizeof(packet));
      return true;
    }

    bool PDU::ping() {
      pdu_packet pingPacket;
      pingPacket.type = PDU_Type::CommandPing;

      if (!send(pingPacket)) {
        print_debug(Helpers::PDU, "Failed to send ping packet to PDU");
        return false;
      }
      if (!recv(pingPacket)) {
        print_debug(Helpers::PDU, "Failed to receive pong reply from PDU");
        return false;
      }
      return pingPacket.type == PDU_Type::DataPong;
    }

    bool PDU::set_switch(PDU_SW sw, PDU_SW_State state) {
      if (switch_states[(uint8_t)sw - 2] == state) {
        print_debug(Helpers::PDU, "Switch already set to desired state");
        return true;
      }
      pdu_packet packet;
      packet.type     = PDU_Type::CommandSetSwitch;
      packet.sw       = sw;
      packet.sw_state = (uint8_t)state;

      if (!send(packet)) {
        print_debug(Helpers::PDU, "Failed to send set switch command to PDU");
        return false;
      }
      if (sw != PDU_SW::All) {
        if (!recv(packet)) {
          print_debug(Helpers::PDU,
                      "Failed to receive set switch reply from PDU");
          return false;
        }
        switch_states[(uint8_t)packet.sw - 2] = (PDU_SW_State)packet.sw_state;
      } else {
        pdu_telem replyPacket;
        if (!recv(replyPacket)) {
          print_debug(Helpers::PDU,
                      "Failed to receive set all switches reply from PDU");
          return false;
        }
        for (int i = 0; i < NUMBER_OF_SWITCHES; i++) {
          switch_states[i] = (PDU_SW_State)replyPacket.sw_state[i];
        }
      }
      return true;
    }

    bool PDU::set_heater(PDU_SW_State state) {
      return set_switch(PDU_SW::SW_5V_2, state);
    }

    bool PDU::set_burn_wire(PDU_SW_State state) {
      return set_switch(PDU_SW::BURN1, state);
    }

    bool PDU::get_all_switch_states() {
      pdu_packet requestPacket;
      pdu_telem  replyPacket;
      requestPacket.type = PDU_Type::CommandGetSwitchStatus;
      requestPacket.sw   = PDU_SW::All;

      if (!send(requestPacket)) {
        print_debug(Helpers::PDU,
                    "Failed to send all switch states request to PDU");
        return false;
      }

      if (!recv(replyPacket)) {
        print_debug(Helpers::PDU,
                    "Failed to receive all switch states reply from PDU");
        return false;
      }

      for (int i = 0; i < NUMBER_OF_SWITCHES; i++) {
        switch_states[i] = (PDU_SW_State)replyPacket.sw_state[i];
      }
      return true;
    }
  } // namespace Devices
} // namespace Artemis