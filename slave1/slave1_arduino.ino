#include <HardwareSerial.h>
#include <math.h>  // Necessário para log()

// Utiliza a UART2 do ESP32 (GPIO16 RX, GPIO17 TX)
HardwareSerial mySerial(2);

// Define o pino que controla o modo RE/DE do MAX485
#define MAX485_CONTROL_PIN 4

// Pino analógico do sensor NTC
#define sensorPin 34

// Parâmetros do sensor NTC
const float seriesResistor = 5000.0;         // Resistor fixo de 5kΩ em série com o NTC
const float nominalResistance = 5000.0;      // Resistência do NTC a 25 °C
const float nominalTemperature = 25.0;       // Temperatura de referência
const float bCoefficient = 3950.0;           // Coeficiente beta do NTC
const int adcMax = 4095;                     // Resolução ADC 12 bits
const float vRef = 3.3;                      // Tensão de referência ADC

// Endereço Modbus deste escravo
const byte slaveAddress = 0x01;

// Vetor com dois registradores:
// [0] = registrador de mensagem (escrita)
// [1] = registrador de temperatura (leitura)
uint16_t holdingRegisters[2] = {0, 0};

void setup() {
  Serial.begin(115200);  // Comunicação serial para debug
  mySerial.begin(9600, SERIAL_8N1, 16, 17);  // UART2 com baudrate 9600

  pinMode(MAX485_CONTROL_PIN, OUTPUT);
  digitalWrite(MAX485_CONTROL_PIN, LOW);  // Inicia em modo recepção
  pinMode(sensorPin, INPUT);              // Pino do sensor como entrada

  Serial.println("Escravo Modbus iniciado");
}

void loop() {
  atualizarTemperatura();  // Atualiza valor do registrador de temperatura

  // Verifica se há dados recebidos via Modbus (UART2)
  if (mySerial.available()) {
    byte request[8];

    // Lê 8 bytes da requisição
    if (readModbusRequest(request, 8)) {

      // Verifica se CRC é válido e se o endereço é o do escravo
      if (verifyCRC(request, 8) && request[0] == slaveAddress) {
        byte functionCode = request[1];

        if (functionCode == 0x03) {  // Leitura de registradores
          uint16_t startAddr = (request[2] << 8) | request[3];
          uint16_t qtyRegs = (request[4] << 8) | request[5];

          // Garante que o número de registradores requisitados é válido
          if (startAddr + qtyRegs <= 2) {
            byte response[5 + qtyRegs * 2];
            response[0] = slaveAddress;
            response[1] = 0x03;
            response[2] = qtyRegs * 2;  // Número de bytes de dados

            // Copia os dados dos registradores para a resposta
            for (int i = 0; i < qtyRegs; i++) {
              response[3 + i * 2] = (holdingRegisters[startAddr + i] >> 8) & 0xFF;
              response[4 + i * 2] = holdingRegisters[startAddr + i] & 0xFF;
            }

            // Calcula e adiciona o CRC
            uint16_t crc = calculateCRC(response, 3 + qtyRegs * 2);
            response[3 + qtyRegs * 2] = crc & 0xFF;
            response[4 + qtyRegs * 2] = (crc >> 8) & 0xFF;

            sendModbusResponse(response, 5 + qtyRegs * 2);
            Serial.println("Resposta enviada ao mestre.");
          }

        } else if (functionCode == 0x06) {  // Escrita de registrador único
          uint16_t regAddr = (request[2] << 8) | request[3];
          uint16_t value = (request[4] << 8) | request[5];

          // Se o mestre quer escrever no registrador 0 (mensagem)
          if (regAddr == 0) {
            holdingRegisters[0] = value;  // Armazena valor
            processMensagem(value);      // Processa a mensagem

            // Retorna o mesmo quadro recebido como resposta
            sendModbusResponse(request, 8);
            Serial.print("Mensagem recebida e processada: ");
            Serial.println(value);
          }
        }
      } else {
        Serial.println("Requisição com erro de CRC ou endereço incorreto.");
      }
    }
  }
}

// Lê o valor do sensor NTC e atualiza o registrador [1]
void atualizarTemperatura() {
  int adcValue = analogRead(sensorPin);

  // Converte valor ADC em tensão
  float voltage = adcValue * vRef / adcMax;

  // Calcula resistência do termistor
  float resistance = (vRef - voltage) * seriesResistor / voltage;

  // Aplica equação de Steinhart-Hart (simplificada)
  float steinhart = resistance / nominalResistance;
  steinhart = log(steinhart);
  steinhart /= bCoefficient;
  steinhart += 1.0 / (nominalTemperature + 273.15);
  steinhart = 1.0 / steinhart;
  steinhart -= 273.15;  // Converte de Kelvin para Celsius

  // Armazena a temperatura em décimos de grau (ex: 25.0°C → 250)
  holdingRegisters[1] = (uint16_t)(steinhart * 10.0);
}

// Interpreta valor escrito no registrador 0 e imprime mensagem na Serial
void processMensagem(uint16_t valor) {
  switch (valor) {
    case 1:
      Serial.println("Boa noite");
      break;
    case 2:
      Serial.println("Bom dia");
      break;
    case 3:
      Serial.println("Mensagem recebida: sistema ativo");
      break;
    default:
      Serial.print("Código de mensagem desconhecido: ");
      Serial.println(valor);
      break;
  }
}

// Lê quadro Modbus da UART2
bool readModbusRequest(byte *frame, byte length) {
  unsigned long timeout = millis() + 500;
  byte index = 0;
  while (index < length) {
    if (mySerial.available()) {
      frame[index++] = mySerial.read();
      timeout = millis() + 500;
    }
    if (millis() > timeout) {
      Serial.println("Timeout na leitura da requisição");
      return false;
    }
  }
  return true;
}

// Envia quadro Modbus como resposta via MAX485
void sendModbusResponse(byte *frame, byte length) {
  digitalWrite(MAX485_CONTROL_PIN, HIGH);  // Ativa modo transmissão
  delay(2);

  for (byte i = 0; i < length; i++) {
    mySerial.write(frame[i]);
  }

  mySerial.flush();  // Aguarda fim da transmissão
  delay(2);
  digitalWrite(MAX485_CONTROL_PIN, LOW);  // Volta ao modo recepção
}

// Verifica se o CRC recebido confere com o CRC calculado
bool verifyCRC(byte *frame, byte length) {
  uint16_t receivedCRC = (frame[length - 1] << 8) | frame[length - 2];
  uint16_t calculatedCRC = calculateCRC(frame, length - 2);
  return receivedCRC == calculatedCRC;
}

// Calcula o CRC-16 Modbus
uint16_t calculateCRC(byte *frame, byte length) {
  uint16_t crc = 0xFFFF;
  for (byte i = 0; i < length; i++) {
    crc ^= frame[i];
    for (byte j = 0; j < 8; j++) {
      if (crc & 0x0001) {
        crc >>= 1;
        crc ^= 0xA001;
      } else {
        crc >>= 1;
      }
    }
  }
  return crc;
}
