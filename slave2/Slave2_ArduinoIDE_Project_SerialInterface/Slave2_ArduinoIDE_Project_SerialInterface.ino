#include <HardwareSerial.h>
 
// Create a HardwareSerial object to communicate with the MAX485 module
HardwareSerial mySerial(2); // Using UART2 (TX2, RX2)
 
// Define Modbus parameters
const byte slaveAddress = 0x01;          // Address of the Modbus slave device
const byte functionCode = 0x03;          // Function code to read holding registers
const byte startAddressHigh = 0x00;      // High byte of the starting address
const byte startAddressLow = 0x00;       // Low byte of the starting address
const byte registerCountHigh = 0x00;     // High byte of the number of registers to read
const byte registerCountLow = 0x02;      // Low byte of the number of registers to read
 
void setup() {
  // Initialize serial communication for debugging
  Serial.begin(115200);
  // Initialize HardwareSerial for Modbus communication
  mySerial.begin(9600, SERIAL_8N1, 16, 17); // RX=16, TX=17 for UART2
 
  // Allow some time for initialization
  delay(1000);
}
 
void loop() {
  // Create a request frame for Modbus communication
  byte requestFrame[8];
  constructModbusRequest(requestFrame, slaveAddress, functionCode, startAddressHigh, startAddressLow, registerCountHigh, registerCountLow);
 
  // Send the Modbus request frame
  sendModbusRequest(requestFrame, 8);
 
  // Read and process the Modbus response frame
  if (mySerial.available()) {
    // Create a buffer to store the response frame
    byte responseFrame[9];
    // Read the response frame from the slave device
    readModbusResponse(responseFrame, 9);
    // Verify the CRC of the received response frame
    if (verifyCRC(responseFrame, 9)) {
      // Process the response frame to extract data
      processModbusResponse(responseFrame);
    } else {
      // Print an error message if CRC verification fails
      Serial.println("CRC error.");
    }
  } else {
    // Print an error message if no response is received
    Serial.println("No response from slave.");
  }
 
  // Wait for 2 seconds before the next request
  delay(2000);
}
 
// Function to construct a Modbus request frame
void constructModbusRequest(byte *frame, byte address, byte function, byte startHigh, byte startLow, byte countHigh, byte countLow) {
  frame[0] = address;          // Address of the slave device
  frame[1] = function;         // Function code
  frame[2] = startHigh;        // High byte of the starting address
  frame[3] = startLow;         // Low byte of the starting address
  frame[4] = countHigh;        // High byte of the number of registers to read
  frame[5] = countLow;         // Low byte of the number of registers to read
 
  // Calculate and append the CRC to the request frame
  uint16_t crc = calculateCRC(frame, 6);
  frame[6] = crc & 0xFF;         // CRC low byte
  frame[7] = (crc >> 8) & 0xFF;  // CRC high byte
}
 
// Function to send a Modbus request frame
void sendModbusRequest(byte *frame, byte length) {
  for (byte i = 0; i < length; i++) {
    mySerial.write(frame[i]); // Send each byte of the frame
  }
}
 
// Function to read a Modbus response frame
// void readModbusResponse(byte *frame, byte length) {
//   for (byte i = 0; i < length; i++) {
//     if (mySerial.available()) {
//       // frame[i] = mySerial.read(); // Read each byte of the frame
//       frame[i] = 0x88; // Dados fictícios
//     }
//   }
// }

void readModbusResponse(byte *frame, byte length) {
  // Simulando uma resposta válida:
  // ID do slave: 0x01
  // Function: 0x03
  // Byte count: 0x04 (dois registradores)
  // Dados: umidade = 625 (62.5%), temperatura = 233 (23.3°C)
  frame[0] = 0x01;
  frame[1] = 0x03;
  frame[2] = 0x04;
  frame[3] = 0x02; // Humidade alto
  frame[4] = 0x71; // Humidade baixo => 0x0271 = 625
  frame[5] = 0x00; // Temperatura alto
  frame[6] = 0xE9; // Temperatura baixo => 0x00E9 = 233

  // Calcular CRC real da resposta simulada
  uint16_t crc = calculateCRC(frame, 7);
  frame[7] = crc & 0xFF;
  frame[8] = (crc >> 8) & 0xFF;
}
 
// Function to verify the CRC of a Modbus frame
bool verifyCRC(byte *frame, byte length) {
  uint16_t receivedCRC = (frame[length - 1] << 8) | frame[length - 2]; // Extract the received CRC
  // Calculate the CRC of the received frame (excluding the received CRC bytes)
  return calculateCRC(frame, length - 2) == receivedCRC;
}
 
// Function to process the Modbus response frame and extract data
void processModbusResponse(byte *frame) {
  // Extract the humidity and temperature data from the response frame
  uint16_t humidity = (frame[3] << 8) | frame[4];
  uint16_t temperature = (frame[5] << 8) | frame[6];
 
  // Convert the raw data to actual values
  float humidityValue = humidity / 10.0;
  float temperatureValue = temperature / 10.0;
 
  // Print the humidity and temperature values to the Serial Monitor
  Serial.print("Humidity: ");
  Serial.print(humidityValue);
  Serial.println(" %RH");
 
  Serial.print("Temperature: ");
  Serial.print(temperatureValue);
  Serial.println(" °C");
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
