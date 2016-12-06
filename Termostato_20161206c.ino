/*
  2016/12/06  Daniel Ferrer
  
  --- PROYECTO CONTROL CALEFACCION ---
  Se trata de realizar un control de temperatura inspirado en el termostato NEST https://nest.com/thermostat/meet-nest-thermostat/
  pero con ciertas limitaciones, aportando además, otras opciones que no posee NEST y que podemos personalizar.
  
  Componentes:
  - 1 WEMOS D1 mini pro https://www.wemos.cc/product/d1-mini-pro.html
  - 1 Display 128x64 OLED 0.96" I2C https://es.aliexpress.com/item/Free-Shipping-1Pcs-white-128X64-OLED-LCD-0-96-I2C-IIC-SPI-Serial-new-original/32389025950.html?spm=2114.13010608.0.0.zo2Y2b&detailNewVersion=&categoryId=400401
  - 1 Rotary encoder https://es.aliexpress.com/store/product/Encoder-module-OpenJumper-rotary-encoder-Encoder-Rotary/121628_32768922357.html?spm=2114.30011108.3.2.WqbQJq&ws_ab_test=searchweb0_0,searchweb201602_5_10065_10068_10084_10083_10080_10082_10081_10060_10061_10062_10056_10055_10037_10054_10059_10032_10099_10078_10079_10077_10073_10100_10096_10070_10052_423_10050_424_10051,searchweb201603_2&btsid=417769ff-2fb2-4c6c-96c3-e9c145efdec3
  - 1 Sensor de temperatura y humedad DHT22 http://www.ebay.es/itm/DHT22-Digital-Temperature-Humidity-Sensor-Module-for-Arduino-DHT-22-raspberry-pi-/322222315164?hash=item4b05f2569c&_uhb=1
  - 1 Resistencia de 10kohm (en caso de tener el DHT22 sin la placa shield de wemos)
  - 1 Condensador de 10nF   (en caso de tener el DHT22 sin la placa shield de wemos)
  - 1 mt de cable rigido de un hilo
  - 3 fichas de empalme
  
  Circuito:
    D0 - Libre (sin usar, en un futuro puede ser un reset tanto para la pantalla como para el wemos o un interruptor para apagar el sistema).
    D1 - SCL pantalla I2C
    D2 - SDA pantalla I2C
    D3 - Sensor DHT22
    D4 - Led placa WEMOS 
    D5 - Encoder rotativo pulsador (pulsado = LOW)
    D6 - Encoder rotativo canal B
    D7 - Encoder rotativo canal A
    D8 - Salida relé activar calor https://www.wemos.cc/product/relay-shield.html

  Funciones:
   - Mostrar en pantalla temperatura y humedad actual.                                                        OK.
   - Mostrar en pantalla temperatura deseada, modificable a través del encoder rotativo.                      OK.
   - Mostrar en pantalla estado de conexión, señal wifi, SSID, estado de activación salida.                   Pendiente.
   - Mejorar la presentación de arranque informando del los pasos del "setup" en pantalla.                    Pendiente.
   - Informar de los fallos o errores que se presentan en comunicación, lectura, etc...                       Pendiente.
   - Mandar a la nube -> thinkspeak, los datos de temperatura, humedad y setpoint temperatura cada 5 minutos. OK.
   - En el caso de cambio de temperatura mayor a 1ºC en menos de 5 minutos, actualizar estado nube.           Pendiente.
   - En el caso de cambio de humedad mayor a 10% en menos de 5 minutos actualizar estado nube.                Pendiente.
   - Servidor WEB para poder modificar la temperatura deseada desde internet bajo password.                   OK.
   - Alarma twitter-thinkspeak cuando la temperatura suba o baje de ciertos límites establecidos.             OK.
   - Estado mediante twitter de la activación del sistema de calefacción.                                     OK.
   - Wemos actualizable a través de OTA.                                                                      Pendiente.
   - Wemos configurable wifi mediante Wifimanager.                                                            OK.
   - Acceso al termostato a través de DNS (NO-IP) por ejemplo.                                                OK. (esto cada uno con su router de casa).
   - Registrar en la nube los eventos de encendido y apagado de la señal de salida (p.e. vía twitter).        Pendiente.
 */

