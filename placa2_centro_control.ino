#include <LiquidCrystal.h>
#include <Wire.h>

const int rs = 11, en = 4, d4 = 5, d5 = 6, d6 = 7, d7 = 8;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
#define LED_CALEFACCION_PIN 12 // LED para aire acondicionado
#define LED_LUZ_PIN 13 // LED para aire acondicionado
#define BUZZER_PIN 10 //zumbador
#define PULSADOR_PUERTA_PIN 2 // Pulsador para abrir la puerta
#define PULSADOR_AUTOM_PIN 3 // Pulsador para automatización

#define LUZ_THRESHOLD 750 // Luz umbral para luz natural
#define TEMP_THRESHOLD 18 // Temperatura umbral para calefacción
#define TIME_DEBOUNCE 200 // Tiempo (en ms) de eliminacion de rebotes

boolean abrirPuerta = false;
boolean personaDetectada = false;
boolean personaYaNoEnLaPuerta = false;
boolean timbre = false;
boolean cambioModoAutomatico = false;
boolean automatizacion = true;
long startTime = 0;


template <typename T> int I2C_writeAnything (const T& value)
  {
    const byte * p = (const byte*) &value;
    unsigned int i;
    for (i = 0; i < sizeof value; i++)
          Wire.write(*p++);
    return i;
  }

template <typename T> int I2C_readAnything(T& value)
  {
    byte * p = (byte*) &value;
    unsigned int i;
    for (i = 0; i < sizeof value; i++)
          *p++ = Wire.read();
    return i;
  }

void setup() {
  // Se une al bus i2C con la ID #1
  Wire.begin(2);
  // Función que se ejecuta cuando se reciben datos por el I2C
  Wire.onReceive(receiveEvent);
  Serial.begin(9600);
  // Botón por interrupciones en flanco de subida
  attachInterrupt(digitalPinToInterrupt(PULSADOR_PUERTA_PIN), botonPuertaISR, RISING);
  attachInterrupt(digitalPinToInterrupt(PULSADOR_AUTOM_PIN), botonAutomISR, FALLING);
  pinMode(PULSADOR_PUERTA_PIN, INPUT);
  pinMode(PULSADOR_AUTOM_PIN, INPUT);
  pinMode(LED_CALEFACCION_PIN , OUTPUT);
  pinMode(LED_LUZ_PIN , OUTPUT);
  lcd.begin(16, 2);
  lcd.print("Bienvenido!");
}

// Comprobamos los distintos flags que se activan en las interrupciones para
// realizar la correspondiente acción
void loop() {
  compruebaYEnviaAbrirPuerta();
  compruebaPersonaYaNoPuerta();
  compruebaTimbre();
  muestraModoAutomatico();
}

// Si presionan el botón para abrir la puerta
void botonPuertaISR(){
  // Eliminar rebotes
  if (millis() - startTime > TIME_DEBOUNCE){
    abrirPuerta = true;
    startTime = millis();
  }
}

// Si presionan el botón para cambiar la automatización
void botonAutomISR(){
  // Eliminar rebotes
  if (millis() - startTime > TIME_DEBOUNCE){
    Serial.println("Automatización modificada");
    cambioModoAutomatico = true;
    automatizacion = !automatizacion;
    startTime = millis();
  }
  
}

// Emite en el buffer el sonido cuando tocan al timbre
void sonar(){
  int melodyOff[] = {131, 147};
  int durationOff = 700;
    // Sonido de bienvenida
  for (int thisNote = 0; thisNote < 2; thisNote++) {
      tone(BUZZER_PIN, melodyOff[thisNote], durationOff);
      delay(200);
  }
}

// Si el dueño ha indicado que quiere abrir la puerta se indica a la placa de la puerta
void compruebaYEnviaAbrirPuerta(){
  if(abrirPuerta){
    enviaAbrirPuerta();
    abrirPuerta = false;
  }
}

// Envía un caracter a la placa 1, indicando que abra la puerta al invitado
void enviaAbrirPuerta(){
  Wire.beginTransmission(1); // Transimisión a la placa 1
  // Enviamos una 'O' de Open puerta a la placa 1 (placa de la puerta)
  I2C_writeAnything('O');
  Wire.endTransmission();
}

