#include <SoftwareSerial.h>

SoftwareSerial portaSerial(10, 11);

void setup()
{
  Serial.begin(4800);
  portaSerial.begin(4800);
}

void loop()
{
  if(portaSerial.available() > 0){
  	uint8_t startByte = portaSerial.read();
  	
    if(startByte == 0xAA){
    	delay(5);
      
      uint8_t tipo = portaSerial.read();
      uint8_t tamanho = portaSerial.read();
      
      switch(tipo) {
      	case 0x01:
        	processarByte(tamanho);
        	break;
        
        case 0x02:
        	processarWord(tamanho);
        	break;
        
        case 0x03:
        	processarFloat(tamanho);
        	break;
        
        case 0x04:
        	processarData(tamanho);
        	break;
        
        default:
        	Serial.println("[ERRO] Tipo de dado desconhecido");
        	portaSerial.write(0x15); //NACK
        	break;
      }
    }
  }
}


void processarByte(uint8_t tamanho) {
  if (tamanho != 1) {
    Serial.println("[ERRO] Tamanho incompatível para Word!");
    portaSerial.write(0x15); // NACK 
    return;
  }
  uint8_t byte = portaSerial.read();

  uint8_t checksumRecebido = portaSerial.read();
  
  uint8_t checksumCalculado = 0x01 ^ tamanho ^ byte;
  
  if (checksumCalculado == checksumRecebido) {
      
    Serial.println("[SUCESSO] Byte recebida: ");
    Serial.println(byte);
    portaSerial.write(0x06); // ACK Ainda nao tratado no receptor
    
  } else {
    Serial.println("[ERRO] Falha no Checksum da Word!");
    portaSerial.write(0x15); // NACK Ainda nao tratado no receptor
  }
}



void processarWord(uint8_t tamanho) {
  if (tamanho != 2) {
    Serial.println("[ERRO] Tamanho incompatível para Word!");
    portaSerial.write(0x15); // NACK 
    return;
  }

  uint8_t byteAlto = portaSerial.read();
  uint8_t byteBaixo = portaSerial.read();
  uint8_t checksumRecebido = portaSerial.read();
  
  uint8_t checksumCalculado = 0x02 ^ tamanho ^ byteAlto ^ byteBaixo;
  
  if (checksumCalculado == checksumRecebido) {
    uint16_t wordRemontada = ((uint16_t)byteAlto << 8) | byteBaixo;
      
    Serial.println("[SUCESSO] Word recebida: ");
    Serial.println(wordRemontada);
    portaSerial.write(0x06); // ACK Ainda nao tratado no receptor
    
  } else {
    Serial.println("[ERRO] Falha no Checksum da Word!");
    portaSerial.write(0x15); // NACK Ainda nao tratado no receptor
  }
}


void processarFloat(uint8_t tamanho) {
  if (tamanho != 4) {
    Serial.println("[ERRO] Tamanho incompatível para Float!");
    portaSerial.write(0x15); // NACK 
    return;
  }

  uint8_t dadosByte[4];
  for(uint8_t i = 0; i < 4; i++){
  	dadosByte[i] = portaSerial.read();
  }
  
  uint8_t checksumRecebido = portaSerial.read();
  
  uint8_t checksumCalculado = 0x03 ^ tamanho;
  
  for(uint8_t i = 0; i < 4; i++){
  	checksumCalculado ^= dadosByte[i];
  }
  
  if (checksumCalculado == checksumRecebido) {
    float floatRemontada = *(float*)dadosByte;
      
    Serial.println("[SUCESSO] Float recebida: ");
    Serial.println(floatRemontada);
    portaSerial.write(0x06); // ACK Ainda nao tratado no receptor
    
  } else {
    Serial.println("[ERRO] Falha no Checksum da Float!");
    portaSerial.write(0x15); // NACK Ainda nao tratado no receptor
  }
}


void processarData(uint8_t tamanho) {
  uint8_t dadosByte[tamanho];
  
  for(uint8_t i = 0; i < tamanho; i++){
  	dadosByte[i] = portaSerial.read();
  }
  
  uint8_t checksumRecebido = portaSerial.read();
  
  uint8_t checksumCalculado = 0x04 ^ tamanho;
  
  for(uint8_t i = 0; i < tamanho; i++){
  	checksumCalculado ^= dadosByte[i];
  }
  
  if (checksumCalculado == checksumRecebido) {
      
    Serial.println("[SUCESSO] Data recebida: ");
    
    for(uint8_t i = 0; i < tamanho; i++){
    	Serial.print((char)dadosByte[i]);
    }
    portaSerial.write(0x06); // ACK Ainda nao tratado no receptor
    
  } else {
    Serial.println("[ERRO] Falha no Checksum da Data!");
    portaSerial.write(0x15); // NACK Ainda nao tratado no receptor
  }
}