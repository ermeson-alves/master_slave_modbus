#include <ModbusRTU.h>
#include <HardwareSerial.h>

// --- FIX FOR LED_BUILTIN ---
// Define the LED pin if it's not already, usually GPIO 2 for ESP32
#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif

// --- SETTINGS FROM YOUR CONFIGURATION ---
#define SLAVE_ID 1
#define BAUD_RATE 19200

// Using Serial2 (RX=16, TX=17) for an adapter with automatic direction control
HardwareSerial& modbusSerial = Serial2; 

ModbusRTU slave;

// Define the number of registers for each type
#define COIL_COUNT 10
#define ISTS_COUNT 9  // Discrete Inputs are called "Input Status" (Ists) in the library
#define IREG_COUNT 4
#define HREG_COUNT 2

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("ESP32 Modbus Slave - All Registers Enabled");

  // For ESP32, define the built-in LED pin
  pinMode(LED_BUILTIN, OUTPUT);

  // Start the Modbus serial port
  modbusSerial.begin(BAUD_RATE, SERIAL_8N1);
  Serial.println("1. Modbus serial port started at 19200 baud.");

  // Configure the Modbus slave
  slave.slave(SLAVE_ID);
  Serial.println("2. Slave ID set to 3.");

  if (slave.begin(&modbusSerial)) {
    Serial.println("3. Modbus slave.begin() was successful.");
  } else {
    Serial.println("3. FATAL: Modbus slave.begin() FAILED.");
  }

  // --- CREATE ALL REGISTER TYPES ---
  // Add 10 Coils (%QX100.0 to %QX101.1)
  if (slave.addCoil(0, false, COIL_COUNT)) {
    Serial.println("4. SUCCESS: Coils (%%QX) were added.");
  } else {
    Serial.println("4. FATAL: Adding coils (%%QX) FAILED.");
  }
  // Add 9 Discrete Inputs (%IX100.0 to %IX101.0)
  if (slave.addIsts(0, false, ISTS_COUNT)) {
    Serial.println("4. SUCCESS: Discrete inputs (%%IX) were added.");
  } else {
    Serial.println("4. FATAL: Adding discrete inputs (%%IX) FAILED.");
  }
  // Add 4 Input Registers (%IW108 to %IW111)
  if (slave.addIreg(0, 0, IREG_COUNT)) {
    Serial.println("4. SUCCESS: Input registers (%%IW) were added.");
  } else {
    Serial.println("4. FATAL: Adding input registers (%%IW) FAILED.");
  }
  // Add 2 Holding Registers (%QW104, %QW105)
  if (slave.addHreg(0, 0, HREG_COUNT)) {
    Serial.println("4. SUCCESS: Holding registers (%%QW) were added.");
  } else {
    Serial.println("4. FATAL: Adding holding registers (%%QW) FAILED.");
  }
}

void loop() {
  // Let the slave listen for master requests
  slave.task();

  // --- EXAMPLE LOGIC ---

  // 1. Coils: Make the ESP32's built-in LED follow the state of Coil 0.
  // You can control this by forcing a value to %QX100.0 in OpenPLC.
  bool coil0_status = slave.Coil(0);
  digitalWrite(LED_BUILTIN, coil0_status);

  // 2. Discrete Inputs: Set the status of Discrete Input 0.
  // You can monitor this in OpenPLC at %IX100.0.
  // Here we just make it toggle every 2 seconds.
  bool discrete0_status = (millis() / 2000) % 2;
  slave.Ists(0, discrete0_status);

  // 3. Input Registers: Write a simulated sensor value.
  // Monitor this in OpenPLC at %IW108.
  uint16_t sensor_value = 1000 + 500 * sin(millis() / 1000.0);
  slave.Ireg(0, sensor_value);

  // 4. Holding Registers: You can read the value OpenPLC writes here.
  // OpenPLC writes to this register via %QW104.
  uint16_t value_from_master = slave.Hreg(0);
}