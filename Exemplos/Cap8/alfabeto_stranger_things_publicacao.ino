/*
* Projeto: alfabeto stranger things (com comunicação via Internet / MQTT)
* Autor: Pedro Bertoleti
*/
#include <FastLED.h>
#include <WiFi.h> 
#include <PubSubClient.h>

/* Defines gerais*/
#define BAUDRATE_SERIAL_DEBUG   115200
#define SIM                     1
#define NAO                     0
#define OFFSET_CONVERSAO_CHAR   0x41
#define TEMPO_ACENDIMENTO_LED   500 //ms

/* Defines da Fita LED */
#define NUM_LETRAS_ALFABETO     26  /* A, B, C, ... Z */
#define NUM_LEDS                NUM_LETRAS_ALFABETO     /* Número de LEDs do projeto */
#define DATA_PIN                13     /* GPIO do módulo que será ligado ao Data Pin */
#define LED_TYPE                WS2811 /* Tipo de fita led do projeto */
#define ORDEM_DAS_CORES         GRB
#define COR_LEDS                CRGB::Blue  /* Cor que os LEDs devem ter ao acender */
#define LED_APAGADO             CRGB::Black

/* Defines de id mqtt e tópicos para publicação e subscribe */
#define TOPICO_SUB "MQTTStrangerThingsMsg"   /* tópico MQTT de escuta */
#define ID_MQTT  "StrangerThingsINCB"        /* id mqtt (para identificação de sessão) */
                                             /* IMPORTANTE: este deve ser único no broker (ou seja, 
                                                            se um client MQTT tentar entrar com o mesmo 
                                                            id de outro já conectado ao broker, o broker 
                                                            irá fechar a conexão de um deles). 
                                             */

/* Variáveis globais */
/* Variáveis gerais */
CRGB leds_projeto[NUM_LEDS];
int ha_mensagem_a_ser_escrita = NAO;

/* Mensagem a ser escrita */
String msg_a_ser_escrita = "";

/* WIFI */
const char* SSID_WIFI = " ";     /* Coloque aqui o SSID_WIFI / nome da rede WI-FI que deseja se conectar */
const char* PASSWORD_WIFI = " "; /* Coloque aqui a senha da rede WI-FI que deseja se conectar */
 
/* MQTT */
const char* BROKER_MQTT = "broker.hivemq.com"; /* URL do broker MQTT que se deseja utilizar */
int BROKER_PORT = 1883;                      /* Porta do Broker MQTT */

WiFiClient espClient;              /* cria o objeto espClient */
PubSubClient MQTT(espClient);      /* objeto PubSubClient */

/* Prototypes */
void init_serial_debug(void);
void init_wifi(void);
void init_MQTT(void);
void connect_wifi(void); 
void init_outputs(void);
void mqtt_callback(char* topic, byte* payload, unsigned int length);
void verifica_conexoes_wifi_e_MQTT(void);
void escreve_mensagem_alfabeto(void);

/*
 * Implementações
 */

/* Função: inicializa comunicação serial com baudrate 115200 (para fins de monitorar no terminal serial 
           o que está acontecendo.
   Parâmetros: nenhum
   Retorno: nenhum
*/
void init_serial_debug(void) 
{
    Serial.begin(BAUDRATE_SERIAL_DEBUG);
}

/* Função: inicializa e conecta-se na rede WI-FI desejada
   Parâmetros: nenhum
   Retorno: nenhum
*/
void init_wifi(void) 
{
    delay(10);
    Serial.println("------Conexao WI-FI -----");
    Serial.print("Conectando-se na rede: ");
    Serial.println(SSID_WIFI);
    Serial.println("Aguarde");
    
    connect_wifi();
}
 
/* Função: inicializa parâmetros de conexão MQTT(endereço do 
           broker, porta e seta função de callback)
   Parâmetros: nenhum
   Retorno: nenhum
*/
void init_MQTT(void) 
{
    MQTT.setServer(BROKER_MQTT, BROKER_PORT);   //informa qual broker e porta deve ser conectado
    MQTT.setCallback(mqtt_callback);            //atribui função de callback (função chamada quando qualquer informação de um dos tópicos subescritos chega)
}
 
/* Função: função de callback 
           esta função é chamada toda vez que uma informação de 
           um dos tópicos subescritos chega)
   Parâmetros: nenhum
   Retorno: nenhum
*/
void mqtt_callback(char* topic, byte* payload, unsigned int length) 
{
    String msg_recebida;
    int i;

    /* Se o tamanho estiver dentro dos limites permitidos, obtem a string do payload recebido */
    for(i = 0; i < length; i++) 
    {
       char c = (char)payload[i];
       msg_recebida += c;
    }

    Serial.println("Mensagem recebida: ");
    Serial.print(msg_recebida);
  
    /* Faz o upper case da string */
    msg_recebida.toUpperCase();
    msg_a_ser_escrita = msg_recebida;
    ha_mensagem_a_ser_escrita = SIM;
}
 
