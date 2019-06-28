/*
* Projeto: emissor - medição de turbudez da agua via LoRa
* Autor: Pedro Bertoleti
*/

#include <LoRa.h>
#include <SPI.h>
#include <Wire.h>

/* Definicoes para comunicação com radio LoRa */
#define SCK_LORA 5
#define MISO_LORA 19
#define MOSI_LORA 27
#define RESET_PIN_LORA 14
#define SS_PIN_LORA 18
#define HIGH_GAIN_LORA 20 /* dBm */
#define BAND 915E6 /* 915MHz de frequencia */

/* Definições gerais */
#define DEBUG_SERIAL_BAUDRATE 115200

/* Definições do buffer (para filtro de média móvel) */
#define TAMANHO_BUFFER 50

/* Definições da leitura do sensor de turbidez */
#define ADC_MAX 4095 /* ADC do modulo WiFi LoRa 32(V2) tem 12 bits de resolucao */
#define ENTRADA_ANALOGICA_SENSOR_TURBIDEZ 13 /* GPIO 13 => ADC2_CH4 */
#define TENSAO_MAXIMA_SAIDA_SENSOR_TURBIDEZ 5 /* Sensor apresenta saida analogica de 0 a 5V */
#define FATOR_DIVISOR_TENSAO 10 /* No projeto, para serem respeitados os limites de operação do 
                                   ADC e fazer com que a tensão do sensor excursione corretamente em
                                   todo seu range, ha um divisor resistivo para que a tensão lida pelo
                                   canal do ADC utilizado seja igual a 10% da tensão real de saída do sensor. 
                                   Portanto, no cálculo da tensão real, este fator é utilizado
                                   para se recuperar corretamente este valor */
#define NUM_LEITURAS_OVERSAMPLING 512 

/* Variáveis globais */
float buffer_tensao_sensor_turbidez[TAMANHO_BUFFER];

/* Protótipos */
bool init_comunicacao_lora(void);
float le_tensao_sensor_turbidez_sem_filtro(void);
float le_tensao_sensor_turbidez_com_filtro(void);
float le_turbidez_da_agua(void);

/* Função: inicia comunicação com chip LoRa
* Parâmetros: nenhum
* Retorno: true: comunicação ok
* false: falha na comunicação
*/
bool init_comunicacao_lora(void)
{
    bool status_init = false;
    
    Serial.println("[Modulo medidor] Tentando iniciar comunicacao com o radio LoRa...");
    SPI.begin(SCK_LORA, MISO_LORA, MOSI_LORA, SS_PIN_LORA);
    LoRa.setPins(SS_PIN_LORA, RESET_PIN_LORA, LORA_DEFAULT_DIO0_PIN);

    if (!LoRa.begin(BAND)) 
    {
        Serial.println("[Modulo medidor] Comunicacao com o radio LoRa falhou. Nova tentativa em 1 segundo..."); 
        delay(1000);
        status_init = false;
    }
    else
    {
        /* Configura o ganho do receptor LoRa para 20dBm, o maior ganho possível (visando maior alcance possível) */ 
        LoRa.setTxPower(HIGH_GAIN_LORA); 
        Serial.println("[Modulo medidor] Comunicacao com o radio LoRa ok");
        status_init = true;
    }
    return status_init;
}

/* Função: faz a leitura da tensão de saída do sensor de turbidez (sem filtro)
* Parâmetros: nenhum 
* Retorno: tensão de saída (de 0 até 5V)
*/
float le_tensao_sensor_turbidez_sem_filtro(void)
{
    float tensao_saida_sensor = 0.0;
    int leitura_adc = 0;
    
    /* Faz a leitura do canal do ADC do modulo que a saida do sensor de turbidez está ligado */
    leitura_adc = analogRead(ENTRADA_ANALOGICA_SENSOR_TURBIDEZ);
  
    /* converte a leitura do ADC (canal onde está ligada a saída do sensor de turbidez) em uma tensão de leitura de 0 a 5V */
    tensao_saida_sensor = ((leitura_adc/(float)ADC_MAX)*(float)TENSAO_MAXIMA_SAIDA_SENSOR_TURBIDEZ);
    Serial.print("[Modulo medidor] Tensao de saida (no divisor): ");
    Serial.print(tensao_saida_sensor);
    Serial.println("V");
    tensao_saida_sensor = tensao_saida_sensor*FATOR_DIVISOR_TENSAO; /* Corrige a leitura com base no divisor resistivo utilizado */
    return tensao_saida_sensor;
}