/*----------------- LIBRERIAS ----------------*/
#include <dht.h>                  /*    DHT22 temperatura y humedad       */
#include <ESP8266WiFi.h>          /*    Internet                          */
#include <WiFiClient.h>           /*    Modo cliente                      */
#include <DNSServer.h>            /*    Internet                          */
#include <ESP8266WebServer.h>     /*    Internet                          */
#include <ESP8266mDNS.h>          /*    Internet                          */
#include <WiFiManager.h>          /*    Configurar la conexión wifi       */
#include <Wire.h>                 /*    Pantalla OLED                     */
#include <Adafruit_GFX.h>         /*    Pantalla OLED                     */
#include <Adafruit_SSD1306.h>     /*    Pantalla OLED                     */
#include <ThingSpeak.h>           /*    Datos en la nube                  */
#include <EEPROM.h>               /*    Guardar el Setpoint               */
#include "variables.h"            /*    variables                         */
#include "funciones.h"            /*    Funciones                         */

/*----------------- INSTANCIAS ----------------*/
WiFiClient  client;                   /* Wifi cliente                 */
dht DHT;                              /* Sensor temperatura y humedad */
Adafruit_SSD1306 display(OLED_RESET); /* Pantalla OLED 128x64 pixeles */
ESP8266WebServer server (80);         /* Puerto 80 para comunicar con el Wemos en modo servidor */

/*----------------- ARRANQUE ------------------*/
void setup() {
  byte spEnteroEEPROM;      /* Parte entera de la consigna de temperatura (0-255) */
  byte spDecimalEEPROM;     /* Parte decimal de la consigna de temperatura (0-255) */
  byte arranqueEEPROM;      /* Arranque número de ciclo (0-255) */
  String ciclo;             /* Texto que identifica el ciclo de arranque */
  String mensaje;           /* Mensaje a enviar a través de Twitter */

  inestable = HIGH;         /* Cuando arranca el Wemos, activa esta marca para evitar trabajar con valores no estables. Pasado 30 segundos de arrancar, pasa a LOW */
  Serial.begin(9600);       /* Configuramos el puerto serie */
  WiFiManager wifiManager;  /* Instanciamos a wifimanager para configurar la red si no lo esta */
  wifiManager.autoConnect("Termostato");   /* Nombre SSID del termostato en modo AP */
  ThingSpeak.begin(client); /* Arrancamos el cliente para Thinkspeak */

// Encoder rotativo
  pinMode(ENCODER_A_PIN, INPUT); 
  pinMode(ENCODER_B_PIN, INPUT); 
  pinMode(ENCODER_PUL_PIN, INPUT); 
  attachInterrupt(D5,pulsador,CHANGE);    /* En encoder trabaja por Interrupciones */
  attachInterrupt(D7,encoder,CHANGE);     /* En encoder trabaja por Interrupciones */

// Display
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // inicializa la pantalla I2C con la dirección 0x3C
  display.display();
  display.clearDisplay();                     // Borra la pantalla
  display.setTextColor(WHITE);                /* Color por defecto */
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.display();

/* Pagina WEB */
  Serial.println ( "" );
  Serial.print ( "Conectado a: " );
  Serial.println (WiFi.SSID());
  Serial.print ( "Dirección IP: " );
  Serial.println ( WiFi.localIP() );

  if ( MDNS.begin ( "esp8266" ) ) {
    Serial.println ( "MDNS responder started" );
  }
  server.on ( "/",paginaWEB);                                   /* Pagina WEB principal */
  server.onNotFound (paginaNoEncontrada);                       /* En caso de equivocación del URI */
  server.begin();                                               /* Arranque del servidor */
  Serial.println ("Servidor HTTP a arrancado");                   

// Rele
  pinMode(D8, OUTPUT);

  EEPROM.begin(512);
  spEnteroEEPROM = EEPROM.read(0);    /* Parte entera de la consigna de temperatura (0-255) */
  spDecimalEEPROM = EEPROM.read(1);   /* Parte decimal de la consigna de temperatura (0-255) */
  arranqueEEPROM = EEPROM.read(2);    /* Arranque número de ciclo (0-255) */
  ciclo=itoa(arranqueEEPROM,buf,10);  /* En texto ponemos el nº de arranque que llevamos */
  mensaje = "Arranque nº" + ciclo;
  EEPROM.write(2,byte(arranqueEEPROM + 1));
  EEPROM.commit();
  
  updateTwitterStatus(mensaje);                                           /* Informamos que se ha arrancado el termostato */
  setpoint = float(int(spEnteroEEPROM)) + float(int(spDecimalEEPROM))/10; /* Actualizamos el Setpoint con lo que tenemos en EEPROM */
  encoder_valor_actual = setpoint*10;                                     /* Actualizamos el valor del encoder */
}

