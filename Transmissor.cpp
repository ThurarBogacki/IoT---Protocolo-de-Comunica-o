#include <SoftwareSerial.h>

#define MAX_BUFFER_SIZE 64
SoftwareSerial portaSerial(10,11);

void setup()
{
  Serial.begin(4800);
  portaSerial.begin(4800);
  
  while (!Serial);
  exibirMenu();
}


void loop()
{
  if (Serial.available() > 0) {
    char opcao = Serial.read();
    
    while (Serial.available() > 0) {
      Serial.read();
    }
    
	uint8_t errorMask = 0x00;
    
    switch (opcao) {
      case '1':
        Serial.println("\n[TEST] Sending BYTE: 150");
      	errorMask = promptErrorInjection();
        sendByte(150, errorMask);
        break;

      case '2':
        Serial.println("\n[TEST] Sending WORD: 1024");
        errorMask = promptErrorInjection();
        sendWord(1024, errorMask);
        break;

      case '3':
        Serial.println("\n[TEST] Sending FLOAT: 3.1415");
      	errorMask = promptErrorInjection();
        sendFloat(3.1415, errorMask);
        break;

        case '4':
        {
          Serial.println(F("Informe o tamanho da mensagem (ex: 3):"));
          while (Serial.available() == 0) { }
          uint16_t sizeInput = Serial.parseInt(); 
          
          while (Serial.available() > 0) { Serial.read(); }

          if (sizeInput <= 0 || sizeInput > MAX_BUFFER_SIZE) {
            Serial.println(F("Tamanho invalido!"));
            break;
          }

          Serial.println(F("Informe a mensagem e aperte Enviar:"));
          
          while (Serial.available() < sizeInput) {
       
          }

          uint8_t mensagem[MAX_BUFFER_SIZE];
          
          for(uint16_t i = 0; i < sizeInput; i++) {
              mensagem[i] = Serial.read();
          }

          delay(20);
          while (Serial.available() > 0) {
            Serial.read();
          }

          Serial.println(F("\n[TEST] Sending DATA:"));
          errorMask = promptErrorInjection(); 
          sendData(mensagem, sizeInput, errorMask);
        }
        break;

      default:
        Serial.println("\n[OPÇÃO INVÁLIDA] Escolha de 1 a 4.");
        break;
    }

    exibirMenu();
  }
  
}

bool sendByte(uint8_t value, uint8_t maskError){
	uint8_t tipo = 0x01;
  	uint8_t tamanho = 1;
  	uint8_t checksum = tipo ^ tamanho ^ value ^ maskError;
  	
  	for(uint8_t tentativa = 0; tentativa < 3; tentativa++){
  		Serial.print("[TX] Tentativa ");
        Serial.print(tentativa);
        Serial.println(" de 3...");
    
      	portaSerial.write(0xAA); 
        portaSerial.write(tipo);
        portaSerial.write(tamanho);
        portaSerial.write(value);
        portaSerial.write(checksum);
  		
    	unsigned long inicioTempo = millis();
    	bool respostaRecebida = false;
      	uint8_t resposta = 0;
      
      while(millis() - inicioTempo < 500) {
        if(portaSerial.available() > 0){
        	resposta = portaSerial.read();
          	respostaRecebida = true;
          	break;
        }
      }
      
      if(respostaRecebida){
        if(resposta == 0x06){
        	Serial.println("[TX] SUCESSO: ACK recebido!");
            return true;
        }else if(resposta == 0x15){
        	Serial.println("[TX] AVISO: NACK recebido. Tentando reenviar...");
        }else{
        	Serial.print("[TX] AVISO: Resposta desconhecida (0x");
            Serial.print(resposta, HEX);
            Serial.println("). Reenviando...");
        }
        
      }else {
            Serial.println("[TX] AVISO: Timeout! Nenhuma resposta do receptor. Reenviando...");
      }
  		delay(200);
  	}
	Serial.println("[TX ERRO] Falha definitiva: Limite de 3 tentativas esgotado!");
  	return false;
}


bool sendWord(uint16_t value, uint8_t maskError){
	uint8_t tipo = 0x02;
  	uint8_t tamanho = 2;

    uint8_t byteAlto = (value >> 8) & 0xFF;
    uint8_t byteBaixo = value & 0xFF;
  
  	uint8_t checksum = tipo ^ tamanho ^ byteAlto ^ byteBaixo ^ maskError;
  
    for(uint8_t tentativa = 0; tentativa < 3; tentativa++){
      portaSerial.write(0xAA); 
      portaSerial.write(tipo);
      portaSerial.write(tamanho);
      portaSerial.write(byteAlto);
      portaSerial.write(byteBaixo);
      portaSerial.write(checksum);
		
	  unsigned long inicioTempo = millis();
      bool respostaRecebida = false;
      uint8_t resposta = 0;
      
      while(millis() - inicioTempo < 500){
        if(portaSerial.available() > 0){
        	respostaRecebida = true;
          	resposta = portaSerial.read();
          	break;
        }
      }
      
      if(respostaRecebida){
        if(resposta == 0x06){
        	Serial.println("[TX] SUCESSO: ACK recebido!");
          	return true;
        }else if(resposta == 0x15){
            Serial.println("[TX] AVISO: NACK recebido. Tentando reenviar...");
        }else{
            Serial.print("[TX] AVISO: Resposta desconhecida (0x");
            Serial.print(resposta, HEX);
            Serial.println("). Reenviando...");
        }
      }else{
      	Serial.println("[TX] AVISO: Timeout! Nenhuma resposta do receptor. Reenviando...");
      }
      delay(200);
    }
  
	Serial.println("[TX ERRO] Falha definitiva: Limite de 3 tentativas esgotado!");
  	return false;
}

