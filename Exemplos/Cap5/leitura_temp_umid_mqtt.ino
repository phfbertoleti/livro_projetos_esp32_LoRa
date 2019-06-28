/*
* Projeto: medição local e via mqtt de temperatura e umidade
* Autor: Pedro Bertoleti
*/
#include <WiFi.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <PubSubClient.h>

/*
 * Defines do projeto
 */
/* GPIO do módulo WiFi LoRa 32(V2) que o pino de comunicação do sensor está ligado. */
#define DHTPIN    13 /* (GPIO 13) */

/* Endereço I2C do display */
#define OLED_ADDR 0x3c

/* distancia, em pixels, de cada linha em relacao ao topo do display */
#define OLED_LINE1 0
#define OLED_LINE2 10
#define OLED_LINE3 20
#define OLED_LINE4 30
#define OLED_LINE5 40
#define OLED_LINE6 50

/* Configuração da resolucao do display (este modulo possui display 128x64) */
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

/*
A biblioteca serve para os sensores DHT11, DHT22 e DHT21. 
No nosso caso, usaremos o DHT22, porém se você desejar utilizar 
algum dos outros disponíveis, basta descomentar a linha correspondente.
*/
//#define DHTTYPE DHT11   // DHT 11
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
//#define DHTTYPE DHT21   // DHT 21 (AM2301)

/* defines de id mqtt e tópicos para publicação e subscribe */
#define TOPICO_PUBLISH   "MQTTINCBTempUmid"    /*tópico MQTT de envio de informações para Broker
                                                 IMPORTANTE: recomenda-se fortemente alterar os nomes
                                                             desses tópicos. Caso contrário, há grandes
                                                             chances de você controlar e monitorar o módulo
                                                             de outra pessoa (pois o broker utilizado contém 
                                                             dispositivos do mundo todo). Altere-o para algo 
                                                             o mais único possível para você. */

#define ID_MQTT  "INCBTempUmid"     /* id mqtt (para identificação de sessão)
                                       IMPORTANTE: este deve ser único no broker (ou seja, 
                                                   se um client MQTT tentar entrar com o mesmo 
                                                   id de outro já conectado ao broker, o broker 
                                                   irá fechar a conexão de um deles). Pelo fato
                                                   do broker utilizado conter  dispositivos do mundo 
                                                   todo, recomenda-se fortemente que seja alterado 
                                                   para algo o mais único possível para você.*/
                                                   

/* Constantes */
const char* SSID = " "; // coloque aqui o SSID / nome da rede WI-FI que deseja se conectar
const char* PASSWORD = " "; // coloque aqui a senha da rede WI-FI que deseja se conectar

const char* BROKER_MQTT = "iot.eclipse.org"; //URL do broker MQTT que se deseja utilizar
int BROKER_PORT = 1883; // Porta do Broker MQTT
  
/* Variáveis e objetos globais*/
WiFiClient espClient; // Cria o objeto espClient
PubSubClient MQTT(espClient); // Instancia o Cliente MQTT passando o objeto espClient


/*
 * Variáveis e objetos globais
 */
/* objeto para comunicação com sensor DHT22 */
DHT dht(DHTPIN, DHTTYPE);

/* objeto do display */
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, 16);

/* variáveis que armazenam os valores máximo e mínimo de temperatura registrados. */
float temperatura_max;
float temperatura_min;

/* prototypes */
void atualiza_temperatura_max_e_minima(float temp_lida);
void envia_medicoes_para_serial(float temp_lida, float umid_lida);
void escreve_temperatura_umidade_display(float temp_lida, float umid_lida);
void init_wifi(void);
void init_MQTT(void);
void reconnect_wifi(void); 
void reconnect_MQTT(void);
void verifica_conexoes_wifi_e_MQTT(void);
void envia_informacoes_por_mqtt(float temp_lida, float umid_lida);

/*
 * Implementações
 */

/* Função: inicializa e conecta-se na rede WI-FI desejada
 * Parâmetros: nenhum
 *Retorno: nenhum
*/
void init_wifi(void) 
{
    delay(10);
    Serial.println("------Conexao WI-FI------");
    Serial.print("Conectando-se na rede: ");
    Serial.println(SSID);
    Serial.println("Aguarde");    
    reconnect_wifi();
}
 
/* Função: inicializa parâmetros de conexão MQTT(endereço do broker e porta)
 * Parâmetros: nenhum
 * Retorno: nenhum
*/
void init_MQTT(void) 
{
    //informa qual broker e porta deve ser conectado
    MQTT.setServer(BROKER_MQTT, BROKER_PORT);   
}

