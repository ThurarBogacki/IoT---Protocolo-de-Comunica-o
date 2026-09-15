#include <SoftwareSerial.h>

// ===== Protocolo (manter idêntico no Receptor) =====
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
const long BAUD_RATE = 2400;

const uint8_t MAX_TENTATIVAS = 5;
const unsigned long TIMEOUT_RESPOSTA_MS = 500;
const unsigned long ESPERA_RETRANSMISSAO_MS = 200;
const unsigned long TIMEOUT_LINHA_MS = 100;
const uint16_t TAMANHO_ENTRADA = TAMANHO_MAX_DATA + 1;

SoftwareSerial portaSerial(10, 11); // RX, TX

// Falhas que o usuário pode provocar para demonstrar a recuperação do protocolo.
enum FalhaSimulada {
  SEM_FALHA,
  CORROMPER_PRIMEIRA,
  CORROMPER_TODAS,
  PERDER_QUADRO,
  PERDER_ACK
};

FalhaSimulada falhaSimulada = SEM_FALHA;
uint8_t sequencia;

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

void transmitirQuadro(uint8_t tipo, const uint8_t *payload, uint16_t tamanho, bool corromper) {
  const uint8_t cabecalho[] = { tipo, sequencia, highByte(tamanho), lowByte(tamanho) };
  uint8_t crc = crc8(payload, tamanho, crc8(cabecalho, sizeof(cabecalho), 0));

  portaSerial.write(INICIO_QUADRO);
  portaSerial.write(cabecalho, sizeof(cabecalho));
  for (uint16_t i = 0; i < tamanho; i++) {
    uint8_t valor = payload[i];
    if (corromper && i == 0) {
      valor ^= 0x01; // simula ruído: inverte 1 bit sem recalcular o CRC
    }
    portaSerial.write(valor);
  }
  portaSerial.write(crc);
}

void imprimirHex(uint8_t valor) {
  Serial.print(F(" 0x"));
  if (valor < 0x10) {
    Serial.print('0');
  }
  Serial.print(valor, HEX);
}

// Retorna true somente para um ACK da sequência atual.
bool aguardarAck(bool simularPerdaAck) {
  uint8_t resposta[2];
  if (!lerBytes(resposta, sizeof(resposta), TIMEOUT_RESPOSTA_MS)) {
    Serial.println(F("[TX] Timeout: nenhuma resposta do receptor"));
    return false;
  }
  if (resposta[0] == NACK) {
    Serial.println(F("[TX] NACK recebido: receptor detectou erro no quadro"));
    return false;
  }
  if (resposta[0] != ACK || resposta[1] != sequencia) {
    Serial.print(F("[TX] Resposta invalida do receptor:"));
    imprimirHex(resposta[0]);
    imprimirHex(resposta[1]);
    Serial.println();
    return false;
  }
  if (simularPerdaAck) {
    Serial.println(F("[SIMULACAO] ACK perdido na linha (tratado como timeout)"));
    return false;
  }
  Serial.println(F("[TX] ACK recebido"));
  return true;
}

