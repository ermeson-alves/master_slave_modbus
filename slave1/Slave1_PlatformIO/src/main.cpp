#include <ModbusRTU.h>
#include <HardwareSerial.h>
#include <Wire.h>
#include <math.h>

// === CONFIGURAÇÕES DO HARDWARE ===
#define PIR 19
#define LED 4
#define LED_BUILTIN 2
#define PIN_SENSOR_NTC 34

// === CONFIGURAÇÕES DO MODBUS ===
#define SLAVE_ID 1
#define BAUD_RATE 9600
HardwareSerial& modbusSerial = Serial2;
ModbusRTU slave;

// === REGISTRADORES MODBUS ===
#define COIL_PIR 0
#define COIL_LED 1
#define IREG_SENSOR 0

#define COIL_COUNT 10
#define ISTS_COUNT 9
#define IREG_COUNT 4
#define HREG_COUNT 2

// === SENSOR NTC CONFIG ===
const float seriesResistor = 5000.0;
const float nominalResistance = 5000.0;
const float nominalTemperature = 25.0;
const float bCoefficient = 3950.0;
const int adcMax = 4095;
const float vRef = 3.3;

// === VARIÁVEIS ===
unsigned long lastUpdate = 0;
bool state_pir = false;
bool estadoLED = false;

float lerTemperatura();
void gerenciarPIR(bool ligado);


void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("ESP32 Modbus Slave - com NTC e PIR");

  pinMode(PIR, INPUT);
  pinMode(LED, OUTPUT);
  pinMode(PIN_SENSOR_NTC, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  // Inicializa Modbus
  modbusSerial.begin(BAUD_RATE, SERIAL_8E1);
  slave.begin(&modbusSerial);
  slave.slave(SLAVE_ID);

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
  slave.task(); // Necessário para manter o Modbus funcionando

  if (millis() - lastUpdate > 500) {
    lastUpdate = millis();

    float temperatura = lerTemperatura();
    uint16_t temperaturaX10 = (uint16_t)(temperatura * 10); // Ex: 25.3°C → 253

    slave.Ireg(IREG_SENSOR, temperaturaX10);
    Serial.print("Temperatura enviada: ");
    Serial.print(temperatura);
    Serial.println(" °C");
  }

  // Lê comando do mestre (HIGH/LOW) para ativar PIR
  bool comandoMestre = slave.Coil(COIL_PIR);
  if (comandoMestre != state_pir) {
    state_pir = comandoMestre;
  }

  // gerenciarPIR(state_pir);
  estadoLED = slave.Coil(COIL_LED);
  digitalWrite(LED, estadoLED);
}

// === LÊ E CALCULA A TEMPERATURA COM NTC ===
float lerTemperatura() {
  int adcValue = analogRead(PIN_SENSOR_NTC);
  float voltage = adcValue * vRef / adcMax;

  float resistance = (vRef - voltage) * seriesResistor / voltage;
  float steinhart;
  steinhart = resistance / nominalResistance;
  steinhart = log(steinhart);
  steinhart /= bCoefficient;
  steinhart += 1.0 / (nominalTemperature + 273.15);
  steinhart = 1.0 / steinhart;
  steinhart -= 273.15;

  return steinhart;
}

// === LÓGICA DE DETECÇÃO COM SENSOR PIR ===
void gerenciarPIR(bool ligado) {
  if (ligado) {
    digitalWrite(LED, HIGH);

    // int movimento = digitalRead(PIR);
    // if (movimento == HIGH) {
    //   estadoLED = !estadoLED; // Alterna estado do LED
    //   Serial.println("Movimento detectado: alternando LED.");
    //   delay(200); // Evita múltiplas leituras em sequência
    // }
  } else {
    // estadoLED = false; // Força LED a desligar
    digitalWrite(LED, LOW);

  }

  // digitalWrite(LED, estadoLED ? HIGH : LOW);
}