void updateTwitterStatus(String tsData) {
  if (client.connect(thingSpeakAddress, 80))
  { 
    // Create HTTP POST Data
    tsData = "api_key="+thingtweetAPIKey+"&status="+tsData;
    client.print("POST /apps/thingtweet/1/statuses/update HTTP/1.1\n");
    client.print("Host: api.thingspeak.com\n");
    client.print("Connection: close\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: ");
    client.print(tsData.length());
    client.print("\n\n");
    client.print(tsData);
    lastConnectionTime = millis();
    if (client.connected()) {
      Serial.println("Connecting to ThingSpeak...");
      Serial.println();
      failedCounter = 0;
    } else {
      failedCounter++;
      Serial.println("Connection to ThingSpeak failed ("+String(failedCounter, DEC)+")");   
      Serial.println();
    }
  } else {
    failedCounter++;
    Serial.println("Connection to ThingSpeak Failed ("+String(failedCounter, DEC)+")");   
    Serial.println();
    lastConnectionTime = millis(); 
  }
}

void encoder() {
  noInterrupts();
  delayMicroseconds(10);
  bool n = digitalRead(ENCODER_A_PIN);
  if ((encoder_aux1 == LOW) && (n == HIGH)) {
    if (digitalRead(ENCODER_B_PIN) == LOW) {
      encoder_valor_actual--;
    } else {
      encoder_valor_actual++;
    }
    encoder_evento_giro = HIGH;  
  } 
  encoder_aux1 = n; 
  interrupts();
}

void pulsador() {
  noInterrupts();
  encoder_estado_pulsador = !digitalRead(ENCODER_PUL_PIN);
  if (encoder_estado_pulsador != encoder_aux2) {
    encoder_aux2 = encoder_estado_pulsador;
    encoder_evento_pulsador = HIGH;
  }
  interrupts();
}

void OB35() { /*100ms*/

}

void OB36() { /*200ms*/
  char car = 247; /* Simbolo de grado centrigrado */
  setpoint = float(encoder_valor_actual)/10;
  floatToString(setpoint);
  display.clearDisplay();  // Borra la pantalla
  display.setTextSize(1);
  display.setCursor(  0, 5);  display.print("Set:");
  display.setTextSize(2);
  display.setCursor(  40, 0);  display.print(realTexto); display.setCursor(100, 0); display.print(car);  display.print("C");
  display.setTextSize(1);
  display.setCursor(  0, 30);  display.print("Act:");
  display.setTextSize(2);
  display.setCursor(  40, 24);  display.print(temperatura_parte_entera);  display.print(".");  display.print(temperatura_parte_decimal); display.setCursor(100, 24); display.print(car);  display.print("C");
  display.setTextSize(1);
  display.setCursor(  25, 55);  display.print("Humedad: "); display.print(humedad_parte_entera);  display.print("%");
  if (rele == HIGH) {
    display.fillCircle(5, 58, 5, WHITE);
  } 
  display.display();
}

void OB37() { /*500ms*/

}

void OB38() { /*1s*/
  float histeresis = 0.1;                                                      /* Histeresis SETPOINT */
  if((setpoint-histeresis) > temperatura_filtrada && inestable == LOW) {       /* Se activa el rele cuando el setpoint-Histeresis es mayor a la temperatura actual */
    rele = HIGH;
    digitalWrite(D8, HIGH);
  }
  if((setpoint+histeresis) < temperatura_filtrada && inestable == LOW) {       /* Se desactiva el rele cuando el setpoint+Histeresis es menor a la temperatura actual */
    rele = LOW;
    digitalWrite(D8, LOW);
  }
}

