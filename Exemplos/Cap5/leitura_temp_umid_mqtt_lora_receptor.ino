/*
* Projeto: receptor - medição local, via LoRa e via mqtt de temperatura e umidade
* Autor: Pedro Bertoleti
*/
#include <LoRa.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

/* Definicoes para comunicação com radio LoRa */
#define SCK_LORA           5
#define MISO_LORA          19
#define MOSI_LORA          27
#define RESET_PIN_LORA     14
#define SS_PIN_LORA        18

#define HIGH_GAIN_LORA     20  /* dBm */
#define BAND               915E6  /* 915MHz de frequencia */

/* Definicoes do OLED */
#define OLED_SDA_PIN    4
#define OLED_SCL_PIN    15
#define SCREEN_WIDTH    128 
#define SCREEN_HEIGHT   64  
#define OLED_ADDR       0x3C 
#define OLED_RESET      16

/* Offset de linhas no display OLED */
#define OLED_LINE1     0
#define OLED_LINE2     10
#define OLED_LINE3     20
#define OLED_LINE4     30
#define OLED_LINE5     40
#define OLED_LINE6     50

/* Definicoes gerais */
#define DEBUG_SERIAL_BAUDRATE    115200

/* Variaveis e objetos globais */
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

/* Local prototypes */
void display_init(void);
bool init_comunicacao_lora(void);

/* typedefs */
typedef struct __attribute__((__packed__))  
{
  float temperatura;
  float umidade;
  float temperatura_min;
  float temperatura_max;
}TDadosLora;

/* Funcao: inicializa comunicacao com o display OLED
 * Parametros: nenhnum
 * Retorno: nenhnum
*/ 
void display_init(void)
{
    if(!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) 
    {
        Serial.println("[LoRa Receiver] Falha ao inicializar comunicacao com OLED");        
    }
    else
    {
        Serial.println("[LoRa Receiver] Comunicacao com OLED inicializada com sucesso");
    
        /* Limpa display e configura tamanho de fonte */
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(WHITE);
    }
}

/* Funcao: inicia comunicação com chip LoRa
 * Parametros: nenhum
 * Retorno: true: comunicacao ok
 *          false: falha na comunicacao
*/
bool init_comunicacao_lora(void)
{
    bool status_init = false;
    Serial.println("[LoRa Receiver] Tentando iniciar comunicacao com o radio LoRa...");
    SPI.begin(SCK_LORA, MISO_LORA, MOSI_LORA, SS_PIN_LORA);
    LoRa.setPins(SS_PIN_LORA, RESET_PIN_LORA, LORA_DEFAULT_DIO0_PIN);
    
    if (!LoRa.begin(BAND)) 
    {
        Serial.println("[LoRa Receiver] Comunicacao com o radio LoRa falhou. Nova tentativa em 1 segundo...");        
        delay(1000);
        status_init = false;
    }
    else
    {
        /* Configura o ganho do receptor LoRa para 20dBm, o maior ganho possível (visando maior alcance possível) */ 
        LoRa.setTxPower(HIGH_GAIN_LORA); 
        Serial.println("[LoRa Receiver] Comunicacao com o radio LoRa ok");
        status_init = true;
    }

    return status_init;
}

/* Funcao de setup */
void setup() 
{
    /* Configuracao da I²C para o display OLED */
    Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);

    /* Display init */
    display_init();

    /* Print message telling to wait */
    display.clearDisplay();    
    display.setCursor(0, OLED_LINE1);
    display.print("Aguarde...");
    display.display();
    
    Serial.begin(DEBUG_SERIAL_BAUDRATE);
    while (!Serial);

    /* Tenta, até obter sucesso, comunicacao com o chip LoRa */
    while(init_comunicacao_lora() == false);       
}

/* Programa principal */
void loop() 
{
    char byte_recebido;
    int packet_size = 0;
    int lora_rssi = 0;
    TDadosLora dados_lora;
    char * ptInformaraoRecebida = NULL;
  
    /* Verifica se chegou alguma informação do tamanho esperado */
    packet_size = LoRa.parsePacket();
    
    if (packet_size == sizeof(TDadosLora)) 
    {
        Serial.print("[LoRa Receiver] Há dados a serem lidos");
        
        /* Recebe os dados conforme protocolo */               
        ptInformaraoRecebida = (char *)&dados_lora;  
        while (LoRa.available()) 
        {
            byte_recebido = (char)LoRa.read();
            *ptInformaraoRecebida = byte_recebido;
            ptInformaraoRecebida++;
        }

        /* Escreve RSSI de recepção e informação recebida */
        lora_rssi = LoRa.packetRssi();
        display.clearDisplay();   
        display.setCursor(0, OLED_LINE1);
        display.print("Temp: ");
        display.println(dados_lora.temperatura);

        display.setCursor(0, OLED_LINE2);
        display.print("Umid: ");
        display.println(dados_lora.umidade);

        display.setCursor(0, OLED_LINE3);
        display.println("Temp min/max:");
        display.setCursor(0, OLED_LINE4);
        display.print(dados_lora.temperatura_min);
        display.print("/");
        display.print(dados_lora.temperatura_max);

        display.display();      
    }
}
