#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>


// ======================================================
// CONFIGURACIÓN WIFI
// ======================================================

const char* WIFI_SSID = "RedmiFarit";
const char* WIFI_PASSWORD = "Contraseña";


// ======================================================
// CONFIGURACIÓN MQTT
// ======================================================


const char* MQTT_SERVER = "10.13.98.123";
const int MQTT_PORT = 1883;

// Topics utilizados por el dashboard React.
const char* TOPIC_TEMPERATURA = "clima/esp32/temperatura";
const char* TOPIC_HUMEDAD = "clima/esp32/humedad";
const char* TOPIC_LUZ = "clima/esp32/luz";


// ======================================================
// CONFIGURACIÓN DE SENSORES
// ======================================================

// (Estudiante)
// Definir los pines y objetos necesarios para los sensores.
#define DHTPIN 14
#define LDRPIN 33

// Tipo de sensor DHT
#define DHTTYPE DHT11

// Inicializar el sensor DHT
DHT dht(DHTPIN, DHTTYPE);

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// Enviar una lectura cada 2 segundos.
const unsigned long INTERVALO_ENVIO = 2000;

unsigned long ultimoEnvio = 0;

void conectarWiFi() {

  Serial.print("Conectando a WiFi");

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi conectado");

  Serial.print("IP del ESP32: ");
  Serial.println(WiFi.localIP());
}

void conectarMQTT() {

  while (!mqttClient.connected()) {

    Serial.print("Conectando a Mosquitto...");

    // Crear un Client ID diferente para este ESP32.
    String clientId = "ESP32-";
    clientId += String(
      (uint32_t)(ESP.getEfuseMac() & 0xFFFFFFFF),
      HEX
    );

    if (mqttClient.connect(clientId.c_str())) {

      Serial.println(" conectado");

    } else {

      Serial.print(" error, rc=");
      Serial.print(mqttClient.state());
      Serial.println(". Reintentando en 2 segundos...");

      delay(2000);
    }
  }
}


// ======================================================
// LEER Y PUBLICAR SENSORES
// ======================================================

void publicarSensores() {

   /*
    (Estudiante)
      1. Obtener la temperatura.

      2. Obtener la humedad.

      3. Obtener la lectura del sensor de iluminación.

      4. Preparar los valores para poder publicarlos
         mediante MQTT.

      5. Publicar cada dato en su topic correspondiente:

         TOPIC_TEMPERATURA
         TOPIC_HUMEDAD
         TOPIC_LUZ

      6. Mostrar las mediciones en el Monitor Serial.
  */

  //Leer al valor del LDR
  int valorLDR = analogRead(LDRPIN);

  //Leer el valor del DHT11
  float humedad = dht.readHumidity();
  float temperatura = dht.readTemperature();

  if(isnan(humedad) || isnan(temperatura)){
    Serial.println("Error al leer el sensor DHT11!");
    return;
  }

  int porcentajeLuz = map(valorLDR, 0, 4095, 0, 100);

  String tempStr = String(temperatura, 2);
  String humStr = String(humedad, 2);
  String luzStr = String(porcentajeLuz);

  mqttClient.publish(TOPIC_TEMPERATURA, tempStr.c_str());
  mqttClient.publish(TOPIC_HUMEDAD, humStr.c_str());
  mqttClient.publish(TOPIC_LUZ, luzStr.c_str());  

  Serial.print("Temperatura: ");
  Serial.print(temperatura);
  Serial.println(" °C");
  
  Serial.print("Humedad: ");
  Serial.print(humedad);
  Serial.println(" %");
  
  Serial.print("LDR ADC: ");
  Serial.print(valorLDR);
  Serial.print(" | Luz: ");
  Serial.print(porcentajeLuz);
  Serial.println(" %");
  Serial.println("------------------------------");

}

void setup() {

  Serial.begin(115200);

  dht.begin();
  
  // (Estudiante)
  // Inicializar los sensores y configurar los pines
  // necesarios.

  conectarWiFi();

  mqttClient.setServer(
    MQTT_SERVER,
    MQTT_PORT
  );
}

void loop() {

  // Si se pierde WiFi, volver a conectarse.
  if (WiFi.status() != WL_CONNECTED) {
    conectarWiFi();
  }

  // Si se pierde MQTT, volver a conectarse.
  if (!mqttClient.connected()) {
    conectarMQTT();
  }

  mqttClient.loop();


  // Publicar sensores cada 2 segundos.
  if (millis() - ultimoEnvio >= INTERVALO_ENVIO) {

    ultimoEnvio = millis();

    publicarSensores();
  }
}

