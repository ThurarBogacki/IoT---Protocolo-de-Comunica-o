#include <Arduino.h>
#include <SoftwareSerial.h>

// ===== Protocolo (manter idêntico no Transmissor) =====
// Quadro:   [0xAA][TIPO][SEQ][TAM_H][TAM_L][PAYLOAD][CRC8]
// Resposta: [ACK|NACK][SEQ]
const uint8_t INICIO_QUADRO = 0xAA;
const uint8_t TIPO_BYTE = 0x01;
const uint8_t TIPO_WORD = 0x02;
const uint8_t TIPO_FLOAT = 0x03;
const uint8_t TIPO_DATA = 0x04;
const uint8_t ACK = 0x06;
const uint8_t NACK = 0x15;
const uint16_t TAMANHO_MAX_DATA = 64;
const long BAUD_RATE = 4800;

const unsigned long TIMEOUT_ENTRE_BYTES_MS = 50;

SoftwareSerial portaSerial(10, 11); // RX, TX

int16_t ultimaSequencia = -1; // última mensagem aceita, para descartar retransmissões duplicadas

// ===== Camada de enlace =====

// CRC-8 (polinômio 0x07)
uint8_t crc8(const uint8_t *dados, uint16_t tamanho, uint8_t crc) {
  for (uint16_t i = 0; i < tamanho; i++) {
    crc ^= dados[i];
    for (uint8_t bit = 0; bit < 8; bit++) {
      crc = (crc & 0x80) ? (crc << 1) ^ 0x07 : crc << 1;
    }
  }
  return crc;
}

// Falha se qualquer byte demorar mais que timeoutMs para chegar.
bool lerBytes(uint8_t *destino, uint16_t quantidade, unsigned long timeoutMs) {
  for (uint16_t i = 0; i < quantidade; i++) {
    unsigned long inicio = millis();
    while (portaSerial.available() == 0) {
      if (millis() - inicio >= timeoutMs) {
        return false;
      }
    }
    destino[i] = portaSerial.read();
  }
  return true;
}

// Espera a linha silenciar, descartando o restante de um quadro inválido para não perder a sincronia.
void descartarEntrada() {
  unsigned long ultimoByte = millis();
  while (millis() - ultimoByte < TIMEOUT_ENTRE_BYTES_MS) {
    if (portaSerial.available() > 0) {
      portaSerial.read();
      ultimoByte = millis();
    }
  }
}

void responder(uint8_t codigo, uint8_t seq) {
  portaSerial.write(codigo);
  portaSerial.write(seq);
}

void rejeitarQuadro(uint8_t seq, const __FlashStringHelper *motivo) {
  descartarEntrada();
  responder(NACK, seq);
  Serial.print(F("[RX] NACK enviado: "));
  Serial.println(motivo);
}

bool tamanhoValido(uint8_t tipo, uint16_t tamanho) {
  switch (tipo) {
    case TIPO_BYTE:  return tamanho == 1;
    case TIPO_WORD:  return tamanho == 2;
    case TIPO_FLOAT: return tamanho == 4;
    case TIPO_DATA:  return tamanho >= 1 && tamanho <= TAMANHO_MAX_DATA;
    default:         return false;
  }
}

// ===== Apresentação =====

void exibirDados(uint8_t tipo, uint8_t seq, const uint8_t *payload, uint16_t tamanho) {
  Serial.print(F("[RX] seq "));
  Serial.print(seq);
  Serial.print(F(" | "));

  switch (tipo) {
    case TIPO_BYTE:
      Serial.print(F("BYTE: "));
      Serial.println(payload[0]);
      break;

    case TIPO_WORD:
      Serial.print(F("WORD: "));
      Serial.println(word(payload[0], payload[1]));
      break;

    case TIPO_FLOAT: {
      uint32_t bits = ((uint32_t)payload[0] << 24) | ((uint32_t)payload[1] << 16) |
                      ((uint32_t)payload[2] << 8) | payload[3];
      float valor;
      memcpy(&valor, &bits, sizeof(valor));
      Serial.print(F("FLOAT: "));
      Serial.println(valor, 4);
      break;
    }

    case TIPO_DATA:
      Serial.print(F("DATA ("));
      Serial.print(tamanho);
      Serial.print(F(" bytes): "));
      Serial.write(payload, tamanho);
      Serial.println();
      break;
  }
}

// ===== Recepção =====

void receberQuadro() {
  uint8_t cabecalho[4] = { 0 }; // [TIPO][SEQ][TAM_H][TAM_L]
  bool cabecalhoCompleto = lerBytes(cabecalho, sizeof(cabecalho), TIMEOUT_ENTRE_BYTES_MS);
  uint8_t tipo = cabecalho[0];
  uint8_t seq = cabecalho[1];
  uint16_t tamanho = word(cabecalho[2], cabecalho[3]);

  if (!cabecalhoCompleto) {
    rejeitarQuadro(seq, F("quadro incompleto"));
    return;
  }
  // Validar antes de ler o payload também impede estouro do buffer
  if (!tamanhoValido(tipo, tamanho)) {
    rejeitarQuadro(seq, F("tipo desconhecido ou tamanho incompativel"));
    return;
  }

  uint8_t payload[TAMANHO_MAX_DATA];
  uint8_t crcRecebido;
  if (!lerBytes(payload, tamanho, TIMEOUT_ENTRE_BYTES_MS) ||
      !lerBytes(&crcRecebido, 1, TIMEOUT_ENTRE_BYTES_MS)) {
    rejeitarQuadro(seq, F("quadro incompleto"));
    return;
  }

  if (crc8(payload, tamanho, crc8(cabecalho, sizeof(cabecalho), 0)) != crcRecebido) {
    rejeitarQuadro(seq, F("CRC invalido"));
    return;
  }

  // ACK antes de imprimir, para não consumir o timeout do transmissor
  responder(ACK, seq);

  if (seq == ultimaSequencia) {
    Serial.print(F("[RX] seq "));
    Serial.print(seq);
    Serial.println(F(" | duplicata: ACK reenviado e dados descartados"));
    return;
  }
  ultimaSequencia = seq;
  exibirDados(tipo, seq, payload, tamanho);
}

void setup() {
  Serial.begin(BAUD_RATE);
  portaSerial.begin(BAUD_RATE);
  Serial.println(F("[RX] Receptor pronto"));
}

void loop() {
  // Bytes fora de um quadro são ignorados até o próximo byte de início
  if (portaSerial.available() > 0 && portaSerial.read() == INICIO_QUADRO) {
    receberQuadro();
  }
}
