#include <Arduino.h>
#include <vector>
#include <artemis_channels.h>

Adafruit_LIS3MDL magnetometer;
Adafruit_LSM6DSOX imu;
Adafruit_INA219 current_1(0x40); // Solar 1
Adafruit_INA219 current_2(0x41); // Solar 2
Adafruit_INA219 current_3(0x42); // Solar 3
Adafruit_INA219 current_4(0x43); // Solar 4
Adafruit_INA219 current_5(0x44); // Battery

#define current_count 5 // The total number of current sensors
const char *current_sen_names[current_count] = {"solar_panel_1", "solar_panel_2", "solar_panel_3", "solar_panel_4", "battery_board"};
float busvoltage[current_count] = {0}, current[current_count] = {0}, power[current_count] = {0};
Adafruit_INA219 *p[current_count] = {&current_1, &current_2, &current_3, &current_4, &current_5};

#define temp_count 7 // The total number of temperature sensors
#define aref_voltage 3.3
const int temps[temp_count] = {A0, A1, A6, A7, A8, A9, A17};
const char *temp_sen_names[temp_count] = {"solar_panel_1", "solar_panel_2", "solar_panel_3", "solar_panel_4", "battery_board"};
float voltage[temp_count] = {0}, temperatureC[temp_count] = {0};

float magx = {0}, magy = {0}, magz = {0};
float accelx = {0}, accely = {0}, accelz = {0};
float gyrox = {0}, gyroy = {0}, gyroz = {0};
float imutemp = {0};

struct thread_struct
{
  int thread_id;
  const char *thread_name;
};

std::vector<struct thread_struct> thread_list;

bool setupmagnetometer(void)
{
  if (!magnetometer.begin_I2C())
  {
    return false;
  }

  magnetometer.setPerformanceMode(LIS3MDL_LOWPOWERMODE);
  magnetometer.setDataRate(LIS3MDL_DATARATE_0_625_HZ);
  magnetometer.setRange(LIS3MDL_RANGE_16_GAUSS);
  magnetometer.setOperationMode(LIS3MDL_CONTINUOUSMODE);

  return true;
}

bool setupimu(void)
{
  if (!imu.begin_I2C())
  {
    return false;
  }
  imu.setAccelRange(LSM6DS_ACCEL_RANGE_16_G);
  imu.setGyroRange(LSM6DS_GYRO_RANGE_2000_DPS);
  imu.setAccelDataRate(LSM6DS_RATE_6_66K_HZ);
  imu.setGyroDataRate(LSM6DS_RATE_6_66K_HZ);

  return true;
}

void setupcurrent() // go through library and see what we need to configure and callibrate
{
  current_1.begin(&Wire2);
  current_2.begin(&Wire2);
  current_3.begin(&Wire2);
  current_4.begin(&Wire2);
  current_5.begin(&Wire2);

  return;
}

void setuptemperature()
{
  for (const int pin : temps)
  {
    pinMode(pin, INPUT);
  }

  return;
}

void setup()
{
  Serial.begin(115200);

  setupmagnetometer();
  setupimu();
  setupcurrent();

  // Threads
  thread_list.push_back({threads.addThread(Artemis::Teensy::Channels::rfm23_channel), "rfm23 thread"});
  thread_list.push_back({threads.addThread(Artemis::Teensy::Channels::pdu_channel), "pdu thread"});
}
void readtempertaure() // future make this its own library
{
  for (int i = 0; i < temp_count; i++)
  {
    const int reading = analogRead(temps[i]);
    voltage[i] = reading * aref_voltage;
    voltage[i] /= 1024.0;
    const float temperatureF = (voltage[i] * 1000) - 58;
    temperatureC[i] = (temperatureF - 32) / 1.8;
  }
}

void readcurrent()
{
  for (int i = 0; i < current_count; i++)
  {
    busvoltage[i] = (p[i]->getBusVoltage_V());
    current[i] = (p[i]->getCurrent_mA());
    power[i] = (p[i]->getPower_mW());
  }
}

void readimu()
{
  sensors_event_t event;
  sensors_event_t accel;
  sensors_event_t gyro;
  sensors_event_t temp;
  imu.getEvent(&accel, &gyro, &temp);
  magnetometer.getEvent(&event);
  magx = (event.magnetic.x);
  magy = (event.magnetic.y);
  magz = (event.magnetic.z);
  accelx = (accel.acceleration.x);
  accely = (accel.acceleration.y);
  accelz = (accel.acceleration.z);
  gyrox = (gyro.gyro.x);
  gyroy = (gyro.gyro.y);
  gyroz = (gyro.gyro.z);
  imutemp = (temp.temperature);
}
void loop()
{
  readtempertaure();
  readcurrent();
  readimu();

  // create new packet this will be done by packetcom
  // put battery level in packet
  // send to radio queue
  // repeat with any other telem hardware data
  // ...
  threads.sleep(10000);
}
