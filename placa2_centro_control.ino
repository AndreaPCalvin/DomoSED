#include <LiquidCrystal.h>
#include <Wire.h>

const int rs = 3, en = 4, d4 = 5, d5 = 6, d6 = 7, d7 = 8;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
#define LED_CALEFACCION_PIN 12 // LED para aire acondicionado
#define LED_LUZ_PIN 13 // LED para aire acondicionado
#define BUZZER_PIN 10 //zumbador
#define PULSADOR_PIN 2 // Pulsador

boolean abrirPuerta = false;
boolean personaDetectada = false;
boolean personaYaNoEnLaPuerta = false;
boolean timbre = false;


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
  Wire.begin(2); // Se une al bus i2C con la ID #1
  Wire.onReceive(receiveEvent); // Función a ejecutar al recibir datos
  Serial.begin(9600);
  // Botón por interrupciones
  attachInterrupt(digitalPinToInterrupt(PULSADOR_PIN), botonISR, RISING);
  pinMode(PULSADOR_PIN, INPUT);
  pinMode(LED_CALEFACCION_PIN , OUTPUT);
  pinMode(LED_LUZ_PIN , OUTPUT);
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  // Print a message to the LCD.
  lcd.print("Bienvenido!");
}

void loop() {
  compruebaYEnviaAbrirPuerta();
  compruebaPersonaYaNoPuerta();
  compruebaTimbre();
}


void botonISR(){
  abrirPuerta = true;
}

void sonar(){
  int melodyOff[] = {131, 147};
  int durationOff = 200;
    // Sonido de bienvenida
  for (int thisNote = 0; thisNote < 2; thisNote++) {
      tone(BUZZER_PIN, melodyOff[thisNote], 700);
      delay(200);
  }
}

void compruebaYEnviaAbrirPuerta(){
  if(abrirPuerta){
    enviaAbrirPuerta();
    abrirPuerta = false;
  }
}

void enviaAbrirPuerta(){
  Wire.beginTransmission(1);
  // Enviamos una 'O' de Open puerta a la placa 1 (placa de la puerta)
  I2C_writeAnything('O');
  Wire.endTransmission();
}

// Esta función se ejecutará al recibir datos, lo cual provocará que se salga del loop principal.
void receiveEvent(int howMany){
  char placaRemitente;
  I2C_readAnything(placaRemitente);
  Serial.print("Remitente ");
  Serial.println(placaRemitente);
  if(placaRemitente == '1'){
     char aviso;
     I2C_readAnything(aviso);
     Serial.print("Aviso ");
     Serial.println(aviso);
     if(aviso == 'P') {
      personaDetectada=true;
      muestraPersonaDetectada();
     }
     else if(aviso == 'N') personaYaNoEnLaPuerta  = true;
     else if(aviso == 'T') timbre = true;
  }
  else if(placaRemitente == '3'){
    float myTemp;
    I2C_readAnything(myTemp);
    int myLuz;
    I2C_readAnything(myLuz);
    muestraMeteoDisplay(myTemp, myLuz);
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
   if(!personaDetectada && !personaYaNoEnLaPuerta){
     lcd.clear();
     lcd.setCursor(0, 0);
     lcd.print("Temper: ");
     lcd.print(myTemp);
     lcd.print(" ");
     lcd.print((char)223);
     lcd.print("C");
     if(myTemp < 18) {
      digitalWrite(LED_CALEFACCION_PIN , HIGH);
     }
     else{
      digitalWrite(LED_CALEFACCION_PIN , LOW);
     }
     lcd.setCursor(0, 1);
     if(myLuz > 500) {
      lcd.print("Es de dia: ");
      digitalWrite(LED_LUZ_PIN , LOW);
     }
     else {
      lcd.print("Es de noche: ");
      digitalWrite(LED_LUZ_PIN , HIGH);
     }
     lcd.print(myLuz);
   }
}

void encenderCalefaccionYLuz(float myTemp, int myLuz){
  if(myLuz > 500) {
    digitalWrite(LED_LUZ_PIN , LOW);
  } else {
    digitalWrite(LED_LUZ_PIN , HIGH);
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