/* Função: reconecta-se ao broker MQTT (caso ainda não esteja conectado ou em caso de a conexão cair)
           em caso de sucesso na conexão ou reconexão, o subscribe dos tópicos é refeito.
   Parâmetros: nenhum
   Retorno: nenhum
*/
void reconnectMQTT() 
{
    while (!MQTT.connected()) 
    {
        Serial.print("* Tentando se conectar ao Broker MQTT: ");
        Serial.println(BROKER_MQTT);
        if (MQTT.connect(ID_MQTT)) 
        {
            Serial.println("Conectado com sucesso ao broker MQTT!");
            MQTT.subscribe(TOPICO_SUB); 
        } 
        else 
        {
            Serial.println("Falha ao reconectar no broker.");
            Serial.println("Havera nova tentativa de conexao em 2s");
            delay(2000);
        }
    }
}
 
/* Função: reconecta-se ao WiFi
   Parâmetros: nenhum
   Retorno: nenhum
*/
void connect_wifi(void) 
{
    /* se já está conectado a rede WI-FI, nada é feito. 
       Caso contrário, são efetuadas tentativas de conexão 
    */
    if (WiFi.status() == WL_CONNECTED)
        return;
        
    WiFi.begin(SSID_WIFI, PASSWORD_WIFI); // Conecta na rede WI-FI
    
    while (WiFi.status() != WL_CONNECTED) 
    {
        delay(100);
        Serial.print(".");
    }
  
    Serial.println();
    Serial.print("Conectado com sucesso na rede ");
    Serial.print(SSID_WIFI);
    Serial.println("IP obtido: ");
    Serial.println(WiFi.localIP());
}

/* Função: verifica o estado das conexões WiFI e ao broker MQTT. 
           Em caso de desconexão (qualquer uma das duas), a conexão
           é refeita.
  Parâmetros: nenhum
  Retorno: nenhum
*/
void verifica_conexoes_wifi_e_MQTT(void)
{
    connect_wifi(); //se não há conexão com o WiFI, a conexão é refeita
    
    if (!MQTT.connected()) 
        reconnectMQTT(); //se não há conexão com o Broker, a conexão é refeita
} 

/* Função: escreve mensagem recebida no alfabeto stranger things
  Parâmetros: nenhum
  Retorno: nenhum
*/
void escreve_mensagem_alfabeto(void)
{
    int i;
    int led_para_acender=0;
    char caracter_msg=0x00;
    
    for(i=0; i < msg_a_ser_escrita.length(); i++)
    
        /* Obtem qual led da fita deve acender
           Ex: A: led 0; B: led 1; ... ; Z: led 25 */
        caracter_msg = msg_a_ser_escrita.charAt(i);

        /* Qualquer caracter fora do range A..Z é ignorado */ 
        if ( (caracter_msg < 'A') && (caracter_msg > 'Z') )
           continue;
           
        led_para_acender = int(caracter_msg) - OFFSET_CONVERSAO_CHAR;
        
        leds_projeto[led_para_acender] = COR_LEDS; 
        FastLED.show();
        delay(TEMPO_ACENDIMENTO_LED);
      
        leds_projeto[led_para_acender] = LED_APAGADO;
        FastLED.show();
        delay(TEMPO_ACENDIMENTO_LED);
    
}


/* Função de setup */
void setup() 
{
    /* Inicializa serial de debug / serial monitor */
    init_serial_debug();

    /* Inicializa wifi e MQTT */
    init_wifi();
    init_MQTT();
    
    /* Inicializa biblioteca de LEDs para operar com fita led do projeto (WS2811) */
    FastLED.addLeds<LED_TYPE, DATA_PIN, ORDEM_DAS_CORES>(leds_projeto, NUM_LEDS).setCorrection(TypicalLEDStrip);
}

void loop() 
{
    int i;

    /* Verifica se o wi-fi e MQTT estão conectados */
    verifica_conexoes_wifi_e_MQTT();

    /* Verifica se há mensagem a ser excrita. 
       Se sim, faz a escrita no alfabeto Stranger Things */
    if (ha_mensagem_a_ser_escrita == SIM)
    {
        escreve_mensagem_alfabeto();
        ha_mensagem_a_ser_escrita = NAO;   
        msg_a_ser_escrita = "";
    }

    /* Faz o keep-alive do MQTT */
    MQTT.loop();
}