/* Função: faz a leitura da tensao de saida do sensor de turbidez (com filtro)
* Parâmetros: nenhum 
* Retorno: tensão de saída (de 0 até 5V)
*/
float le_tensao_sensor_turbidez_com_filtro(void)
{
    float tensao_saida_sensor = 0.0;
    int leitura_adc = 0;
    long soma_leituras_adc = 0;
    int i = 0;
    float soma = 0.0;
    float tensao_filtrada = 0.0;
    
    /* Faz a leitura do canal do ADC do modulo que a saída do sensor de turbidez está ligado */
    /* Para amenizar efeitos de leituras ruidosas / com oscilações, um oversampling é feito */
    soma_leituras_adc = 0;
    for (i=0; i<NUM_LEITURAS_OVERSAMPLING; i++)
        soma_leituras_adc = soma_leituras_adc + analogRead(ENTRADA_ANALOGICA_SENSOR_TURBIDEZ);
    
    leitura_adc = soma_leituras_adc/NUM_LEITURAS_OVERSAMPLING;
    
    /* converte a leitura do ADC (canal onde está ligada a saída do sensor de turbidez) em uma tensão de leitura de 0 a 5V */
    tensao_saida_sensor = ((leitura_adc/(float)ADC_MAX)*(float)TENSAO_MAXIMA_SAIDA_SENSOR_TURBIDEZ);
    Serial.print("Counts de ADC: ");
    Serial.println(leitura_adc);
    Serial.print("[Modulo medidor] Tensao de saida (no divisor): ");
    Serial.print(tensao_saida_sensor);
    Serial.println("V");
    tensao_saida_sensor = tensao_saida_sensor*FATOR_DIVISOR_TENSAO; /* Corrige a leitura com base no divisor resistivo utilizado */
    
    /* Desloca para a esquerda todas as medidas de tensão já feitas */
    for(i=1; i<TAMANHO_BUFFER; i++)
        buffer_tensao_sensor_turbidez[i-1] = buffer_tensao_sensor_turbidez[i];
    
    /* Insere a nova medida na última posição do buffer */
    buffer_tensao_sensor_turbidez[TAMANHO_BUFFER-1] = tensao_saida_sensor;
   
    /* Calcula a média do buffer (valor da média móvel, ou valor filtrado) */
    soma = 0.0;
    tensao_filtrada = 0.0;
    
    for(i=0; i<TAMANHO_BUFFER; i++)
        soma = soma + buffer_tensao_sensor_turbidez[i];
  
    tensao_filtrada = soma/TAMANHO_BUFFER;
    return tensao_filtrada;
}

/* Função: faz a leitura da turbidez da agua
* Parâmetros: nenhum 
* Retorno: turbidez da água (em NTU)
*/
float le_turbidez_da_agua(void)
{
    float turbidez_agua = 0.0;
    float tensao_filtrada = 0.0;
    float primeiro_fator = 0.0;
    float segundo_fator = 0.0;
    float terceiro_fator = 0.0;
    
    /* Faz a leitura da tensão filtrada do sensor de turbidez */
    tensao_filtrada = le_tensao_sensor_turbidez_com_filtro();
  
    /* Limita a tensão a máxima permitida pelo sensor */
    if (tensao_filtrada > 4.2)
        tensao_filtrada = 4.2;
      
    Serial.print("[Modulo medidor] Tensao de saida do sensor de turbidez: ");
    Serial.print(tensao_filtrada);
    Serial.println("V");
    
    /* Calcula a turbidez */
    primeiro_fator = ((-1)*(1120.4) * tensao_filtrada * tensao_filtrada);
    segundo_fator = (5742.3 * tensao_filtrada);
    terceiro_fator = ((-1)*4352.9);
    turbidez_agua = primeiro_fator + segundo_fator + terceiro_fator;
 
    return turbidez_agua; 
}

/* Função de setup */
void setup() 
{ 
    int i;
    
    Serial.begin(DEBUG_SERIAL_BAUDRATE);
    
    /* Configura ADC em sua resolução máxima (12 bits) */
    analogReadResolution(12);
    
    /* Inicializa buffer de leituras de tensão do sensor de turbidez  (para inicializar o buffer do filtro de média móvel)*/
    for(i=0; i<TAMANHO_BUFFER; i++)
        buffer_tensao_sensor_turbidez[i] = le_tensao_sensor_turbidez_sem_filtro(); 
 
    while (!Serial);
    
    /* Tenta, até obter sucesso, comunicação com o chip LoRa */
    while(init_comunicacao_lora() == false); 
}

/* Programa principal */
void loop() 
{
    float turbidez_da_agua = 0.0;
    long inicio_medicoes = 0;
 
    /* Faz a leitura da turbidez da água (em NTU) */
    inicio_medicoes = millis();
    
    while ((millis() - inicio_medicoes) < 1000)
    {
        turbidez_da_agua = le_turbidez_da_agua();
    }
     
    /* Envia a turbidez da água medida */
    LoRa.beginPacket();
    LoRa.write((unsigned char *)&turbidez_da_agua, sizeof(float));
    LoRa.endPacket();
    
    /* aguarda um segundo para nova leitura e envio */
    delay(1000); 
 }
