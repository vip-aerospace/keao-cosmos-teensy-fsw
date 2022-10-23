#include <Arduino.h>
#include <vector>
#include <artemis_channels.h>
// #include <cosmos-defs.h>
// #include <configCosmos.h>
// #include <packetcomm.h>
// #include <convertlib.h>
// #include <cosmos-errno.h>
// #include <datalib.h>
// #include <elapsedtime.h>
// #include <enumlib.h>
// #include <ephemlib.h>
// #include <geomag.h>
// #include <jsonobject.h>
// #include <jsonvalue.h>
// #include <sliplib.h>
// #include <stringlib.h>
// #include <timelib.h>
// #include <timeutils.h>

struct thread_struct
{
  int thread_id;
  const char *thread_name;
};

std::vector<struct thread_struct> thread_list;

void setup()
{
  Serial.begin(115200);

  // Threads
  // thread_list.push_back({threads.addThread(Artemis::Teensy::Channels::rfm23_channel), "rfm23 thread"});
  thread_list.push_back({threads.addThread(Artemis::Teensy::Channels::accelerometer_gyroscope_channel), "accelerometer gyroscope thread"});
  thread_list.push_back({threads.addThread(Artemis::Teensy::Channels::magnetometer_channel), "magnetomter thread"});
  thread_list.push_back({threads.addThread(Artemis::Teensy::Channels::temperature_channel), "temperature thread"});
  thread_list.push_back({threads.addThread(Artemis::Teensy::Channels::current_channel), "current thread"});
  thread_list.push_back({threads.addThread(Artemis::Teensy::Channels::pdu_channel), "pdu thread"});

// remove telem threads and create function for them 
}

int getbatterylevel(){ // replace this with actual hardware read
  return 5;
}
void loop()
{
  //put telem threads into here 

  int batterylevel = getbatterylevel();
  // create new packet this will be done by packetcom
  // put battery level in packet 
  // send to radio queue 
  // repeat with any other telem hardware data 
  // ... 
  threads.sleep(10000);
}


