//--------------DEFINE----------------------------------------
#define DHT22_PIN         D3
#define ENCODER_A_PIN     D7
#define ENCODER_B_PIN     D6
#define ENCODER_PUL_PIN   D5
#define OLED_RESET         0 

//--------------VARIABLES-------------------------------------
/* Periodicas */
const unsigned long MAXUL = 4294967295UL; 
unsigned long LT[10] = {0,0,0,0,0,0,0,0,0,0};
unsigned long MS[10] = {100,200,500,1000,2000,5000,10000,30000,60000,300000};

/* DHT22 */
bool  inestable;
float humedad;
float temperatura;
float setpoint;
float setpoint_anterior;
float humedad_filtrada;
float temperatura_filtrada;
int   humedad_parte_entera;
int   temperatura_parte_entera;
int   temperatura_parte_decimal;

/* Encoder */
volatile int  encoder_valor_actual;
volatile bool encoder_evento_giro;
volatile bool encoder_evento_pulsador;
volatile bool encoder_estado_pulsador;
bool          encoder_aux1;
bool          encoder_aux2;
int           aux;

/* ThingSpeak */
unsigned long myChannelNumber = xxx;
const char * myWriteAPIKey = "xxx";
String    thingtweetAPIKey = "xxx";
char   thingSpeakAddress[] = "api.thingspeak.com";
long    lastConnectionTime = 0; 
boolean      lastConnected = false;
int          failedCounter = 0;

/* Para pasar de real a cadena */
String realTexto;       /* Texto entero + decimal */
char buf[12];           /* Auxiliar */