// Stop-and-Wait ARQ: retransmite o mesmo quadro (mesma sequência) após NACK, timeout ou resposta inválida.
// Implementar um Go-Back-N ARQ ou outros algoritmos com janela deslizante eh possivel, mas mais trabalhoso
bool enviarQuadro(uint8_t tipo, const uint8_t *payload, uint16_t tamanho) {
  sequencia++;

  for (uint8_t tentativa = 1; tentativa <= MAX_TENTATIVAS; tentativa++) {
    bool primeira = tentativa == 1;

    Serial.print(F("[TX] Tentativa "));
    Serial.print(tentativa);
    Serial.print(F("/"));
    Serial.print(MAX_TENTATIVAS);
    Serial.print(F(" (seq "));
    Serial.print(sequencia);
    Serial.println(F(")"));

    // Descarta respostas atrasadas de tentativas anteriores
    while (portaSerial.available() > 0) {
      portaSerial.read();
    }

    if (falhaSimulada == PERDER_QUADRO && primeira) {
      Serial.println(F("[SIMULACAO] Quadro perdido na linha"));
    } else {
      bool corromper = falhaSimulada == CORROMPER_TODAS || (falhaSimulada == CORROMPER_PRIMEIRA && primeira);
      if (corromper) {
        Serial.println(F("[SIMULACAO] Quadro corrompido (1 bit invertido)"));
      }
      // A SoftwareSerial amostra os bits por temporização: esperar o log sair evita que as
      // interrupções da Serial nativa atrasem a leitura da resposta
      Serial.flush();
      transmitirQuadro(tipo, payload, tamanho, corromper);
    }

    if (aguardarAck(falhaSimulada == PERDER_ACK && primeira)) {
      return true;
    }
    if (tentativa < MAX_TENTATIVAS) {
      delay(ESPERA_RETRANSMISSAO_MS);
    }
  }

  Serial.println(F("[TX] Limite de tentativas atingido"));
  return false;
}

// ===== API do protocolo =====

bool sendByte(uint8_t value) {
  return enviarQuadro(TIPO_BYTE, &value, 1);
}

bool sendWord(uint16_t value) {
  const uint8_t payload[] = { highByte(value), lowByte(value) };
  return enviarQuadro(TIPO_WORD, payload, sizeof(payload));
}

bool sendFloat(float value) {
  uint32_t bits;
  memcpy(&bits, &value, sizeof(bits)); // IEEE 754, enviado em big-endian como os demais campos
  const uint8_t payload[] = {
    (uint8_t)(bits >> 24), (uint8_t)(bits >> 16), (uint8_t)(bits >> 8), (uint8_t)bits
  };
  return enviarQuadro(TIPO_FLOAT, payload, sizeof(payload));
}

bool sendData(const uint8_t *data, uint16_t size) {
  if (data == NULL || size == 0 || size > TAMANHO_MAX_DATA) {
    Serial.println(F("[TX] Dados invalidos ou tamanho fora do limite"));
    return false;
  }
  return enviarQuadro(TIPO_DATA, data, size);
}

// ===== Interface com o usuário (Monitor Serial) =====

// Funciona com ou sem terminador de linha no Monitor Serial.
// Retorna quantos caracteres foram digitados, mesmo que excedam a capacidade do destino.
uint16_t lerLinha(char *destino, uint16_t capacidade) {
  while (Serial.available() == 0) {}

  uint16_t digitados = 0;
  unsigned long ultimoCaractere = millis();
  while (millis() - ultimoCaractere < TIMEOUT_LINHA_MS) {
    if (Serial.available() == 0) {
      continue;
    }
    char c = Serial.read();
    ultimoCaractere = millis();
    if (c == '\n') {
      break;
    }
    if (c == '\r') {
      continue;
    }
    if (digitados < capacidade - 1) {
      destino[digitados] = c;
    }
    digitados++;
  }
  destino[digitados < capacidade ? digitados : capacidade - 1] = '\0';
  return digitados;
}

bool lerInteiro(long minimo, long maximo, long &valor) {
  char entrada[TAMANHO_ENTRADA];
  lerLinha(entrada, sizeof(entrada));
  char *fim;
  valor = strtol(entrada, &fim, 10);
  return fim != entrada && *fim == '\0' && valor >= minimo && valor <= maximo;
}

bool lerFloat(float &valor) {
  char entrada[TAMANHO_ENTRADA];
  lerLinha(entrada, sizeof(entrada));
  char *fim;
  valor = strtod(entrada, &fim);
  return fim != entrada && *fim == '\0';
}

void escolherFalha() {
  Serial.println(F("\nSimular falha?"));
  Serial.println(F(" [0] Nenhuma"));
  Serial.println(F(" [1] Corromper a 1a tentativa   (NACK -> retransmissao)"));
  Serial.println(F(" [2] Corromper todas            (falha definitiva)"));
  Serial.println(F(" [3] Perder o quadro na 1a      (timeout -> retransmissao)"));
  Serial.println(F(" [4] Perder o ACK na 1a         (retransmissao -> duplicata descartada)"));

  long opcao;
  while (!lerInteiro(SEM_FALHA, PERDER_ACK, opcao)) {
    Serial.println(F("Opcao invalida. Digite de 0 a 4:"));
  }
  falhaSimulada = (FalhaSimulada)opcao;
}

