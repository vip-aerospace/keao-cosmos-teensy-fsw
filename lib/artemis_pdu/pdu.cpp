/**
 * @file pdu.cpp
 * @brief The PDU class.
 *
 * This file contains definitions for the PDU class.
 */
#include <pdu.h>

namespace Artemis {
namespace Devices {
  /**
   * @brief Construct a new PDU object.
   *
   * @param hw_serial The HardwareSerial connection unique to this instance.
   * @param baud_rate The baud rate of the serial connection to the PDU.
   */
  PDU::PDU(HardwareSerial *hw_serial, int baud_rate) {
    serial = hw_serial;
    serial->begin(baud_rate);
    serial->clear();
    serial->flush();
  }

  /**
   * @brief Send a packet to the PDU.
   *
   * @param packet The pdu_packet to be sent to the PDU.
   * @return true All bytes of the packet were successfully sent to the PDU.
   * @return false There was an error sending a packet to the PDU.
   */
  bool PDU::send(pdu_packet packet) {
    char *ptr = (char *)&packet;

    print_hexdump(Helpers::PDU, "Sending to PDU: ", (uint8_t *)ptr,
                  sizeof(packet));
    for (size_t i = 0; i < sizeof(packet); i++) {
      if (!serial->print((char)(*(ptr + i) + PDU_CMD_OFFSET))) {
        print_debug(Helpers::PDU,
                    "Failed to send character to PDU:", (u_int32_t)i);
        return false;
      }
    }
    if (!serial->write('\0')) {
      print_debug(Helpers::PDU, "Failed to send null character to PDU");
      return false;
    }
    if (!serial->write('\n')) {
      print_debug(Helpers::PDU, "Failed to send newline to PDU");
      return false;
    }

    threads.delay(100);
    return true;
  }

  /**
   * @brief Receive a packet from the PDU.
   *
   * @param packet A pointer to the packet that will carry the received data.
   * @return true A packet has been received successfully.
   * @return false No packet is available from the PDU, or the received packet
   * is invalid.
   */
  bool PDU::recv(pdu_packet *packet) {
    if (serial->available() <= 0) {
      print_debug_rapid(Helpers::PDU, "Nothing in serial buffer to receive");
      return false;
    }
    String UART1_RX = serial->readString();
    if (UART1_RX.length() <= 0) {
      print_debug(Helpers::PDU, "Received serial string empty or corrupted");
      return false;
    }
    packet->type     = (PDU_Type)(UART1_RX[0] - PDU_CMD_OFFSET);
    packet->sw       = (PDU_SW)(UART1_RX[1] - PDU_CMD_OFFSET);
    packet->sw_state = (uint8_t)(UART1_RX[2] - PDU_CMD_OFFSET);

    print_hexdump(Helpers::PDU, "UART received: ", (uint8_t *)&packet,
                  sizeof(packet));
    return true;
  }

  /**
   * @brief Receive a telemetry packet from the PDU.
   *
   * @param packet A pointer to the telemetry packet that will carry the
   * received data.
   * @return true A telemetry packet has been received successfully.
   * @return false No telemetry packet is available from the PDU, or the
   * received packet is invalid.
   */
  bool PDU::recv(pdu_telem *packet) {
    if (serial->available() <= 0) {
      print_debug(Helpers::PDU, "Nothing in serial buffer to receive");
      return false;
    }
    String UART1_RX = serial->readString();
    if (UART1_RX.length() <= 0) {
      print_debug(Helpers::PDU, "Received serial string empty or corrupted");
      return false;
    }
    packet->type = (PDU_Type)(UART1_RX[0] - PDU_CMD_OFFSET);
    for (int i = 0; i < NUMBER_OF_SWITCHES; i++) {
      packet->sw_state[i] = (uint8_t)(UART1_RX[i + 1] - PDU_CMD_OFFSET);
    }

    print_hexdump(Helpers::PDU, "UART received: ", (uint8_t *)packet,
                  sizeof(pdu_telem));
    return true;
  }

  /**
   * @brief Ping the PDU.
   *
   * @return true The PDU replied to the ping request with a pong reply.
   * @return false There was an error in sending the ping request or receiving
   * the pong reply.
   */
  bool PDU::ping() {
    pdu_packet pingPacket;
    pingPacket.type = PDU_Type::CommandPing;

    if (!send(pingPacket)) {
      print_debug(Helpers::PDU, "Failed to send ping packet to PDU");
      return false;
    }
    if (!recv(&pingPacket)) {
      print_debug(Helpers::PDU, "Failed to receive pong reply from PDU");
      return false;
    }
    print_debug(Helpers::PDU, "pingPacket.type=", (u_int32_t)pingPacket.type);
    return pingPacket.type == PDU_Type::DataPong;
  }

  /**
   * @brief Set a switch on the PDU.
   *
   * @param sw The PDU_SW representing the switch on the PDU to be set.
   * @param state The desired switch state.
   * @return true The switch has been successfully set to the desired state.
   * @return false There was an issue setting the switch or receiving the reply,
   * or the switch was not set.
   *
   * @todo This function should check the switch state and ensure it matches
   * what we expect. However, the reply from the PDU with the switch state
   * should still be used to update the internal PDU class switch state, even
   * if the switch hasn't been set.
   */
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
      if (!recv(&packet)) {
        print_debug(Helpers::PDU,
                    "Failed to receive set switch reply from PDU");
        return false;
      }
      switch_states[(uint8_t)packet.sw - 2] = (PDU_SW_State)packet.sw_state;
    } else {
      pdu_telem replyPacket;
      if (!recv(&replyPacket)) {
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

  /**
   * @brief Wrapper function to set the heater switch.
   *
   * @param state The desired state of the heater.
   * @return true The heater switch has been successfully set.
   * @return false The heater switch failed to be set.
   */
  bool PDU::set_heater(PDU_SW_State state) {
    return set_switch(PDU_SW::SW_5V_2, state);
  }

  /**
   * @brief Wrapper function to set the burn wire switch.
   *
   * @param state The desired state of the burn wire.
   * @return true The burn wire switch has been successfully set.
   * @return false The burn wire switch failed to be set.
   */
  bool PDU::set_burn_wire(PDU_SW_State state) {
    return set_switch(PDU_SW::BURN1, state);
  }

  /**
   * @brief Refresh the internal PDU class's switch states.
   *
   * @return true The switch states have been updated successfully and can be
   * trusted.
   * @return false The switch state request failed to be sent or replied to.
   */
  bool PDU::refresh_switch_states() {
    pdu_packet requestPacket;
    pdu_telem  replyPacket;
    requestPacket.type = PDU_Type::CommandGetSwitchStatus;
    requestPacket.sw   = PDU_SW::All;

    if (!send(requestPacket)) {
      print_debug(Helpers::PDU,
                  "Failed to send all switch states request to PDU");
      return false;
    }

    if (!recv(&replyPacket)) {
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