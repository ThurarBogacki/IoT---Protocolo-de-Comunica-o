#include <SoftwareSerial.h>


SoftwareSerial portaSerial(10, 11);

void setup()
{
  Serial.begin(9600);
  portaSerial.begin(9600);
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
        	break;
        
        case 0x02:
        	processarWord(tamanho);
        	break;
        
        case 0x03:
        	break;
        
        case 0x04:
        	break;
        
        default:
        	Serial.println("[ERRO] Tipo de dado desconhecido");
        	portaSerial.write(0x15); //NACK
        	break;
      }
    }
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
      
    Serial.print("[SUCESSO] Word recebida: ");
    Serial.println(wordRemontada);
    portaSerial.write(0x06); // ACK Ainda nao tratado no receptor
    
  } else {
    Serial.println("[ERRO] Falha no Checksum da Word!");
    portaSerial.write(0x15); // NACK Ainda nao tratado no receptor
  }
}