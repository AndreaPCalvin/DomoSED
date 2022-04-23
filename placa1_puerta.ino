#include <Wire.h>
#include <Servo.h>

#define TRIG_PIN 4 //trig ultrasonidos
#define ECHO_PIN 6 //echo ultrasonidos
#define SERVO_PIN 9 // Servo para puerta abierta o cerrada
#define PULSADOR_PIN 3 // Timbre de la puerta
#define BUZZER_PIN 12 // Sonido para visitante
Servo servo;

boolean timbre = false;
boolean abrirPuerta = false;
boolean puertaAbierta = false;


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
  Wire.begin(1); // Se une al bus i2C con la ID #1
  Wire.onReceive(receiveEvent); // Función a ejecutar al recibir datos
  Serial.begin(9600);
  servo.attach(SERVO_PIN);
  //Ultrasonido
  pinMode(TRIG_PIN, OUTPUT); 
  pinMode(ECHO_PIN, INPUT); //Echo
  // Boton por interrupciones
  attachInterrupt(digitalPinToInterrupt(PULSADOR_PIN), botonISR, RISING);
  //pinMode(PULSADOR_PIN, INPUT);
  servo.write(0);
}

void loop() {
  /* No podemos usar un timer para el ultrasonidos porque
   * el Timer 0 lo usan delay(), millis() y las necesitamos,
   * el Timer 1 lo utiliza la biblioteca de servos y la necesitamos,
   * el Timer 2 lo utiliza Tone que lo necesitamos. Medir el
   * ultrasonidos es el único cómputo de esta placa y con este
   * HW precario poco más se puede hacer. (Usamos timers en otras placas)
   */
   ultrasonidos();
   //boton();
   delay(200);
   compruebaYEnviaTimbre();
}

void ultrasonidos(){
  // Si la distancia es pequeña aviso de una presencia en la puerta
  long myDist = distancia();
  if(myDist < 10){
    Serial.print("Hay una persona en la puerta.");
    Serial.print(myDist);
    Serial.println("cm." );
    Wire.beginTransmission(2);
    // Le indico quien soy para que sepa tratar mi mensaje
    I2C_writeAnything('1');
    // Enviamos una 'P' de persona detectada a la placa 2 (centro de control)
    I2C_writeAnything('P');
    Wire.endTransmission();
    compruebaYEnviaTimbre();
    // No estoy enviando todo el rato la misma deteccion de persona
    /* Mientras la persona siga en la puerta simplemente espero
     * a que me indiquen desde control si abrirle la puerta o
     * si la persona que está delante de ella toca el timbre
    */
    while(distancia() < 10){
      compruebaYEnviaTimbre();
      compruebaSiAbrirPuerta();
      delay(200);
    }
    delay(200);
    // Informamos al control que la persona ya no está en la puerta
    Wire.beginTransmission(2);
    // Le indico quien soy para que sepa tratar mi mensaje
    I2C_writeAnything('1');
    // Enviamos una 'n' de no persona a la placa 2 (centro de control)
    I2C_writeAnything('N');
    Wire.endTransmission();
  }
  else{
    Serial.print("No hay nadie aquí.");
    Serial.print(myDist);
    Serial.println("cm." );
    /* Si nos mandaron abrir la puerta y no hay nadie en
     * ella limpiamos el flag 'abrirPuerta' para no 
     * abrirle a la proxima persona que aparezca automáticamente.
     */
    abrirPuerta = false;
    compruebaAbiertaYCierra();
  }
}

void botonISR(){
  timbre = true;
}

void compruebaYEnviaTimbre(){
      if(timbre) {
      enviaTimbre();
      timbre = false;
   }
}

void enviaTimbre(){
  Wire.beginTransmission(2);
  // Le indico quien soy para que sepa tratar mi mensaje
  I2C_writeAnything('1');
  // Enviamos una 'T' de timbre a la placa 2 (centro de control)
  I2C_writeAnything('T');
  Wire.endTransmission();
  while(digitalRead(PULSADOR_PIN) == HIGH);
  Serial.println("Botón pulsado");
}

void receiveEvent(int howMany){
  char aviso;
  I2C_readAnything(aviso);
  // Si recibimos una 'O' de open door abrimos la puerta
  if(aviso == 'O'){
    Serial.println("Recibida apertura puerta");
    abrirPuerta = true;
  }
}

void compruebaSiAbrirPuerta(){
  if(abrirPuerta){
    abrirPuerta = false;
    if(distancia() < 10) {
      Serial.println("Abriendo puerta");
      servo.write(180);
      puertaAbierta = true;
    }
  }
}

void compruebaAbiertaYCierra(){
  if(puertaAbierta){
    puertaAbierta = false;
    servo.write(0);
    delay(700);
    for (int i = 0; i < 1500; i+= 1500){
      tone(BUZZER_PIN,800-i,600);
      delay(400);
    }
  }
}


long distancia(){
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(5);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  //a partir del tiempo, se calcula la distancia
  long myTime = pulseIn(ECHO_PIN, HIGH);
  long myDist = int(0.017*myTime);
  return myDist;
}