/* Função: reconecta-se ao broker MQTT (caso ainda não esteja conectado ou em caso de a conexão cair)
 *         em caso de sucesso na conexão ou reconexão, o subscribe dos tópicos é refeito.
 * Parâmetros: nenhum
 * Retorno: nenhum
*/
void reconnect_MQTT(void) 
{
    while (!MQTT.connected()) 
    {
        Serial.print("* Tentando se conectar ao Broker MQTT: ");
        Serial.println(BROKER_MQTT);
        if (MQTT.connect(ID_MQTT)) 
            Serial.println("Conectado com sucesso ao broker MQTT!");
        else 
        {
            Serial.println("Falha ao reconectar no broker.");
            Serial.println("Havera nova tentatica de conexao em 2s");
            delay(2000);
        }
    }
}
 
/* Função: reconecta-se ao WiFi
 * Parâmetros: nenhum
 * Retorno: nenhum
*/
void reconnect_wifi(void) 
{
    /* se já está conectado a rede WI-FI, nada é feito. 
       Caso contrário, são efetuadas tentativas de conexão */
    if (WiFi.status() == WL_CONNECTED)
        return;
        
    WiFi.begin(SSID, PASSWORD); // Conecta na rede WI-FI
    
    while (WiFi.status() != WL_CONNECTED) 
    {
        delay(100);
        Serial.print(".");
    }
  
    Serial.println();
    Serial.print("Conectado com sucesso na rede ");
    Serial.print(SSID);
    Serial.println("IP obtido: ");
    Serial.println(WiFi.localIP());
}
 
/* Função: verifica o estado das conexões WiFI e ao broker MQTT. 
 *         Em caso de desconexão (qualquer uma das duas), a conexão
 *        é refeita.
 * Parâmetros: nenhum
 * Retorno: nenhum
*/
void verifica_conexoes_wifi_e_MQTT(void)
{
    /* se não há conexão com o WiFI, a conexão é refeita */ 
    reconnect_wifi(); 

    /* se não há conexão com o Broker, a conexão é refeita  */ 
    if (!MQTT.connected()) 
        reconnect_MQTT(); 
}

 
/* Função: verifica se os valores de temperatura máxima e mínima devem ser atualizados
 * Parâmetros: temperatura lida
 * Retorno: nenhum
 */  
void atualiza_temperatura_max_e_minima(float temp_lida)
{
  if (temp_lida > temperatura_max)
    temperatura_max = temp_lida;

  if (temp_lida < temperatura_min)
    temperatura_min = temp_lida;  
}
 
/* Função: envia, na forma de mensagens textuais, as medições para a serial
 * Parâmetros: - Temperatura lida
 *             - Umidade relativa do ar lida
 *             - Máxima temperatura registrada
 *             - Mínima temperatura registrada
 * Retorno: nenhum
*/ 
void envia_medicoes_para_serial(float temp_lida, float umid_lida) 
{
  char mensagem[200];
  char i;

  /* pula 80 linhas, de forma que no monitor serial seja exibida somente as mensagens atuais (impressao de refresh de tela) */
  for(i=0; i<80; i++)
      Serial.println(" ");

  /* constrói mensagens e as envia */
  /* - temperatura atual */
  memset(mensagem,0,sizeof(mensagem));
  sprintf(mensagem,"- Temperatura: %.2f C", temp_lida);
  Serial.println(mensagem);
  
  //- umidade relativa do ar atual
  memset(mensagem,0,sizeof(mensagem));
  sprintf(mensagem,"- Umidade atual: %.2f/100",umid_lida);
  Serial.println(mensagem);
  
  //- temperatura maxima
  memset(mensagem,0,sizeof(mensagem));
  sprintf(mensagem,"- Temperatura maxima: %.2f C", temperatura_max);
  Serial.println(mensagem); 
  
  //- temperatura minima
  memset(mensagem,0,sizeof(mensagem));
  sprintf(mensagem,"- Temperatura minima: %.2f C", temperatura_min);
  Serial.println(mensagem);
}

