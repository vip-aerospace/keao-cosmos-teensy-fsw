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
 * @section Multithreading Understanding Multithreading
 *
 * Although the Teensy is a powerful microcontroller, it does have limitations.
 * In particular, it only has a single CPU to execute instructions. Despite
 * this, we want the Teensy (and Artemis) to do multiple things at the same time
 * (known as multitasking).
 *
 * To accomplish this, we use a technique known as multithreading. In
 * multithreading, a program (such as the FSW) creates multiple smaller programs
 * (called "threads") that are switched between in order to execute different
 * parts of a program nearly simultaneously.
 *
 * The specific implementation that the FSW uses is based on the [TeensyThreads
 * library](https://github.com/ftrias/TeensyThreads). In the FSW, we call
 * threads "channels", and we switch between these channels as the program
 * proceeds. The main channel is the channel that is started when the Teensy is
 * powered on. It creates a channel for each functional part of Artemis, and
 * switches between these channels (and itself) based on the computing time
 * assigned to each channel.
 *
 * @subsection Mutexes Mutual Exclusions (Mutexes)
 *
 * Each channel is defined in its own file, and essentially behaves as its own
 * independent Arduino sketch. In theory, this is all that is needed to
 * implement multitasking. However, it is not this simple. Just because the
 * channel is the currently active one, does not mean that it has exclusive
 * access to all of the Teensy's hardware.
 *
 * Consider this example. Channel A uses the Teensy's I2C connection to
 * communicate with a serial device (say, a radio). I2C is a bus-based system,
 * where all devices are connected to the same data bus, and it is up to the
 * Teensy to specify which device on the bus it wishes to communicate with. Now
 * Channel A initiates a connection to the radio, indicating that it will send
 * data to the radio to transmit. However, Channel A's computing time runs out
 * before it can actually send the data that it wants to transmit.
 *
 * Channel B is up next, and it polls a temperature sensor over the same I2C
 * connection. The Teensy keeps the I2C connection active and sends a request
 * addressed to the sensor. However, the radio interprets the data in its
 * entirety to be the data to transmit, and incorrectly transmits the request.
 * In reality, there are other safeguards against this, but this illustrates
 * the problem.
 *
 * This type of error is due to collisions in shared resources. These resources
 * could be pins on the Teensy, serial connections, shared memory, etc. To avoid
 * these collisions, we use mutual exclusions (mutexes).
 *
 * A mutex is essentially a token that ensures that a given channel (thread) is
 * the only one that has access to a resource. It is "locked" when a channel
 * claims access to a resource, and is "unlocked" when the channel is done using
 * the resource. Importantly, if one channel has a mutex locked, it will retain
 * exclusive access to that resource even if that channel is not being actively
 * executed. Thus, if another channel wants to lock that mutex, it will either
 * fail outright or wait until the mutex is no longer locked (depending on the
 * specific implementation).
 *
 * @subsection MultithreadingAnalogy An Analogy
 *
 * To illustrate this, consider this analogy. You have multiple homework
 * assignments for different classes (say Calculus, Circuits, Physics and
 * English). You procrastinated, so they're all due at the same time and there's
 * no time to complete all of them. You could do each individually and in
 * sequence. That is, you complete all Calculus problems, then Circuits, etc.
 * This would allow you to complete all of the Calculus, Circuits, and Physics
 * problems, but you wouldn't have any time to even start the English paper.
 *
 * You decide to implement a multitasking approach based on multithreading. You
 * set an alarm to go off every 20 minutes, and work on one assignment at a time
 * in that interval. If the alarm goes off, you switch to the next assignment,
 * even if you are in the middle of a problem.
 *
 * This approach works well and you get a little bit of each assignment done.
 * However, you're working on a Calculus problem, using your calculator, when
 * the alarm goes off! The next assignment is a Circuits program that also
 * requires a calculator. You can't reset or clear the calculator (it has the
 * values you were working on for Calculus), so you skip to the next Circuits
 * problem, noting that you should return to the previous problem when you have
 * access to the calculator again.
 *
 * You skip forward to Physics. It's a short assignment, so you complete it
 * quickly. You can take the rest of the time you allocated to Physics and idle
 * (do nothing). You then do some of the English paper, then return to Calculus.
 * You complete the problem and release the calculator. You then switch to
 * Physics, now able to use the calculator now that it is free for your use.
 *
 * This gives a simplified view of what the Teensy's processor is doing when
 * using multithreading. As you can see, computers are much better at
 * multitasking than humans. Even so, just as a human requires time to "get up
 * to speed" on each transition from one task to another, a computer has a
 * performance penalty for switching between threads. This performance hit is
 * negligible for our purposes, but it does exist. For those interested in
 * computer architecture, the processor must flush and refill its cache when
 * switching between threads.
 */