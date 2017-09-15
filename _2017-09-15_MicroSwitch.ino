#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <Ticker.h>

ESP8266WiFiMulti WiFiMulti;
ADC_MODE(ADC_VCC);

#define VCC_ADJ   1.096

#define pushButton    2

Ticker tickerOSWatch;

#define OSWATCH_RESET_TIME 30
static unsigned long last_loop;

const char* ssid     = "FartNet";
const char* password = "1214406964";
const char* host = "192.168.178.139";
const uint16_t port = 80;

// for mac address
uint8_t MAC_array[6];
char MAC_char[18];

void ICACHE_RAM_ATTR osWatch(void) {
  unsigned long t = millis();
  unsigned long last_run = abs(t - last_loop);
  Serial.printf("Watchdog last run took: %d\n", last_run);
  if (last_run >= (OSWATCH_RESET_TIME * 1000)) {
    Serial.println();
    Serial.println("the watchdog kicks in!!!");
    // save the hit here to eeprom or to rtc memory if needed
    ESP.restart();  // normal reboot
    //ESP.reset();  // hard reset
  }
}

void setup() {
  Serial.begin(115200);

  last_loop = millis();
  tickerOSWatch.attach_ms(((OSWATCH_RESET_TIME / 3) * 1000), osWatch);

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

  pinMode(pushButton, INPUT);

  getMacAddress();
}

void loop() {
  Serial.println("Begin loop \n\n");
  if (WiFiMulti.run() == WL_CONNECTED) {

    String batteryLevel = readBatteryLevel();

    int buttonState = digitalRead(pushButton);
    String trapStatus = "false";
    if (buttonState == 1) {
      trapStatus = "true";
    } else {
      trapStatus = "false";
    }
    Serial.println(trapStatus);

    String url = "/mousetrap/temp/" + String(MAC_char) + "/" + String((int)buttonState) + "/" + batteryLevel;
    Serial.print("Requesting URL: ");
    Serial.println(url);

    sendHttpData(url);
  }

  last_loop = millis();
  delay(10000);
  //ESP.deepSleep(100000);
  //Serial.println("Start deep sleep");
  //ESP.deepSleep(120000000, WAKE_RF_DEFAULT);
  //Serial.println("\n\nEnd loop");
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

void getMacAddress() {
  WiFi.macAddress(MAC_array);
  for (int i = 0; i < sizeof(MAC_array); ++i) {
    sprintf(MAC_char, "%s%02x:", MAC_char, MAC_array[i]);
  }
}
