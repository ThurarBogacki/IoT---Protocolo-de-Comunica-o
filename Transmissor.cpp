#include <SoftwareSerial.h>

SoftwareSerial portaSerial(10,11);

void setup()
{
  Serial.begin(9600);
  portaSerial.begin(9600);
}

void loop()
{
  uint16_t meuNumero = 1024;
  
  Serial.println("Tentando enviar uma Word...");
  
  bool sucesso = sendWord(meuNumero);
  
  if (sucesso) {
    Serial.println("Transmissao concluída com sucesso!\n");
  } else {
    Serial.println("Falha na transmissao!\n");
  }
  
  delay(10000);
  
}


bool sendWord(uint16_t value){
	uint8_t tipo = 0x02;
  	uint8_t tamanho = 2;

    uint8_t byteAlto = (value >> 8) & 0xFF;
    uint8_t byteBaixo = value & 0xFF;
  
  	uint8_t checksum = tipo ^ tamanho ^ byteAlto ^ byteBaixo;
  	
  	portaSerial.write(0xAA); // Sinaliza o inicio
  	portaSerial.write(tipo);
  	portaSerial.write(tamanho);
  	portaSerial.write(byteAlto);
  	portaSerial.write(byteBaixo);
  	portaSerial.write(checksum);
  
  	return true;
}