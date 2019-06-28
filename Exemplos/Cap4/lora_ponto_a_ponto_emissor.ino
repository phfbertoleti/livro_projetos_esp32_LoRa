/*
* Projeto: emissor - comunicação LoRa ponto-a-ponto
* Autor: Pedro Bertoleti
*/
#include <LoRa.h>
#include <SPI.h>
#include <Wire.h>

/* Definicoes para comunicação com radio LoRa */
#define SCK_LORA           5
#define MISO_LORA          19
#define MOSI_LORA          27
#define RESET_PIN_LORA     14
#define SS_PIN_LORA        18

#define HIGH_GAIN_LORA     20  /* dBm */
#define BAND               915E6  /* 915MHz de frequencia */

/* Definicoes gerais */
#define DEBUG_SERIAL_BAUDRATE    115200

/* Variaveis globais */
long informacao_a_ser_enviada = 0;

/* Local prototypes */
bool init_comunicacao_lora(void);

/* Funcao: inicia comunicação com chip LoRa
 * Parametros: nenhum
 * Retorno: true: comunicacao ok
 *          false: falha na comunicacao
*/
bool init_comunicacao_lora(void)
{
    bool status_init = false;
    Serial.println("[LoRa Sender] Tentando iniciar comunicacao com o radio LoRa...");
    SPI.begin(SCK_LORA, MISO_LORA, MOSI_LORA, SS_PIN_LORA);
    LoRa.setPins(SS_PIN_LORA, RESET_PIN_LORA, LORA_DEFAULT_DIO0_PIN);
    
    if (!LoRa.begin(BAND)) 
    {
        Serial.println("[LoRa Sender] Comunicacao com o radio LoRa falhou. Nova tentativa em 1 segundo...");        
        delay(1000);
        status_init = false;
    }
    else
    {
        /* Configura o ganho do receptor LoRa para 20dBm, o maior ganho possível (visando maior alcance possível) */ 
        LoRa.setTxPower(HIGH_GAIN_LORA); 
        Serial.println("[LoRa Sender] Comunicacao com o radio LoRa ok");
        status_init = true;
    }

    return status_init;
}

/* Funcao de setup */
void setup() 
{    
    Serial.begin(DEBUG_SERIAL_BAUDRATE);
    while (!Serial);

    /* Tenta, até obter sucesso, comunicacao com o chip LoRa */
    while(init_comunicacao_lora() == false);       
}

/* Programa principal */
void loop() 
{
    /* Envia a informação */
    LoRa.beginPacket();
    LoRa.write((unsigned char *)&informacao_a_ser_enviada, sizeof(informacao_a_ser_enviada));
    LoRa.endPacket();
   
    /* Incrementa a informação para o próximo envio e aguarda 
       1 segundo até enviar a próxima informação */
    informacao_a_ser_enviada++;
    delay(1000);
    
}