void OB39() { /*2s*/
  int chk = DHT.read22(DHT22_PIN);  
  filtro(DHT.temperature,DHT.humidity); 
}

void OB40() { /*5s*/
  byte entero = byte(floor(setpoint));
  byte decimal;
  if (setpoint != setpoint_anterior) {
    if ((round(setpoint*10) - floor(setpoint)*10) == 10) {
      decimal = 0;
    } else {
      decimal = round(setpoint*10) - floor(setpoint)*10;
    }
    EEPROM.write(0, entero); 
    EEPROM.write(1, decimal); 
    EEPROM.commit();
    setpoint_anterior = setpoint;
  } 
}

void OB41() { /*10s*/

}

void OB42() { /*30s*/

}

void OB43() { /*1m*/

}

void OB44() { /*5m*/
  int humedadEntero;
  if(inestable == LOW) {  /* Cada 5 minutos, enviamos a thinkspeak los valores filtrados */
    display.clearDisplay();  // Borra la pantalla
    display.setCursor(11,10);
    display.print("SUBIENDO DATOS");
    display.setCursor(14,28);
    display.print("A THINGSPEAK");
    display.setCursor(10,46);
    display.print("ESPERE POR FAVOR");
    display.display();

    humedadEntero = int(round(humedad_filtrada));  /* Convertimos el valor de humedad en un entero para redondear en thinkspeak */
    
    ThingSpeak.setField(1,temperatura_filtrada);   /* Dato de la temperatura actual a enviar a thinkspeak */
    ThingSpeak.setField(2,humedadEntero);          /* Dato de la humedad actual a enviar a thinkspeak */
    ThingSpeak.setField(3,setpoint);               /* Dato del setpoint actual a enviar a thinkspeak */
    ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey); /* Enviamos los datos */

    display.clearDisplay();  // Borra la pantalla
    display.setCursor(0,0);
    display.display();
  }
}

void periodicas() {
  unsigned long T = millis();
  if (LT[0]+MS[0] <= T)   {if (LT[0] < MAXUL - MS[0])   {LT[0] = T;} else {LT[0] = MAXUL - T;} OB35();} /*100ms*/
  if (LT[1]+MS[1] <= T)   {if (LT[1] < MAXUL - MS[1])   {LT[1] = T;} else {LT[1] = MAXUL - T;} OB36();} /*200ms*/
  if (LT[2]+MS[2] <= T)   {if (LT[2] < MAXUL - MS[2])   {LT[2] = T;} else {LT[2] = MAXUL - T;} OB37();} /*500ms*/
  if (LT[3]+MS[3] <= T)   {if (LT[3] < MAXUL - MS[3])   {LT[3] = T;} else {LT[3] = MAXUL - T;} OB38();} /*1s   */
  if (LT[4]+MS[4] <= T)   {if (LT[4] < MAXUL - MS[4])   {LT[4] = T;} else {LT[4] = MAXUL - T;} OB39();} /*2s   */
  if (LT[5]+MS[5] <= T)   {if (LT[5] < MAXUL - MS[5])   {LT[5] = T;} else {LT[5] = MAXUL - T;} OB40();} /*5s   */
  if (LT[6]+MS[6] <= T)   {if (LT[6] < MAXUL - MS[6])   {LT[6] = T;} else {LT[6] = MAXUL - T;} OB41();} /*10s  */
  if (LT[7]+MS[7] <= T)   {if (LT[7] < MAXUL - MS[7])   {LT[7] = T;} else {LT[7] = MAXUL - T;} OB42();} /*30s  */
  if (LT[8]+MS[8] <= T)   {if (LT[8] < MAXUL - MS[8])   {LT[8] = T;} else {LT[8] = MAXUL - T;} OB43();} /*1m   */
  if (LT[9]+MS[9] <= T)   {if (LT[9] < MAXUL - MS[9])   {LT[9] = T;} else {LT[9] = MAXUL - T;} OB44();} /*5m   */
}

