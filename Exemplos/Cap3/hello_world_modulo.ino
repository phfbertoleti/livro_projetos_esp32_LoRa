/* Includes para header files das biliotecas do OLED */
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

/* Endereço I2C do display */
#define OLED_ADDR   0x3c

/* distancia, em pixels, de cada linha em relacao ao topo do display */
#define OLED_LINE1     0
#define OLED_LINE2     10
#define OLED_LINE3     20
#define OLED_LINE4     30
#define OLED_LINE5     40

/* Configuração da resolucao do display (este modulo possui display 128x64) */
#define SCREEN_WIDTH    128 
#define SCREEN_HEIGHT   64  

/* Objeto do display */
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, 16);

/* Variaveis */
int contador = 0;

/* Funcao setup */
void setup() 
{
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
}      

/* Programa principal */
void loop() 
{
    display.clearDisplay();
    
    display.setCursor(0, OLED_LINE1);
    display.println("Ola!");
     
    display.setCursor(0, OLED_LINE2);
    display.println("Programa:");

    display.setCursor(0, OLED_LINE3);
    display.println("teste oled");
    
    display.setCursor(0, OLED_LINE4);
    display.print("Tempo ligado: ");

    display.setCursor(0, OLED_LINE5);
    display.print(contador);
    display.println("s");
           
    display.display();

    contador++;
    delay(1000);
}

