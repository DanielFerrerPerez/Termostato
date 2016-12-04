void floatToString (float real) {
  int ent;
  int dec;
  char buf[12];
  String enteroTexto;   
  String decimalTexto;  
  ent = int(floor(real));
  if ((round(real*10) - floor(real)*10) == 10) {
    dec = 0;
  } else {
    dec = round(real*10) - floor(real)*10;
  }
    enteroTexto=itoa(ent,buf,10);
    decimalTexto=itoa(dec,buf,10); 
    realTexto = enteroTexto  + "." + decimalTexto; 
}

void filtro(int temperatura, int humedad){
  static float hume[10];
  static float temp[10];
  static int x = 0;
  static int y = 0;
  static int humedad_max_i = 0;
  static int humedad_min_i = 0;
  static float humedad_max = 0.0;
  static float humedad_min = 0.0;
  static float humedad_suma = 0.0;
  static int temperatura_max_i = 0;
  static int temperatura_min_i = 0;
  static float temperatura_max = 0.0;
  static float temperatura_min = 0.0;
  static float temperatura_suma = 0.0;
  
  if ((humedad >= 0.0) && (humedad <=100.0) && (temperatura >= -20.0) && (temperatura <= 60.0)) {
  temperatura = temperatura +  (0.0);
  humedad = humedad + (0.0);
  /*Calculo de la humedad filtrada*/
  if (x < 10) { hume[x] = humedad; temp[x] = temperatura; x++; } 
  if (x >= 10) { x=0; }

  humedad_max = 0.0;
  humedad_min = 100.0;
  temperatura_max = -30.0;
  temperatura_min = 100.0;
  for(int q=0;q<10;q++) {
    if (hume[q] > humedad_max) {
      humedad_max = hume[q];
      humedad_max_i = q;
    }
    if (hume[q] < humedad_min) {
      humedad_min = hume[q];
      humedad_min_i = q;
    }
    if (temp[q] > temperatura_max) {
      temperatura_max = temp[q];
      temperatura_max_i = q;
    }
    if (temp[q] < temperatura_min) {
      temperatura_min = temp[q];
      temperatura_min_i = q;
    }
  }
  humedad_suma = 0.0;
  temperatura_suma = 0.0;
  for(int q=0;q<10;q++) { 
    if (humedad_max_i == humedad_min_i) {if (humedad_max_i < 8) {humedad_max_i++;} else {humedad_max_i--;}}
    if ((q != humedad_max_i) && (q != humedad_min_i)) {
      humedad_suma = humedad_suma + hume[q];
    }
    if (temperatura_max_i == temperatura_min_i) {if (temperatura_max_i < 8) {temperatura_max_i++;} else {temperatura_max_i--;} }
    if ((q != temperatura_max_i)&&(q != temperatura_min_i)) {
      temperatura_suma = temperatura_suma + temp[q];
    }
  }
  humedad_filtrada = humedad_suma/8;
  temperatura_filtrada = temperatura_suma/8;

  humedad_parte_entera = round(humedad_filtrada);
  temperatura_parte_entera = floor(temperatura_filtrada);
  if ((round(temperatura_filtrada*10) - floor(temperatura_filtrada)*10) == 10) {
    temperatura_parte_decimal = 0;
  } else {
    temperatura_parte_decimal = round(temperatura_filtrada*10) - floor(temperatura_filtrada)*10;
  }
  }
}

