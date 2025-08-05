#include <ModbusRTU.h>
#include <HardwareSerial.h>
#include <FS.h>
#include <SPIFFS.h>
#include <AudioFileSourceSPIFFS.h>
#include <AudioGeneratorWAV.h>
#include <AudioOutputI2S.h>


// --- FIX FOR LED_BUILTIN ---
// Define the LED pin if it's not already, usually GPIO 2 for ESP32
#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif

// --- SETTINGS FROM YOUR CONFIGURATION ---
#define SLAVE_ID 3
#define BAUD_RATE 9600

// Using Serial2 (RX=16, TX=17) for an adapter with automatic direction control
HardwareSerial& modbusSerial = Serial2; 

ModbusRTU slave;

// Define the number of registers for each type
#define COIL_COUNT 10
#define ISTS_COUNT 9  // Discrete Inputs are called "Input Status" (Ists) in the library
#define IREG_COUNT 4
#define HREG_COUNT 2

const int lightSensorPin = 35;
const int soundSensorPin = 34;
const int mutePin = 18;

// ============== Threshold Definitions (CALIBRATE THESE!) ==============
// For LDR: Higher value = brighter light needed to trigger.
// For Sound: Higher value = louder sound needed to trigger.
const int lightThreshold = 1500;
const int soundThreshold = 3000;

// ============== Global Audio Objects ==============
AudioGeneratorWAV *wav;
AudioFileSourceSPIFFS *file;
AudioOutputI2S *out;

// ============== State Machine Flags ==============
bool hasLightAlertPlayed = false;
bool hasSoundAlertPlayed = false;

// Helper function to start playing a WAV file
void playAudioFile(const char *filename) {
  Serial.printf("Attempting to play: %s\n", filename);
  file->open(filename);
  wav = new AudioGeneratorWAV();
  wav->begin(file, out);
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("ESP32 Modbus Slave - All Registers Enabled");

  // --- Mute Pin Control ---
  pinMode(mutePin, OUTPUT);
  digitalWrite(mutePin, LOW); // Unmute the amplifier to make it ready
  Serial.println("Amplifier Unmuted (MUTE on GPIO18 is HIGH).");
  // ------------------------

  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  // This initializes the internal DAC on GPIO25 (and/or GPIO26 if in stereo)
  out = new AudioOutputI2S(0, AudioOutputI2S::INTERNAL_DAC);
  out->SetRate(16000);
  file = new AudioFileSourceSPIFFS();

  // For ESP32, define the built-in LED pin
  pinMode(LED_BUILTIN, OUTPUT);

  // Start the Modbus serial port
  modbusSerial.begin(BAUD_RATE, SERIAL_8E1);
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
  bool LED1Estado = slave.Coil(0);
  digitalWrite(LED_BUILTIN, LED1Estado);

  // 2. Discrete Inputs: Set the status of Discrete Input 0.
  // You can monitor this in OpenPLC at %IX100.0.
  // Here we just make it toggle every 2 seconds.
  bool discrete0_status = (millis() / 2000) % 2;
  slave.Ists(0, discrete0_status);

  // 3. Input Registers: Write a simulated sensor value.
  // Monitor this in OpenPLC at %IW108.
  uint16_t luminosity = analogRead(lightSensorPin);
  slave.Ireg(0, luminosity);
  uint16_t soundIntensity = analogRead(soundIntensity);
  slave.Ireg(1, soundIntensity);

  // 4. Holding Registers: You can read the value OpenPLC writes here.
  // OpenPLC writes to this register via %QW104.
  uint16_t value_from_master = slave.Hreg(0);

  // First, always check if audio is currently playing and service it.
  if (wav != NULL && wav->isRunning()) {
    if (!wav->loop()) {
      wav->stop();
      delete wav;
      wav = NULL;
      Serial.println("Audio Finished.");
    }
  } else {
    // If no audio is playing, we can check for new triggers.
    int lightValue = analogRead(lightSensorPin);
    int soundValue = analogRead(soundSensorPin);

    // Uncomment the line below to help calibrate your thresholds
    Serial.printf("Light: %d, Sound: %d\n", lightValue, soundValue);

    // Check for light trigger (giving it priority)
    if (lightValue < lightThreshold && !hasLightAlertPlayed) {
      hasLightAlertPlayed = true;
      playAudioFile("/light.wav");
    } 
    // If no light trigger, check for sound trigger
    else if (soundValue > soundThreshold && !hasSoundAlertPlayed) {
      hasSoundAlertPlayed = true;
      playAudioFile("/sound.wav");
    }

    // Reset flags when conditions return to normal
    if (lightValue < lightThreshold) {
      hasLightAlertPlayed = false;
    }
    if (soundValue < soundThreshold) {
      hasSoundAlertPlayed = false;
    }
  }
}