void informarResultado(bool entregue) {
  if (entregue) {
    Serial.println(F("[RESULTADO] SUCESSO: mensagem confirmada pelo receptor"));
  } else {
    Serial.println(F("[RESULTADO] FALHA: mensagem nao confirmada pelo receptor"));
  }
  falhaSimulada = SEM_FALHA;
}

void enviarByteDigitado() {
  Serial.println(F("Informe o BYTE (0 a 255):"));
  long valor;
  if (!lerInteiro(0, 255, valor)) {
    Serial.println(F("[ERRO] Valor invalido"));
    return;
  }
  escolherFalha();
  informarResultado(sendByte((uint8_t)valor));
}

void enviarWordDigitada() {
  Serial.println(F("Informe a WORD (0 a 65535):"));
  long valor;
  if (!lerInteiro(0, 65535, valor)) {
    Serial.println(F("[ERRO] Valor invalido"));
    return;
  }
  escolherFalha();
  informarResultado(sendWord((uint16_t)valor));
}

void enviarFloatDigitado() {
  Serial.println(F("Informe o FLOAT (ex: 3.1415):"));
  float valor;
  if (!lerFloat(valor)) {
    Serial.println(F("[ERRO] Valor invalido"));
    return;
  }
  escolherFalha();
  informarResultado(sendFloat(valor));
}

void enviarDataDigitado() {
  Serial.print(F("Informe o tamanho da mensagem (1 a "));
  Serial.print(TAMANHO_MAX_DATA);
  Serial.println(F("):"));
  long tamanho;
  if (!lerInteiro(1, TAMANHO_MAX_DATA, tamanho)) {
    Serial.println(F("[ERRO] Tamanho invalido"));
    return;
  }

  Serial.println(F("Informe a mensagem:"));
  char mensagem[TAMANHO_ENTRADA];
  uint16_t digitados = lerLinha(mensagem, sizeof(mensagem));
  if (digitados < tamanho) {
    Serial.println(F("[ERRO] Mensagem menor que o tamanho informado"));
    return;
  }
  if (digitados > tamanho) {
    Serial.println(F("[AVISO] Mensagem truncada no tamanho informado"));
  }

  escolherFalha();
  informarResultado(sendData((const uint8_t *)mensagem, (uint16_t)tamanho));
}

void exibirMenu() {
  Serial.println(F("\n================================="));
  Serial.println(F("   PROTOCOLO SERIAL - TRANSMISSOR"));
  Serial.println(F("================================="));
  Serial.println(F(" [1] Enviar BYTE"));
  Serial.println(F(" [2] Enviar WORD"));
  Serial.println(F(" [3] Enviar FLOAT"));
  Serial.println(F(" [4] Enviar DATA (tamanho definido pelo usuario)"));
  Serial.println(F("Digite a opcao:"));
}

void setup() {
  Serial.begin(BAUD_RATE);
  portaSerial.begin(BAUD_RATE);
  while (!Serial);

  // Sequência inicial aleatória: se só o transmissor reiniciar, a 1a mensagem não é tomada como duplicata
  randomSeed(analogRead(A0));
  sequencia = random(256);

  exibirMenu();
}

void loop() {
  long opcao;
  if (!lerInteiro(1, 4, opcao)) {
    Serial.println(F("[ERRO] Opcao invalida. Escolha de 1 a 4."));
    exibirMenu();
    return;
  }

  switch (opcao) {
    case 1: enviarByteDigitado(); break;
    case 2: enviarWordDigitada(); break;
    case 3: enviarFloatDigitado(); break;
    case 4: enviarDataDigitado(); break;
  }
  exibirMenu();
}
