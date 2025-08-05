#include <ModbusRTU.h>
#include <HardwareSerial.h>

// --- FIX FOR LED_BUILTIN ---
// Define the LED pin if it's not already, usually GPIO 2 for ESP32
#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif

// --- SETTINGS FROM YOUR CONFIGURATION ---
#define SLAVE_ID 2
#define BAUD_RATE 9600

// Using Serial2 (RX=16, TX=17) for an adapter with automatic direction control
HardwareSerial& modbusSerial = Serial2; 

ModbusRTU slave;

// Define the number of registers for each type
#define COIL_COUNT 10
#define ISTS_COUNT 9  // Discrete Inputs are called "Input Status" (Ists) in the library
#define IREG_COUNT 4
#define HREG_COUNT 2

//------------------------------------------------------------------------------------------------
// Controle interno de tempo para simulação
unsigned long lastFimCursoToggle = 0;
unsigned long lastVibracaoToggle = 0;
bool fimCursoEstado = false;
bool vibracaoEstado = false;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("ESP32 Modbus Slave - All Registers Enabled");

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

  // 1. Coils: Make the ESP32's built-in LED follow the state of Coil 0.
  // You can control this by forcing a value to %QX100.0 in OpenPLC.
  bool coil0_status = slave.Coil(0);
  digitalWrite(LED_BUILTIN, coil0_status);

  // --- Novos Coils e registradores ---
  bool motorLigado = slave.Coil(2); // %QX0.2 → Liga motor
  bool resetFalha = slave.Coil(1);  // %QX0.1 → Resetar falha

  // Estados internos
  static bool falhaDetectada = false;
  static unsigned long tempoUltimaAtualizacao = millis();
  static unsigned long tempoOperacao = 0; // segundos

  // Corrente simulada apenas se motor estiver ligado
  float corrente = motorLigado ? (10 + 5 * sin(millis() / 1000.0)) : 0;
  slave.Ireg(0, (uint16_t)(corrente * 10)); // Corrente x10 (%IW0)

  // Temperatura simulada (aumenta com tempo se motor ligado)
  float temperatura = motorLigado ? (25 + 0.05 * (millis() / 1000.0)) : 25;
  slave.Ireg(1, (uint16_t)(temperatura * 10)); // Temperatura x10 (%IW1)

  // Limite de corrente (pelo mestre)
  uint16_t limite = slave.Hreg(0);

  // Detecção de falha
  if (!falhaDetectada && (uint16_t)(corrente * 10) > limite) {
    falhaDetectada = true;
    motorLigado = false; // Trava o motor
    slave.Coil(2, false); // Desliga via lógica
    slave.Ists(1, true);  // Sinaliza falha (%IX1)
    Serial.println(">>> FALHA: Sobrecorrente detectada! <<<");
  }

  // Reset da falha
  if (resetFalha && falhaDetectada) {
    falhaDetectada = false;
    slave.Ists(1, false); // Limpa falha
    Serial.println(">>> FALHA RESETADA <<<");
  }

  // Tempo de operação
  unsigned long agora = millis();
  if (motorLigado && !falhaDetectada && agora - tempoUltimaAtualizacao >= 1000) {
    tempoOperacao++;
    slave.Hreg(1, tempoOperacao); // %QW1
    tempoUltimaAtualizacao = agora;
  }

  // LED feedback
  if (falhaDetectada) {
    digitalWrite(LED_BUILTIN, millis() % 500 < 250); // Pisca rápido
  } else if (motorLigado) {
    digitalWrite(LED_BUILTIN, HIGH); // LED ligado fixo
  } else {
    digitalWrite(LED_BUILTIN, LOW); // LED desligado
  }

}