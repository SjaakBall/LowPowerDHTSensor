#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

ESP8266WiFiMulti WiFiMulti;
ADC_MODE(ADC_VCC);

#define VCC_ADJ   1.096

#define DHTPIN    4         // Pin which is connected to the DHT sensor.

#define DHTTYPE   DHT11     // DHT 11 

const char* ssid     = "FartNet";
const char* password = "*****";
const char* host = "192.168.178.139";
const uint16_t port = 80;

float humidity, temp_f;

DHT_Unified dht(DHTPIN, DHTTYPE);

// dht sensor delay
uint32_t delaySensorMS;

// for mac address
uint8_t MAC_array[6];
char MAC_char[18];

void setup() {
  Serial.begin(115200);
  delay(100);

  WiFiMulti.addAP(ssid, password);

  Serial.println();
  Serial.println();
  Serial.print("Wait for WiFi");

  byte maxwait = 20;

  while ((WiFiMulti.run() != WL_CONNECTED) && maxwait > 0) {
    Serial.print(".");
    delay(500);
    maxwait--;
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  delay(500);

  dht.begin();

  getMacAddress();
  getInitialSensorData();
}

void loop() {
  Serial.println("Begin loop \n\n");
  if (WiFiMulti.run() == WL_CONNECTED) {

    String batteryLevel = readBatteryLevel();

    readTemperature();

    String url = "/mousetrap/temp/" + String(MAC_char) + "/" + String((int)temp_f) + "/" + batteryLevel;
    Serial.print("Requesting URL: ");
    Serial.println(url);

    sendHttpData(url);
  }
  //  delay(10000);
  //ESP.deepSleep(100000);
  Serial.println("Start deep sleep");
  ESP.deepSleep(120000000, WAKE_RF_DEFAULT);
  Serial.println("\n\nEnd loop");
}

void sendHttpData(String url) {
  Serial.print("connecting to ");
  Serial.println(host);
  Serial.println(port);
  // Use WiFiClient class to create TCP connections
  WiFiClient client;

  if (!client.connect(host, port)) {
    Serial.println("connection failed");
    Serial.println("wait 5 sec...");
    delay(5000);
    return;
  }

  // This will send the request to the server
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n\r\n");
  delay(500);

  // Read all the lines of the reply from server and print them to Serial
  while (client.available()) {
    String line = client.readStringUntil('\r');
    Serial.print(line);
  }

  Serial.println();
  Serial.println("closing connection");
  client.stop();
}

String readBatteryLevel() {
  String batteryLevel = String((ESP.getVcc() * VCC_ADJ) / 1000);
  //webservice cannot cope with . so replace with dec
  batteryLevel.replace(".", "dec");
  return batteryLevel;
}

void readTemperature() {
  delay(delaySensorMS);

  // Get temperature event and print its value.
  sensors_event_t event;
  dht.temperature().getEvent(&event);
  if (isnan(event.temperature)) {
    Serial.println("Error reading temperature!");
  }
  else {
    temp_f = event.temperature;
    Serial.print("Temperature: ");
    Serial.print(temp_f);
    Serial.println(" *C");
  }
}

void getMacAddress() {
  WiFi.macAddress(MAC_array);
  for (int i = 0; i < sizeof(MAC_array); ++i) {
    sprintf(MAC_char, "%s%02x:", MAC_char, MAC_array[i]);
  }
}

void getInitialSensorData() {
  Serial.println("DHTxx Unified Sensor Example");
  // Print temperature sensor details.
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);
  Serial.println("------------------------------------");
  Serial.println("Temperature");
  Serial.print  ("Sensor:       "); Serial.println(sensor.name);
  Serial.print  ("Driver Ver:   "); Serial.println(sensor.version);
  Serial.print  ("Unique ID:    "); Serial.println(sensor.sensor_id);
  Serial.print  ("Max Value:    "); Serial.print(sensor.max_value); Serial.println(" *C");
  Serial.print  ("Min Value:    "); Serial.print(sensor.min_value); Serial.println(" *C");
  Serial.print  ("Resolution:   "); Serial.print(sensor.resolution); Serial.println(" *C");
  Serial.println("------------------------------------");
  // Print humidity sensor details.
  dht.humidity().getSensor(&sensor);
  Serial.println("------------------------------------");
  Serial.println("Humidity");
  Serial.print  ("Sensor:       "); Serial.println(sensor.name);
  Serial.print  ("Driver Ver:   "); Serial.println(sensor.version);
  Serial.print  ("Unique ID:    "); Serial.println(sensor.sensor_id);
  Serial.print  ("Max Value:    "); Serial.print(sensor.max_value); Serial.println("%");
  Serial.print  ("Min Value:    "); Serial.print(sensor.min_value); Serial.println("%");
  Serial.print  ("Resolution:   "); Serial.print(sensor.resolution); Serial.println("%");
  Serial.println("------------------------------------");
  // Set delay between sensor readings based on sensor details.
  delaySensorMS = sensor.min_delay / 1000;
}
