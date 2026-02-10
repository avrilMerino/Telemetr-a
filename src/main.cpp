#include <WiFi.h>
#include <HTTPClient.h>

//wifi
const char* WIFI_SSID = "Wokwi-GUEST";
const char* WIFI_PASS = "";

//THINGSPEAK 
const char* THINGSPEAK_WRITE_KEY = "ZBHRH9ILWVL14KNL";
const char* THINGSPEAK_URL = "https://api.thingspeak.com/update?api_key=ZBHRH9ILWVL14KNL&field1=0";

//pines
const int LED_PIN = 14;
const int BUTTON_PIN = 12;  
const int TEMP_ADC_PIN = 35;

//estado
bool ledState = false;
int pulsaciones = 0;

//antirrebote
bool lastReading = HIGH;
bool stableState = HIGH;
unsigned long lastChangeMs = 0;
const unsigned long DEBOUNCE_MS = 40;

//envío ThingSpeak (MÍNIMO ~15s)
unsigned long lastSendMs = 0;
const unsigned long SEND_EVERY_MS = 20000;

//WIFI CONNECT
void wifiConnect() {
  Serial.print("Conectando a WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("WiFi OK. IP: ");
  Serial.println(WiFi.localIP());
}

//Lctura TEMP
float leerTempSimulada() {
   int raw = analogRead(TEMP_ADC_PIN);
  float temp = (raw / 4095.0f) * 50.0f;
  return temp;
}

//Control boton
void controlBoton() {
  bool reading = digitalRead(BUTTON_PIN);

  if (reading != lastReading) {
    lastReading = reading;
    lastChangeMs = millis();
  }

  if (millis() - lastChangeMs > DEBOUNCE_MS) {
    if (reading != stableState) {
      stableState = reading;

      //detectar pulsación: flanco al pasar a LOW
      if (stableState == LOW) {
        pulsaciones++;
        ledState = !ledState;
        digitalWrite(LED_PIN, ledState ? HIGH : LOW);

        Serial.print("Pulsacion! contador=");
        Serial.print(pulsaciones);
        Serial.print(" LED=");
        Serial.println(ledState ? 1 : 0);
      }
    }
  }
}

//ENVÍO thingspeaker
bool enviarThingSpeak(float temp, int led, int count) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi caido. Reintentando...");
    wifiConnect();
  }

  HTTPClient http;

  String url = String(THINGSPEAK_URL) +
               "?api_key=" + THINGSPEAK_WRITE_KEY +
               "&field1=" + String(temp, 2) +
               "&field2=" + String(led) +
               "&field3=" + String(count);

  http.begin(url);
  int httpCode = http.GET();
  String body = http.getString();
  http.end();

  if (httpCode == 200) {
    long entryId = body.toInt(); // el thingSpeak devuelve entry_id o 0 si falla
    if (entryId > 0) {
      Serial.print("Update OK entry_id=");
      Serial.print(entryId);
      Serial.print(" | T=");
      Serial.print(temp, 2);
      Serial.print(" LED=");
      Serial.print(led);
      Serial.print(" C=");
      Serial.println(count);
      return true;
    } else {
      Serial.print("Update ERROR (rate limit o key mal). Respuesta: ");
      Serial.println(body);
      return false;
    }
  } else {
    Serial.print("HTTP ERROR: ");
    Serial.println(httpCode);
    Serial.print("Body: ");
    Serial.println(body);
    return false;
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  wifiConnect();

  Serial.println("Listo. Enviando telemetria a ThingSpeak...");
}

void loop() {
  controlBoton();

  unsigned long now = millis();
  if (now - lastSendMs >= SEND_EVERY_MS) {
    lastSendMs = now;

    float temp = leerTempSimulada();
    int led = ledState ? 1 : 0;

    enviarThingSpeak(temp, led, pulsaciones);
  }
}