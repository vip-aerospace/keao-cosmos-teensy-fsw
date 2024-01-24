/**
 * \mainpage Start Here
 *
 * @section Introduction Introduction
 * This is the documentation on the Flight Software (FSW) that runs on the
 * Teensy microcontroller in the Artemis CubeSat's On-Board Computer (OBC)
 * board. This page is designed to give a high-level understanding of the FSW
 * and introduce essential computing concepts that the FSW uses.
 *
 * @section Teensy The Teensy
 * This code runs on the [Teensy 4.1
 * microcontroller](https://www.pjrc.com/store/teensy41.html).
 * As the link shows, this is a very capable microcontroller. If you are
 * familiar with [Arduino](https://www.arduino.cc/en/Guide/Introduction), the
 * Teensy is very similar. You can think of the Teensy as a souped-up Arduino,
 * with more ports and processing power.
 *
 * There are many ways to program a Teensy. This project uses the PlatformIO IDE
 * within Visual Studio Code. This allows us to use a more powerful IDE than
 * Arduino IDE (which is still great for prototyping!). Make sure you follow the
 * instructions in the [Development Environment Setup
 * Guide](#development_environment_setup) in order to program the Teensy.
 *
 * @section HighLevel A High-Level Understanding of the FSW
 * This section will give an overview of the FSW, how it works, and its
 * functional components.
 *
 * The Teensy, as mentioned earlier, is essentially a powerful Arduino. The FSW
 * is built on the Arduino's programming principles. That is, there is a
 * `setup()` function that is called once, followed by a `loop()` function that
 * runs in an infinite loop, forever. These links are to the functions in the
 * main.cpp file, and are the starting point for the code. In essence, this is
 * the top-level Arduino code, the same as any other Arduino sketch.
 *
 * It could be possible to put all our code into this one file, but this would
 * be a massive file that would be hard to read and maintain. In addition, we
 * want to be able to separate the code based on what components of the CubeSat
 * it effects.
 *
 * As a result, we split up the codebase and execution using \link
 * Multithreading multithreading.\endlink Each thread (we call them channels in
 * our implementation) is responsible for some component of the CubeSat, such as
 * the radios, sensors, or PDU. A brief description of each channel is given
 * below.
 *
 * - main_channel: Technically, this channel is not listed in the code. That is
 * because this is the first channel that is started when the Teensy is first
 * powered on. Even though it is not explicitly defined, the Teensy switches to
 * it like any other channel and executes its code. It does the following:
 *  - It acts as a "router" for PacketComm packets. Incoming packets are sent to
 * the \ref main_queue, which then are sent to the appropriate channel for
 * handling.
 *  - It polls the built-in sensors that are not handled by a channel (such as
 * the temperature, current, and IMU sensors) for their readings.
 *  - It generates PacketComm packets for testing other channels.
 *  - It creates every other channel and allocates computing time to them.
 * - \link Artemis::Channels::rfm23_channel() rfm23_channel(): \endlink Controls
 * the HopeRF RFM23 radio. It does the following:
 *  - It configures and connects to the RFM23 radio.
 *  - It handles incoming transmissions containing PacketComm packets, and adds
 * those packets to the \ref main_queue.
 *  - It transmits outgoing PacketComm packets from its \ref rfm23_queue.
 * - \link Artemis::Channels::pdu_channel() pdu_channel(): \endlink Controls the
 * Artemis Power Distribution Unit (PDU). It does the following:
 *  - It connects to the PDU's built-in microcontroller.
 *  - It gets switch states on the PDU.
 *  - It sets switch states on the PDU.
 * - \link Artemis::Channels::rpi_channel() rpi_channel(): \endlink Communicates
 * with the Raspberry Pi (RPi) on Ke Ao's OBC.
 *  - It connects to the RPi.
 *  - It sends packets to the RPi.
 *  - It receives packets from the RPi.
 *  - It can command the RPi to power down.
 *
 * As the descriptions show, each channel runs independently, sharing
 * information in the form of PacketComm packets inserted into each others'
 * queues. At a high level, the Teensy flips through these channels quickly,
 * executing a little bit of each channel before moving on to the next channel.
 * The code must therefore be written with the assumption that it may be
 * interrupted at any time. Further information is given in the multithreading
 * section.
 *
 * @section BuildFlags PlatformIO Build Flags
 * This section describes the PlatformIO build flags and their uses.
 *
 * We can take advantage of the build flags feature of PlatformIO to selectively
 * compile the code for testing and debugging. The build flags are found in the
 * platformio.ini file in the root directory of the code. They are listed on
 * individual lines, like so:
 * @verbatim

build_flags = DEBUG_PRINT

@endverbatim

 * The `;` symbol comments out a flag, disabling it.
 *
 *
 * @subsection Tests Tests
 * These flags are used to run test code, generating test data to see how the
 * program handles incoming data.
 *
 * | Flag Name | Function |
 * |----------|----------|
 * | TESTS | Enable to run tests on the satellite. |
 *
 * @subsection DebugFlags Debug Flags
 * If you've used Arduino before, you might be familiar with the
 * `Serial.print()` style of debugging. We use the same method of debugging
 * in this project for simplicity. When enabled, the Teensy will print debug
 * statements over the Serial connection established over USB to the Terminal
 * in VSCode.
 *
 * Multiple flags are available for use, depending on what statements you'd like
 * to print.
 * | Flag Name | Function |
 * |----------|----------|
 * | DEBUG_PRINT | Enable to print debug messages to the Terminal. |
 * | DEBUG_PRINT_RAPID | Enable to print messages that will are generated |
 * | | rapidly, such as polling requests. |
 * | DEBUG_PRINT_HEXDUMP | Enable to print hexdumps to the serial console. |
 * | DEBUG_MEMORY | Enable to print memory capacity messages to the serial |
 * | | console. |
 *
 */