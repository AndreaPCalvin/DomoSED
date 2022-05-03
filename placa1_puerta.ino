#include <Wire.h>
#include <Servo.h>

#define TRIG_PIN 4 //trig ultrasonidos
#define ECHO_PIN 6 //echo ultrasonidos
#define SERVO_PIN 9 // Servo para puerta abierta o cerrada
#define PULSADOR_PIN 3 // Timbre de la puerta
#define BUZZER_PIN 12 // Sonido para visitante
#define TIME_DEBOUNCE 200 // Tiempo (en ms) de eliminacion de rebotes
Servo servo;

boolean timbre = false;
boolean abrirPuerta = false;
boolean puertaAbierta = false;
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
   * HW poco más se puede hacer. (Usamos timers en otras placas)
   */
   ultrasonidos();
   delay(200);
   compruebaYEnviaTimbre();
}

void ultrasonidos(){
  // Medimos la distancia a la puerta
  long myDist = distancia();
  // Si la distancia es suficientemente pequeña
  if(myDist < 10){
    Serial.print("Hay una persona en la puerta.");
    Serial.print(myDist);
    Serial.println("cm." );
    // Informamos al control de que hay una persona delante de la puerta
    enviaInfoACentroControl('P');
    compruebaYEnviaTimbre();
    /* Mientras la persona siga en la puerta simplemente espero
     * a que me indiquen desde control si abrirle la puerta o
     * si la persona que está delante de ella toca el timbre.
     * Con ello no enviamos todo el rato la misma deteccion de persona
    */
    while(distancia() < 10){
      compruebaYEnviaTimbre();
      compruebaSiAbrirPuerta();
      delay(200);
    }
    delay(200);
    // Informamos al control que la persona ya no está en la puerta
    enviaInfoACentroControl('N');
  }
  // 
  else{
    Serial.print("No hay nadie aquí.");
    Serial.print(myDist);
    Serial.println("cm." );
    /* Si nos mandaron abrir la puerta y no hay nadie en
     * ella limpiamos el flag 'abrirPuerta' para no 
     * abrirle a la proxima persona que aparezca automáticamente.
     * Lo mismo con el timbre.
     */
    abrirPuerta = false;
    // Comprobamos si la puerta está abierta, para cerrarla
    compruebaAbiertaYCierra();
  }
}

void enviaInfoACentroControl(char info){
    // Enviamos un mensaje al control (placa 2)
    Wire.beginTransmission(2);
    // Le indico quien soy para que sepa tratar mi mensaje
    I2C_writeAnything('1');
    // Enviamos el caracter con la información
    I2C_writeAnything(info);
    Wire.endTransmission();
}


void botonISR(){
  timbre = true;
}

void compruebaYEnviaTimbre(){
   if(timbre && distancia() < 10) {
      enviaTimbre();
   }
   timbre = false;
}

void enviaTimbre(){
  // Indicamos al centro de control que han tocado el timbre
  enviaInfoACentroControl('T');
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
  // Si está a activado el flag que indica que hay que abrir la puerta
  if(abrirPuerta){
    // Desactivamos el flag
    abrirPuerta = false;
    // Siempre que siga habiendo presencia delante de la puerta
    if(distancia() < 10) {
      Serial.println("Abriendo puerta");
      // Abrimos la puerta
      servo.write(180);
      puertaAbierta = true;
    }
    // Esperamos tiempor suficiente para que la puerta esté abierta
    delay(700);
    // Emitimos un sonido para que las personas ciegas sepan que puedan pasar ya
    pitidoCiegos();
  }
}

void pitidoCiegos(){
   for (int i = 0; i < 1500; i+= 1500){
      tone(BUZZER_PIN,800-i,600);
      delay(400);
   }
}

// Se llama a esta función sólo si ya no hay presencia delante de la puerta
void compruebaAbiertaYCierra(){
  // Si la puerta está abierta
  if(puertaAbierta){
    // Cerramos la puerta
    servo.write(0);
    // Actualizamos el flag para indicar que queda cerrada
    puertaAbierta = false;
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
