# 1. Objetivo

Desenvolver um protocolo de comunicação entre dois dispositivos Arduino, utilizando uma comunicação por porta serial (verificar a biblioteca SoftwareSerial.h).

O protocolo deverá permitir a transmissão de diferentes tipos de dados, bem como identificar falhas na comunicação e garantir que as mensagens sejam corretamente recebidas.

O trabalho tem como objetivo aplicar conceitos relacionados ao desenvolvimento de protocolos de comunicação, transmissão de dados, controle de erros e confiabilidade.


# 2. Descrição do trabalho

O sistema será composto por dois Arduinos que deverão se comunicar por meio de uma interface serial.

Os alunos deverão formar grupos (max 4 pessoas), projetar e implementar um protocolo próprio de comunicação, definindo a estrutura das mensagens e os mecanismos necessários para atender aos requisitos apresentados neste trabalho.

O protocolo deverá permitir a transmissão dos seguintes tipos de dados: byte; word; float; dados de tamanho variável, cujo tamanho seja definido pelo usuário. Obedecendo os seguintes protótipos:

bool sendByte(uint8_t value);
bool sendWord(uint16_t value);
bool sendFloat(float value);
bool sendData(const uint8_t *data, uint16_t size);


# 3. Confiabilidade da comunicação

O protocolo deverá ser capaz de detectar falhas na transmissão ou recepção das mensagens. Quando uma mensagem não for recebida corretamente, o sistema deverá ser capaz de solicitar ou realizar uma nova transmissão da mensagem. O protocolo deverá também tratar situações em que uma resposta do dispositivo receptor não seja recebida.

A estratégia utilizada para garantir a confiabilidade da comunicação deverá ser definida pelos alunos e apresentada na documentação do trabalho.


# 4. Funcionamento

O sistema deverá permitir que o usuário de um dos Arduinos envie mensagens para o outro dispositivo. O Arduino receptor deverá identificar e apresentar corretamente os dados recebidos.

Durante a execução, o sistema deverá informar ao usuário o resultado das transmissões, incluindo situações de sucesso e de falha.

Os alunos deverão também criar situações que permitam demonstrar o comportamento do protocolo diante de uma falha de comunicação.


# 5. Requisitos

O protocolo desenvolvido deverá obrigatoriamente:

    permitir a comunicação entre dois Arduinos por porta serial;
    transmitir dados do tipo byte;
    transmitir dados do tipo word;
    transmitir dados do tipo float;
    transmitir dados de tamanho definido pelo usuário;
    permitir que o receptor identifique corretamente os dados recebidos;
    detectar falhas de comunicação;
    realizar a retransmissão de mensagens quando necessário;
    informar ao usuário o resultado da comunicação.


A definição da estrutura das mensagens e dos mecanismos utilizados pelo protocolo ficará a cargo de cada grupo.


# 6. Documentação

O grupo deverá apresentar uma documentação descrevendo:

    o protocolo desenvolvido;
    a estrutura das mensagens;
    as decisões de projeto;
    o funcionamento da comunicação;
    o mecanismo utilizado para garantir a confiabilidade da transmissão;
    os testes realizados;
    os resultados obtidos.

Deve-se anexar, nesta tarefa, os códigos e documentação gerados. Os códigos serão apresentados e testados na aula do dia 16 de Setembro. 