// Función de rececpción de datos por el bus I2C.
void receiveEvent(int howMany){
  // Leemos el primer caracter que indica quién nos envía información
  char placaRemitente;
  I2C_readAnything(placaRemitente);
  Serial.print("Remitente ");
  Serial.println(placaRemitente);
  // Si nos envía información la placa 1
  if(placaRemitente == '1'){
     char aviso;
     // Leemos el caracter que nos ha enviado
     I2C_readAnything(aviso);
     Serial.print("Aviso ");
     Serial.println(aviso);
     // Si es una 'P', marcamos que hemos detectado a una persona en la puerta y
     // lo mostramos en el LCD
     if(aviso == 'P') {
      personaDetectada=true;
      muestraPersonaDetectada();
     }
     // Si es una 'N' marcamos que ya no está la persona y lo mostramos en el LCD
     else if(aviso == 'N') personaYaNoEnLaPuerta  = true;
     // Si han tocado el timbre, lo marcamos también
     else if(aviso == 'T') timbre = true;
  }
  // Si la información procede de la placa 3
  else if(placaRemitente == '3'){
    // Leemos los datos de temperatura y luz
    float myTemp;
    I2C_readAnything(myTemp);
    int myLuz;
    I2C_readAnything(myLuz);
    // Mostramos la información en el LCD
    muestraMeteoDisplay(myTemp, myLuz);
  }
}

void muestraModoAutomatico(){
  if(cambioModoAutomatico){
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Modo automatico");    
    lcd.setCursor(0, 1);
    if(automatizacion) lcd.print("habilitado");
    else lcd.print("deshabilitado");
    delay(2000);
    cambioModoAutomatico = false;
  }
}

void muestraPersonaDetectada(){
    Serial.println("Persona en la puerta");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Persona");    
    lcd.setCursor(0, 1);
    lcd.print("detectada!");
}

void muestraMeteoDisplay(float myTemp, int myLuz){
   if(!personaDetectada && !personaYaNoEnLaPuerta && !cambioModoAutomatico){
     lcd.clear();
     lcd.setCursor(0, 0);
     lcd.print("Temper: ");
     lcd.print(myTemp);
     lcd.print(" ");
     // Caracter º, para los grados centígrados
     lcd.print((char)223);
     lcd.print("C");
     lcd.setCursor(0, 1);
     // Si la luz es menor que el umbral, es de noche
     if(myLuz < LUZ_THRESHOLD) {
      lcd.print("Es de noche: ");
     }
     // Si no, es de día
     else {
      lcd.print("Es de dia: ");
     }
     // Mostamos también el valor de luz
     lcd.print(myLuz);
   }
   if(automatizacion) modificaCalefaccionYLuz(myTemp, myLuz);
}

void modificaCalefaccionYLuz(float myTemp, int myLuz){
     // Si estamos a menos de 18º, se enciende la calefacción automáticamente
     if(myTemp < TEMP_THRESHOLD) {
      digitalWrite(LED_CALEFACCION_PIN , HIGH);
     }
     // Si no, se apaga la calefacción
     else{
      digitalWrite(LED_CALEFACCION_PIN , LOW);
     }
     // Si la luz es menor que el umbral, es de noche y se enciende la luz artificial
     if(myLuz < LUZ_THRESHOLD) {
      digitalWrite(LED_LUZ_PIN , HIGH);
     }
     // Si no, es de día y se apaga la luz artificial
     else {
      digitalWrite(LED_LUZ_PIN , LOW);
     }
}


void compruebaPersonaYaNoPuerta(){
  if(personaYaNoEnLaPuerta){
    lcd.clear();
    personaDetectada = false;
    lcd.setCursor(0, 0);
    lcd.print("Ya no espera");    
    lcd.setCursor(0, 1);
    lcd.print("en la puerta");
    delay(4000);
    personaYaNoEnLaPuerta = false;
    personaDetectada = false;
  }
}

void compruebaTimbre(){
  if(timbre){
    timbre = false;
    sonar();
  }
}
