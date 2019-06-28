/* Programa: código-fonte do robô tipo rover (controle e acionamento de motores 
             com relés
   Descrição: código-fonte do robô tipo rover, controlado por Internet via MQTT. 
              Sua movimentação é feita conforme a recepção de caracteres via MQTT:
              F: rover se move para frente
              D: rover se move para a direita
              E: rover se move para a esquerda
              P: o rover para (os motores são desacionados)
   Autor: Pedro Bertoleti
*/

/* includes */ 
#include <WiFi.h> 
#include <PubSubClient.h>

/* defines gerais */
#define BAUDRATE_SERIAL_DEBUG       115200

/* defines de id mqtt e tópicos para publicação e subscribe */
#define TOPICO_SUB "MQTTRoverEnvia"          /* tópico MQTT de escuta */
#define TOPICO_PUB "MQTTRoverRecebe"         /* tópico MQTT de envio de informações para Broker */
#define ID_MQTT  "RoboRoverINCB"             /* id mqtt (para identificação de sessão) */
                                             /* IMPORTANTE: este deve ser único no broker (ou seja, 
                                                            se um client MQTT tentar entrar com o mesmo 
                                                            id de outro já conectado ao broker, o broker 
                                                            irá fechar a conexão de um deles). 
                                             */

/* defines - motores */
#define MOTOR_DIRETO     12 /* GPIO 12 */
#define MOTOR_ESQUERDO   13 /* GPIO 13 */

/* defines - movimentacao */
#define CARACTER_MOVE_FRENTE    "F"
#define CARACTER_MOVE_DIREITA   "D"
#define CARACTER_MOVE_ESQUERDA  "E"
#define CARACTER_PARA_ROVER     "P"


/* Variáveis e objetos globais: */
/* WIFI */
const char* SSID_WIFI = " ";     /* Coloque aqui o SSID_WIFI / nome da rede WI-FI que deseja se conectar */
const char* PASSWORD_WIFI = " "; /* Coloque aqui a senha da rede WI-FI que deseja se conectar */
 
/* MQTT */
const char* BROKER_MQTT = "iot.eclipse.org"; /* URL do broker MQTT que se deseja utilizar */
int BROKER_PORT = 1883;                      /* Porta do Broker MQTT */

/* Variáveis e objetos globais */
WiFiClient espClient;              /* cria o objeto espClient */
PubSubClient MQTT(espClient);      /* objeto PubSubClient */
char estado_motor_direito = '0';   /* estado atual do motor da direita */
char estado_motor_esquerdo = '0';  /* estado atual do motor da esquerda */
 
/* Prototypes */
void init_serial_debug(void);
void init_wifi(void);
void init_MQTT(void);
void connect_wifi(void); 
void init_outputs(void);
void mqtt_callback(char* topic, byte* payload, unsigned int length);
void verifica_conexoes_wifi_e_MQTT(void);
void envia_estado_output_MQTT(void);

/* 
 *  Implementações das funções
 */
void setup() 
{
    //Faz as inicializações necessárias
    init_outputs();
    init_serial_debug();
    init_wifi();
    init_MQTT();
}
 
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
    String msg;
    int i;

    /* obtem a string do payload recebido */
    for(i = 0; i < length; i++) 
    {
       char c = (char)payload[i];
       msg += c;
    }
  
    /* toma ação dependendo da string recebida:  */
    if (msg.equals(CARACTER_MOVE_FRENTE))
    {
        //para ir para frente, os dois motores são ligados
        digitalWrite(MOTOR_DIRETO, HIGH);
        digitalWrite(MOTOR_ESQUERDO, HIGH);
 
        estado_motor_direito = '1';
        estado_motor_esquerdo = '1';
    }

    if (msg.equals(CARACTER_MOVE_DIREITA))
    {
        /* para ir para a direita, somente o motor da esquerda é ligado */
        digitalWrite(MOTOR_DIRETO, LOW);
        digitalWrite(MOTOR_ESQUERDO, HIGH);
 
        estado_motor_direito = '0';
        estado_motor_esquerdo = '1';
    }

    if (msg.equals(CARACTER_MOVE_ESQUERDA))
    {
        /* para ir para a esquerda, somente o motor da direita é ligado */
        digitalWrite(MOTOR_DIRETO, HIGH);
        digitalWrite(MOTOR_ESQUERDO, LOW);
 
        estado_motor_direito = '1';
        estado_motor_esquerdo = '0';
    }

    if (msg.equals(CARACTER_PARA_ROVER))
    {
        /* para parar, os dois motores são desligados */
        digitalWrite(MOTOR_DIRETO, LOW);
        digitalWrite(MOTOR_ESQUERDO, LOW);
 
        estado_motor_direito = '0';
        estado_motor_esquerdo = '0';
    }
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
            Serial.println("Havera nova tentatica de conexao em 2s");
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
       Caso contrário, são efetuadas tentativas de conexão */
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
    if (!MQTT.connected()) 
        reconnectMQTT(); //se não há conexão com o Broker, a conexão é refeita
    
     connect_wifi(); //se não há conexão com o WiFI, a conexão é refeita
}

/* Função: envia ao Broker o estado atual do output 
   Parâmetros: nenhum
   Retorno: nenhum
*/
void envia_estado_output_MQTT(void)
{
    char estados_motores[4];
    char separador = '-'; 

    estados_motores[0] = estado_motor_direito;
    estados_motores[1] = separador;
    estados_motores[2] = estado_motor_esquerdo;
    estados_motores[3] = '\0';

    MQTT.publish(TOPICO_PUB, estados_motores);
    Serial.print("- Estados dos motores enviados via MQTT (");
    Serial.print(estados_motores);
    Serial.println(")");
    delay(1000);
}

/* Função: inicializa os outputs em nível lógico baixo (desliga os dois motores)
   Parâmetros: nenhum
   Retorno: nenhum
*/
void init_outputs(void)
{
    pinMode(MOTOR_DIRETO, OUTPUT);
    pinMode(MOTOR_ESQUERDO, OUTPUT);
    
    digitalWrite(MOTOR_DIRETO, LOW);          
    digitalWrite(MOTOR_ESQUERDO, LOW);          
}

/* 
 * Programa principal
 */
void loop() 
{   
    /*
    Ações a serem leitas no loop principak:
    - garante funcionamento das conexões WiFi e ao broker MQTT
    - envia o status de todos os outputs para o Broker
    - envia keep-alive da comunicação com broker MQTT
    */
    
    verifica_conexoes_wifi_e_MQTT();    
    envia_estado_output_MQTT();    
    MQTT.loop();
}
