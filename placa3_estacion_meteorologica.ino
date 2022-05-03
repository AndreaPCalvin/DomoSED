#include <DHT.h>
#include <Wire.h>
#include <TimerOne.h>
#define DHTTYPE DHT11 // Modelo del sensor DHT11
#define DHT_PIN 8 // DHT11
int LDR = A0; // LDR, analogico

template <typename T> int I2C_writeAnything (const T& value)
  {
    const byte * p = (const byte*) &value;
    unsigned int i;
    for (i = 0; i < sizeof value; i++)
          Wire.write(*p++);
    return i;
  }  // end of I2C_writeAnything


float myTemp;
int ldr_val;
boolean enviarDatos = false;
  
void humedadTemp();
void luz();
 
// Inicializamos el sensor DHT11 con sus características
DHT dht(DHT_PIN, DHTTYPE);

void setup() {
  Wire.begin(3); // Se une al bus i2C con la ID #3
  Serial.begin(9600);
  dht.begin();
  // Utilizamos un timer para medir cada 1.5 segundos la meteorología
  Timer1.initialize(1500000);
  Timer1.attachInterrupt(medirMeteorologia);
}

void loop() {
  comprobarEnviarDatos();
}


void medirMeteorologia(){
  // Indicamos que hay que enviar nuevos datos
  enviarDatos = true;
}

void comprobarEnviarDatos(){
  if(enviarDatos){
    enviarDatos = false;
    // Leemos la temperatura
    myTemp = dht.readTemperature();
    // Leemos la luz
    ldr_val = analogRead(LDR);
    Wire.beginTransmission(2);
    I2C_writeAnything ('3');
    I2C_writeAnything (myTemp);
    I2C_writeAnything (ldr_val);
    Wire.endTransmission();
    Serial.print("Temperatura: ");
    Serial.print(myTemp);
    Serial.println(" *C ");
    Serial.println(ldr_val);
    //Ejemplo luz
    if(ldr_val < 500){
      Serial.println("Es de noche.");
    }
    else{
      Serial.println("Es de día.");
    }
  }
}

void humedadTemp(){
  // Leemos la temperatura en grados centígrados (por defecto)
  myTemp = dht.readTemperature();
  // Comprobamos si ha habido algún error en la lectura
  if (isnan(myTemp)) {
    Serial.println("Error obteniendo los datos del sensor DHT11");
    return;
  }
  // Calcular el índice de calor en grados centígrados
  //float myIndxCal = dht.computeHeatIndex(myTemp, myHum, false);

  //Serial.print("Humedad: ");
  //Serial.print(myHum);
  //Serial.println(" %\t");
  Serial.print("Temperatura: ");
  Serial.print(myTemp);
  Serial.println(" *C ");
  //Serial.print("Índice de calor: ");
  //Serial.print(myIndxCal);
  //Serial.println(" *C ");
}

void luz(){
  // Leemos la iluminación y encedemos led si está oscuro
  ldr_val = analogRead(LDR);

  Serial.println(ldr_val);
  //Ejemplo luz
  if(ldr_val < 300){
    Serial.println("Es de noche.");
  }
  else{
    Serial.println("Es de día.");
  }
}

void receiveEvent(int howMany){

}