/* Função: escreve no display OLED a temperatura e umidade lidas, assim como as temperaturas máxima e mínima
 * Parâmetros: - Temperatura lida
 *             - Umidade relativa do ar lida
 * Retorno: nenhum
*/ 
void escreve_temperatura_umidade_display(float temp_lida, float umid_lida)
{
    char str_temp[10] = {0};
    char str_umid[10] = {0};
    char str_temp_max_min[20] = {0};

    /* formata para o display as strings de temperatura e umidade */
    sprintf(str_temp, "%.2fC", temp_lida);
    sprintf(str_umid, "%.2f/100", umid_lida);
    sprintf(str_temp_max_min, "%.2fC / %.2fC", temperatura_min, temperatura_max);
    
    display.clearDisplay();
    display.setCursor(0, OLED_LINE1);
    display.println("Temperatura:");
    display.setCursor(0, OLED_LINE2);
    display.println(str_temp);
    display.setCursor(0, OLED_LINE3);
    display.println("Umidade:");
    display.setCursor(0, OLED_LINE4);
    display.print(str_umid);
    display.setCursor(0, OLED_LINE5);
    display.println("Temp. min / max:");
    display.setCursor(0, OLED_LINE6);
    display.print(str_temp_max_min);
    display.display();
}

/* 
 * Função: envia por MQTT as informações de temperatura e umidade lidas, assim como as temperaturas máxima e mínima
 * Parâmetros: - Temperatura lida
 *             - Umidade relativa do ar lida
 * Retorno: nenhum
 */
void envia_informacoes_por_mqtt(float temp_lida, float umid_lida)
{
  char mensagem_MQTT[200] = {0};

  //constrói mensagens e as envia
  //- temperatura atual
  sprintf(mensagem_MQTT,"Temperatura: %.2f C, Umidade atual: %.2f/100, Temperatura maxima: %.2f C, Temperatura minima: %.2f C", temp_lida, 
                                                                                                                              umid_lida,
                                                                                                                              temperatura_max,
                                                                                                                              temperatura_min);
                                                                                                                              
  MQTT.publish(TOPICO_PUBLISH, mensagem_MQTT);  

}

void setup() {
  /* configura comunicação serial (para enviar mensgens com as medições) 
   e inicializa comunicação com o sensor. 
   */
  Serial.begin(115200);  
  dht.begin();

  /* inicializa temperaturas máxima e mínima com a leitura inicial do sensor */
  temperatura_max = dht.readTemperature();
  temperatura_min = temperatura_max;

   /* inicializa display OLED */
   Wire.begin(4, 15);
   
   if(!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR))
       Serial.println("Display OLED: falha ao inicializar");
   else
   {
       Serial.println("Display OLED: inicializacao ok");
  
       /* Limpa display e configura tamanho de fonte */
       display.clearDisplay();
       display.setTextSize(1);
       display.setTextColor(WHITE);
   }

  /* inicializações do WI-FI e MQTT */
  init_wifi();
  init_MQTT();
}

/*
 * Programa principal
 */
void loop() {
  float temperatura_lida;
  float umidade_lida;

  /* Verifica se as conexões MQTT e wi-fi estão ativas 
     Se alguma delas não estiver ativa, a reconexão é feita */
  verifica_conexoes_wifi_e_MQTT();
  
  /* Faz a leitura de temperatura e umidade do sensor */
  temperatura_lida = dht.readTemperature();
  umidade_lida = dht.readHumidity();

  /* se houve falha na leitura do sensor, escreve mensagem de erro na serial */
  if ( isnan(temperatura_lida) || isnan(umidade_lida) ) 
    Serial.println("Erro ao ler sensor DHT22!");
  else
  {
    /*Se a leitura foi bem sucedida, ocorre o seguinte:
       - Os valores mínimos e máximos são verificados e comparados à medição atual de temperatura
         se a temperatura atual for menor que a mínima ou maior que a máxima até então
         registrada, os limites máximo ou mínimo são atualizados.
       - As medições (temperatura, umidade, máxima temperatura e mínima temperatura) são
         enviados pela serial na forma de mensagem textual. Tais mensagens podem ser vistas
         no monitor serial.
       - As medições (temperatura, umidade, máxima temperatura e mínima temperatura) são
         escritas no display OLED
       - As medições (temperatura, umidade, máxima temperatura e mínima temperatura) são
         enviadas por MQTT
     */
    atualiza_temperatura_max_e_minima(temperatura_lida);
    envia_medicoes_para_serial(temperatura_lida, umidade_lida);
    escreve_temperatura_umidade_display(temperatura_lida, umidade_lida);
    envia_informacoes_por_mqtt(temperatura_lida, umidade_lida);
  }  

  /* Faz o keep-alive do MQTT */
  MQTT.loop();
  
  /* espera cinco segundos até a próxima leitura  */
  delay(5000);
}

