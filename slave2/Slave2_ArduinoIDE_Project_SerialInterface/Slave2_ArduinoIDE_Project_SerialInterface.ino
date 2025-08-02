#include <Arduino.h>

// Configurações Modbus
const byte SLAVE_ID = 0x02;  // Endereço do escravo
#define RE_DE 4              // Pino que controla o RS485 (HIGH = Tx, LOW = Rx)

// Simulação de 10 registradores Holding
uint16_t holdingRegisters[10] = {650, 237, 100, 200, 300, 400, 500, 600, 700, 800};

void setup() {
  Serial.begin(115200);
  Serial2.begin(19200, SERIAL_8N1, 16, 17);  // UART2: RX=16, TX=17

  pinMode(RE_DE, OUTPUT);
  digitalWrite(RE_DE, LOW);  // Inicialmente em modo recepção

  Serial.println("Slave Modbus RTU iniciado.");
}

void loop() {
  if (Serial2.available() >= 8) { // Pacote mínimo Modbus
    // Serial2.println("Qualquer coisa!");
    byte request[256];
    int len = Serial2.readBytes(request, 256);

    // Verifica se é para este slave e CRC correto
    // if (request[0] == SLAVE_ID && verifyCRC(request, len)) {
    if (request[0] == SLAVE_ID) {
      byte func = request[1];
      switch (func) {
        case 0x03: // Read Holding Registers
          handleReadHoldingRegisters(request);
          break;
        case 0x06: // Write Single Register
          handleWriteSingleRegister(request);
          break;
        case 0x10: // Write Multiple Registers
          handleWriteMultipleRegisters(request, len);
          break;
        default:
          Serial.println("Funcao Modbus nao suportada.");
          break;
      }
    } else {
      Serial.println("Requisicao invalida ou CRC incorreto.");
    }
  }
}

//
// 0x03 - Ler Holding Registers
//
void handleReadHoldingRegisters(byte* frame) {
  uint16_t startAddr = (frame[2] << 8) | frame[3];
  uint16_t numRegs   = (frame[4] << 8) | frame[5];

  byte response[256];
  response[0] = SLAVE_ID;
  response[1] = 0x03;
  response[2] = numRegs * 2; // Bytes de dados

  for (int i = 0; i < numRegs; i++) {
    uint16_t val = holdingRegisters[startAddr + i];
    response[3 + i*2] = val >> 8;
    response[4 + i*2] = val & 0xFF;
  }

  uint16_t crc = calculateCRC(response, 3 + numRegs*2);
  response[3 + numRegs*2] = crc & 0xFF;
  response[4 + numRegs*2] = (crc >> 8) & 0xFF;

  sendResponse(response, 5 + numRegs*2);
}

//
// 0x06 - Escrever um registrador
//
void handleWriteSingleRegister(byte* frame) {
  uint16_t regAddr = (frame[2] << 8) | frame[3];
  uint16_t value   = (frame[4] << 8) | frame[5];

  holdingRegisters[regAddr] = value;

  // Responde ecoando o comando
  uint16_t crc = calculateCRC(frame, 6);
  frame[6] = crc & 0xFF;
  frame[7] = (crc >> 8) & 0xFF;
  sendResponse(frame, 8);

  Serial.printf("Registrador %d atualizado para %d\n", regAddr, value);
}

//
// 0x10 - Escrever múltiplos registradores
//
void handleWriteMultipleRegisters(byte* frame, int len) {
  uint16_t startAddr = (frame[2] << 8) | frame[3];
  uint16_t numRegs   = (frame[4] << 8) | frame[5];
  byte byteCount     = frame[6];

  for (int i = 0; i < numRegs; i++) {
    uint16_t val = (frame[7 + i*2] << 8) | frame[8 + i*2];
    holdingRegisters[startAddr + i] = val;
  }

  // Resposta: ID, Func, StartAddr(2), NumRegs(2), CRC(2)
  byte response[8];
  response[0] = SLAVE_ID;
  response[1] = 0x10;
  response[2] = frame[2];
  response[3] = frame[3];
  response[4] = frame[4];
  response[5] = frame[5];

  uint16_t crc = calculateCRC(response, 6);
  response[6] = crc & 0xFF;
  response[7] = (crc >> 8) & 0xFF;

  sendResponse(response, 8);

  Serial.printf("%d registradores atualizados a partir de %d\n", numRegs, startAddr);
}

//
// Função para envio no RS485
//
void sendResponse(byte* frame, int len) {
  digitalWrite(RE_DE, HIGH); // Habilita transmissão
  delayMicroseconds(100);
  Serial2.write(frame, len);
  Serial2.flush();
  delayMicroseconds(100);
  digitalWrite(RE_DE, LOW);  // Volta para recepção
}

//
// Funções de CRC
//
bool verifyCRC(byte *frame, int length) {
  uint16_t receivedCRC = (frame[length-1] << 8) | frame[length-2];
  return calculateCRC(frame, length-2) == receivedCRC;
}

uint16_t calculateCRC(byte *frame, int length) {
  uint16_t crc = 0xFFFF;
  for (int pos = 0; pos < length; pos++) {
    crc ^= (uint16_t)frame[pos];
    for (int i = 0; i < 8; i++) {
      if (crc & 0x0001) crc = (crc >> 1) ^ 0xA001;
      else crc >>= 1;
    }
  }
  return crc;
}