bool sendFloat(float value, uint8_t maskError){
	uint8_t tipo = 0x03;
  	uint8_t tamanho = 4;

    uint8_t *dadosByte = (uint8_t*)&value;
  
  	uint8_t checksum = tipo ^ tamanho ^ maskError;
  	for(uint8_t i = 0; i < 4; i++){
  		checksum ^= dadosByte[i];
    }
  	
  for(uint8_t tentativa = 0; tentativa < 3; tentativa++){
    Serial.print("[TX] Tentativa ");
    Serial.print(tentativa);
    Serial.println(" de 3...");
    
  	portaSerial.write(0xAA);
  	portaSerial.write(tipo);
  	portaSerial.write(tamanho);

  	for(uint8_t i = 0; i < 4; i++) {
    	portaSerial.write(dadosByte[i]);
  	}
  	portaSerial.write(checksum);
 	
    bool respostaRecebida = false;
    uint8_t resposta = 0;
    unsigned long inicioTempo = millis();
    
    while(millis() - inicioTempo < 500){
      if(portaSerial.available() > 0){
      	resposta = portaSerial.read();
        respostaRecebida = true;
     	break;
      }
    }
    
    if(respostaRecebida){
      if(resposta == 0x06){
      	Serial.println("[TX] SUCESSO: ACK recebido!");
        return true;
      }else if(resposta == 0x15){
        Serial.println("[TX] AVISO: NACK recebido. Tentando reenviar...");
      }else{
         Serial.print("[TX] AVISO: Resposta desconhecida (0x");
         Serial.print(resposta, HEX);
         Serial.println("). Reenviando...");     	
      }
    }else{
    	Serial.println("[TX] AVISO: Timeout! Nenhuma resposta do receptor. Reenviando...");
    }
    delay(200);
  }
                       
	Serial.println("[TX ERRO] Falha definitiva: Limite de 3 tentativas esgotado!");
  	return false;
}


bool sendData(uint8_t *data, uint16_t size, uint8_t maskError){
    uint8_t tipo = 0x04;
  	
  	uint8_t sizeAlto = (size >> 8) & 0xFF;
    uint8_t sizeBaixo = size & 0xFF;
    uint8_t checksum = tipo ^ sizeAlto ^ sizeBaixo ^ maskError;
  
    for(uint8_t i = 0; i < size; i++){
        checksum ^= data[i];
    }
    
    for(uint8_t tentativa = 1; tentativa <= 3; tentativa++){
      Serial.print("[TX] Tentativa ");
      Serial.print(tentativa);
      Serial.println(" de 3...");
      
      portaSerial.write(0xAA);
      portaSerial.write(tipo);
      portaSerial.write(sizeAlto);
      portaSerial.write(sizeBaixo);

        for (uint8_t i = 0; i < size; i++) {
            portaSerial.write(data[i]);
        }
        portaSerial.write(checksum);
    
        bool respostaRecebida = false;
        uint8_t resposta = 0;
        unsigned long inicioTempo = millis();
        
        while(millis() - inicioTempo < 500){
            if(portaSerial.available() > 0){
                resposta = portaSerial.read();
                respostaRecebida = true;
                break;
            }
        }
        
        if(respostaRecebida){
            if(resposta == 0x06){
                Serial.println("[TX] SUCESSO: ACK recebido!");
                return true;
            }else if(resposta == 0x15){
                Serial.println("[TX] AVISO: NACK recebido. Tentando reenviar...");
            }else{
                Serial.print("[TX] AVISO: Resposta desconhecida (0x");
                Serial.print(resposta, HEX);
                Serial.println("). Reenviando...");     
            }
        }else{
            Serial.println("[TX] AVISO: Timeout! Nenhuma resposta do receptor. Reenviando...");
        }
        delay(200);
    }
                    
    Serial.println("[TX ERRO] Falha definitiva: Limite de 3 tentativas esgotado!");
    return false;
}


void exibirMenu() {
  Serial.println("\n=================================");
  Serial.println("   MENU DE TESTES - TRANSMISSOR  ");
  Serial.println("=================================");
  Serial.println("Escolha o tipo de dado para enviar:");
  Serial.println(" [1] Enviar BYTE   (Ex: 150)");
  Serial.println(" [2] Enviar WORD   (Ex: 1024)");
  Serial.println(" [3] Enviar FLOAT  (Ex: 3.1415)");
  Serial.println(" [4] Enviar DATA   (Ex: 'IoT')");
  Serial.println("Digite a sua opcao:");
  Serial.println("---------------------------------");
}


uint8_t promptErrorInjection() {
    Serial.println("\n--- SIMULACAO DE ERRO ---");
    Serial.println("Deseja injetar erro (corromper checksum)? (S/N): ");
    
    while (true) {
        if (Serial.available() > 0) {
            char resposta = Serial.read();
            while (Serial.available() > 0) {
                Serial.read();
            }
            
            if (resposta == 'S' || resposta == 's') {
                Serial.println("[INFO] Erro ativado! O checksum será corrompido (XOR 0xFF).");
                return 0xFF;
            } else if (resposta == 'N' || resposta == 'n' || resposta == '\r' || resposta == '\n') {
                Serial.println("[INFO] Envio normal sem erro (XOR 0x00).");
                return 0x00;
            } else {
                Serial.println("Opção inválida. Digite S para Sim ou N para Não:");
            }
        }
    }
}
