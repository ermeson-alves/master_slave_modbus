// #include <HardwareSerial.h>
 
// Create a HardwareSerial object to communicate with the MAX485 module
// HardwareSerial mySerial(2); // Configuração para utilizar a UART2. No ESP32 wroom RX2-pino 16 e TX2 pino 17
 
// Define Modbus parameters
const byte slaveAddress = 0x03;          // Address of the Modbus slave device

const byte functionCode = 0x03;          // Function code to read holding registers
const byte startAddressHigh = 0x00;      // High byte of the starting address
const byte startAddressLow = 0x00;       // Low byte of the starting address
const byte registerCountHigh = 0x00;     // High byte of the number of registers to read
const byte registerCountLow = 0x02;      // Low byte of the number of registers to read
 
void setup() {
  // Comunicação para debuggar 
  Serial.begin(115200); 
  // Initialize HardwareSerial for Modbus communication
  Serial2.begin(19200, SERIAL_8N1, 16, 17); // No ESP32 wroom RX2-pino 16 e TX2 pino 1
 
  // Allow some time for initialization
  delay(1000);
}

void loop() {
  // Verifica se chegaram ao menos os 8 bytes mínimos de uma requisição Modbus  
  if (Serial2.available() >= 8) {
    Serial.println("Recebeu dados!");
    byte request[8];
    for (int i = 0; i < 8; i++) {
      request[i] = Serial2.read();
    }

    // Verifica se a requisição é para este escravo e tem CRC válido
    if (request[0] == slaveAddress && verifyCRC(request, 8)) {
      // Simula valores de sensores
      uint16_t humidity = 650;     // 65.0 %RH
      uint16_t temperature = 237;  // 23.7 °C

      // Constrói resposta com os dados simulados
      byte response[9];
      response[0] = slaveAddress;
      response[1] = functionCode;
      response[2] = 4; // número de bytes de dados
      response[3] = humidity >> 8;
      response[4] = humidity & 0xFF;
      response[5] = temperature >> 8;
      response[6] = temperature & 0xFF;

      uint16_t crc = calculateCRC(response, 7);
      response[7] = crc & 0xFF;
      response[8] = (crc >> 8) & 0xFF;

      // Envia a resposta para o mestre
      Serial2.write(response, 9);

      Serial.println("Requisicao atendida e resposta enviada.");
    } else {
      Serial.println("Requisicao inválida ou para outro escravo.");
    }
  } 
  else {
    Serial.println("Nao recebeu bytes de dados!");
  }
}

 
// Function to verify the CRC of a Modbus frame
bool verifyCRC(byte *frame, byte length) {
  uint16_t receivedCRC = (frame[length - 1] << 8) | frame[length - 2]; // Extract the received CRC
  // Calculate the CRC of the received frame (excluding the received CRC bytes)
  return calculateCRC(frame, length - 2) == receivedCRC;
}

// Function to calculate the CRC of a Modbus frame
uint16_t calculateCRC(byte *frame, byte length) {
  uint16_t crc = 0xFFFF; // Initialize CRC to 0xFFFF
  for (byte i = 0; i < length; i++) {
    crc ^= frame[i]; // XOR the frame byte with the CRC
    for (byte j = 0; j < 8; j++) {
      if (crc & 0x0001) { // Check if the LSB of the CRC is 1
        crc >>= 1;        // Right shift the CRC
        crc ^= 0xA001;    // XOR the CRC with the polynomial 0xA001
      } else {
        crc >>= 1;        // Right shift the CRC
      }
    }
  }
  return crc; // Return the calculated CRC
}