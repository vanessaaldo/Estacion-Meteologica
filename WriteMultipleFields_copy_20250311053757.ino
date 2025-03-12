#include <WiFi.h>
#include "secrets.h"
#include "ThingSpeak.h" 
#include "DHT.h"
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define pin2 32  
#define buzzerPin 13  
#define TEMP_THRESHOLD 27.0  
#define HUM_THRESHOLD 60.0   

DHT dht2(pin2, DHT22);  
Adafruit_BMP280 bme;    

char ssid[] = SECRET_SSID;   
char pass[] = SECRET_PASS;   
WiFiClient  client;

unsigned long myChannelNumber = SECRET_CH_ID;
const char * myWriteAPIKey = SECRET_WRITE_APIKEY;

float tempDHT = 0;
float humedad = 0;
float tempBMP = 0;
float presion = 0;

void setup() {
  Serial.begin(115200);  
  WiFi.mode(WIFI_STA);   
  ThingSpeak.begin(client);  

  dht2.begin();
  Wire.begin(21, 22);  

  if (!bme.begin(0x76)) {  
    Serial.println(F("Error al inicializar el sensor BMP280"));
    while (1);  
  }
  Serial.println(F("Sensor BMP280 inicializado correctamente"));

  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, LOW);  

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {  
    Serial.println(F("Error al inicializar la pantalla OLED"));
    while (1);
  }
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);

  conectarWiFi();
}

void loop() {
   leerdht2();
   leerBMP280();
   actualizarOLED();
   enviarAThingSpeak();

   delay(20000);  // Esperar 20 segundos antes de la siguiente actualización
}

void conectarWiFi() {
  Serial.print("Conectando a WiFi...");
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("\nConectado a WiFi.");
}

void leerdht2() {
  float temp = dht2.readTemperature();
  float hum = dht2.readHumidity();

  if (!isnan(temp) && !isnan(hum)) {
    tempDHT = temp;
    humedad = hum;
  } else {
    Serial.println("Lectura fallida en el sensor DHT22.");
  }

  Serial.print("Temperatura DHT22: ");
  Serial.print(tempDHT);
  Serial.println(" ºC.");

  Serial.print("Humedad DHT22: ");
  Serial.print(humedad);
  Serial.println(" %.");

  if (tempDHT > TEMP_THRESHOLD || humedad > HUM_THRESHOLD) {
    activarAlarma();  
  } else {
    detenerAlarma();  
  }
}

void leerBMP280() {
  tempBMP = bme.readTemperature();
  presion = bme.readPressure() / 100.0F; 

  Serial.print(F("Temperatura BMP280: "));
  Serial.print(tempBMP);
  Serial.println(F(" °C"));

  Serial.print(F("Presión BMP280: "));
  Serial.print(presion);
  Serial.println(F(" hPa"));
}

void actualizarOLED() {
  display.clearDisplay();

  display.setCursor(0, 0);
  display.print("T: "); display.print(tempDHT); display.println("C");
  display.setCursor(0, 20);
  display.print("H: "); display.print(humedad); display.println("%");
  display.setCursor(0, 40);
  display.print("P: "); display.print(presion); display.println("hPa");

  display.display();
}

void enviarAThingSpeak() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi desconectado, intentando reconectar...");
    conectarWiFi();
  }

  ThingSpeak.setField(1, tempDHT);
  ThingSpeak.setField(2, humedad);
  ThingSpeak.setField(3, presion);

  int respuesta = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
  if (respuesta == 200) {
    Serial.println("Datos enviados a ThingSpeak correctamente.");
  } else {
    Serial.println("Error al enviar datos a ThingSpeak. Código HTTP: " + String(respuesta));
  }
}

void activarAlarma() {
  Serial.println("¡ALERTA! Temperatura o Humedad fuera de rango.");
  digitalWrite(buzzerPin, HIGH);  
}

void detenerAlarma() {
  digitalWrite(buzzerPin, LOW);  
}