void infoTwitterSetpoint() {
  static float ant;
  static bool b;
  static long w;
  byte nCambio;
  String cambio;
  String mensaje;
    if (setpoint != ant) {
    w = millis();
    b = HIGH;
    ant = setpoint;
  }

  if (millis() >= w + 5000 && b==HIGH) {              /* En el caso de cambio de setpoint espera 5 segundos desde el ultimo cambio */
    display.clearDisplay();  // Borra la pantalla
    display.setCursor(11,10);
    display.print("SUBIENDO DATOS");
    display.setCursor(14,28);
    display.print("A TWITTER");
    display.setCursor(10,46);
    display.print("ESPERE POR FAVOR");
    display.display();
    nCambio = EEPROM.read(3);
    cambio=itoa(nCambio,buf,10);
    floatToString(setpoint); 
    mensaje = "Consigna " + realTexto + "ºC" + " Cambio nº" + cambio;
    updateTwitterStatus(mensaje);                     /* Mandamos mensaje twitter a usuario informando del cambio de setpoint */
    b = LOW;
    EEPROM.write(3,byte(nCambio + 1));                /* Guardamos en EEPROM el numero de cambio */
    EEPROM.commit();
  }
}

void estabilidad() {
  if (millis() >= 30000 && inestable == HIGH) { /* Esperamos 30 segundos después del arranque a que se carge el buffer del filtro y tengamos buenos valores medios */
    inestable = LOW;
  }
}

void paginaNoEncontrada() { /* Pagina WEB no encontrada */
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += ( server.method() == HTTP_GET ) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for ( uint8_t i = 0; i < server.args(); i++ ) {
    message += " " + server.argName ( i ) + ": " + server.arg ( i ) + "\n";
  }
  server.send ( 404, "text/plain", message );
}

void paginaWEB() {   /* Pagina WEB principal */
String temp;
String aux;
static char *texto = "";
static bool pas;
float consignaReal;

if (server.arg("codigo")==PASSWORD_WEB && (server.arg("Boton")=="Aceptar")){
  pas = HIGH;
  aux=server.arg("consigna");
  consignaReal=aux.toFloat();
  setpoint = consignaReal;
  encoder_valor_actual = setpoint*10;      
}

if (server.arg("codigo")!=PASSWORD_WEB && (server.arg("Boton")=="Aceptar")){
  pas = LOW;  
}
  temp = "<html>\
            <head>\
              <meta http-equiv=\"Content-Type\" content=\"text/html; charset=windows-1252\">\
              <title>Termostato</title>\                                                                    
            </head>\
            <body>\
              <font face=\"Comic Sans MS\" size=\"16\">\
                TERMOSTATO\                                                                               
              </font>\
              <p><font face=\"Comic Sans MS\" size=\"5\">\
                Password:";                                                                                
  temp += (pas)?"":" Incorrecto"; 
  temp +="</font></p>\
           <form method = \"post\">\
           <input type=\"password\" name=\"codigo\" style=\"border-style:solid; border-width:2px; width: 189; height: 62; font-size:24pt\" tabindex=\"1\">"; 
  temp +="<p><font face=\"Comic Sans MS\" size=\"5\">Consigna nueva:</font></p>";  
  temp +="<input type=\"consigna\" name=\"consigna\" style=\"border-style:solid; border-width:2px; width: 189; height: 62; font-size:24pt\" tabindex=\"2\" >"; 
  temp +="<input name=\"Boton\" type=\"submit\" value=\"Aceptar\" style=\"width: 189; height: 62; font-size:24pt\" tabindex=\"3\"> </form>";
  floatToString(setpoint);
  temp +="<p><font face=\"Comic Sans MS\" size=\"8\">Consigna actual: " + realTexto + " &degC </font></p>" ;
  floatToString(temperatura_filtrada);
  temp +="<p><font face=\"Comic Sans MS\" size=\"8\">Temperatura actual: " + realTexto + " &degC </font></p>";
  temp +="<p><font face=\"Comic Sans MS\" size=\"10\">Estado actual:</font><font face=\"Comic Sans MS\" size=\"10\""; 
  temp += (rele)?"color=\"#00FF00\">":"color=\"#FF0000\">";
  temp += (rele)?"Calentando":"Sin calentar";
  temp += "</font></p>";
  temp += "</body>\
           </html> "; 
  
  server.send ( 200, "text/html", temp );
}

void loop() {
  periodicas();
  infoTwitterSetpoint();
  estabilidad();
  server.handleClient();
}

