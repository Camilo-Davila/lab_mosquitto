
#include <WiFi.h>
#include <TFT_eSPI.h>
#include <PubSubClient.h>

#include <Arduino.h>
#include <Wire.h>
#include "MAX30100_PulseOximeter.h"

#define REPORTING_PERIOD_MS 1000

// PulseOximeter is the higher level interface to the sensor
// it offers:
//  * beat detection reporting
//  * heart rate calculation
//  * SpO2 (oxidation level) calculation
PulseOximeter pox;
uint32_t tsLastReport = 0;

#define BUTTON_LEFT 0
// Update these with values suitable for your network.

const char *ssid = "Moto One Fusion";
const char *password = "12345566";
const char *mqtt_server = "192.168.233.179";

TFT_eSPI tft = TFT_eSPI();

float spO2 = 0;
char o2String[8];

float ritmo = 0;
char ritmoString[8];

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE (50)
char msg[MSG_BUFFER_SIZE];
int value = 0;

bool toggle = false;
char toggleString[25];

void isr()
{
  if (toggle)
  {
    Serial.println("on");
  }
  else
  {
    Serial.println("off");
  }
  toggle = !toggle;
}
void onBeatDetected()
{
  // Serial.println("Beat!");
}
void setup_wifi()
{

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to: ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char *topic, byte *message, unsigned int length)
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("..Message ");
  String messageTemp;

  for (int i = 0; i < length; i++)
  {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  if (String(topic) == "esp32/led1")
  {
    if (messageTemp == "1")
    {
      Serial.println("led1 on");
      tft.fillCircle(200, 40, 20, TFT_GREEN);
    }
    else if (messageTemp == "0")
    {
      Serial.println("led1 off");
      tft.fillCircle(200, 40, 20, TFT_DARKGREY);
    }
  }

  if (String(topic) == "esp32/led2")
  {
    if (messageTemp == "1")
    {
      Serial.println("led2 on");
      tft.fillCircle(200, 90, 20, TFT_YELLOW);
    }
    else if (messageTemp == "0")
    {
      Serial.println("led2 off");
      tft.fillCircle(200, 90, 20, TFT_DARKGREY);
    }
  }
}

void reconnect()
{
  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");

    // Attempt to connect
    if (client.connect("ESP32Client22"))
    {
      Serial.println("connected");

      // subscribe
      client.subscribe("esp32/led1");
      client.subscribe("esp32/led2");
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup()
{
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  attachInterrupt(BUTTON_LEFT, isr, RISING);

  tft.init();
  tft.fillScreen(TFT_BLACK);
  tft.setRotation(1);

  Serial.print("Initializing pulse oximeter..");

  // Initialize the PulseOximeter instance
  // Failures are generally due to an improper I2C wiring, missing power supply
  // or wrong target chip
  if (!pox.begin())
  {
    Serial.println("FAILED");
    for (;;)
      ;
  }
  else
  {
    Serial.println("SUCCESS");
  }
  pox.setOnBeatDetectedCallback(onBeatDetected);
}

void loop()
{

  if (!client.connected())
  {
    reconnect();
  }
  client.loop();

  pox.update();

  unsigned long now = millis();
  if (now - lastMsg > 2000)
  {
    lastMsg = now;
    //tft.fillScreen(TFT_BLACK);

    spO2 = pox.getSpO2();
    ritmo = pox.getHeartRate();

    dtostrf(pox.getSpO2(), 2, 0, o2String); // variable, numero de digitos, numero decimales, arreglo donde guardarlo
    // snprintf(msg, MSG_BUFFER_SIZE, "hello world #%ld", value);
    Serial.print("SpO2: ");
    Serial.println(o2String);
    client.publish("esp32/spo2", o2String);
    tft.setTextColor(TFT_SKYBLUE, TFT_BLACK);
    tft.drawString("SpO2 (%):", 5, 70, 2);
    tft.drawString(o2String, 50, 90, 6);

    dtostrf(pox.getHeartRate(), 2, 1, ritmoString); // variable, numero de digitos, numero decimales, arreglo donde guardarlo
    Serial.print("Ritmo: ");
    Serial.println(ritmoString);
    client.publish("esp32/ritmo", ritmoString);
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.drawString("Ritmo Cardíaco (bpm):", 5, 0, 2);
    tft.drawString(ritmoString, 30, 20, 6);

    sprintf(toggleString, "%d", toggle);
    Serial.print("Toggle: ");
    Serial.println(toggleString);
    client.publish("esp32/sw", toggleString);
  }
}

// Código simple de prueba sensor MAX30100 ***************************************//

/*
#include <Arduino.h>
#include <Wire.h>
#include "MAX30100_PulseOximeter.h"

#define REPORTING_PERIOD_MS 1000

// PulseOximeter is the higher level interface to the sensor
// it offers:
//  * beat detection reporting
//  * heart rate calculation
//  * SpO2 (oxidation level) calculation
PulseOximeter pox;

uint32_t tsLastReport = 0;

// Callback (registered below) fired when a pulse is detected
void onBeatDetected()
{
  Serial.println("Beat!");
}

void setup()
{
  Serial.begin(115200);

  Serial.print("Initializing pulse oximeter..");

  // Initialize the PulseOximeter instance
  // Failures are generally due to an improper I2C wiring, missing power supply
  // or wrong target chip
  if (!pox.begin())
  {
    Serial.println("FAILED");
    for (;;)
      ;
  }
  else
  {
    Serial.println("SUCCESS");
  }

  // The default current for the IR LED is 50mA and it could be changed
  //   by uncommenting the following line. Check MAX30100_Registers.h for all the
  //   available options.
  // pox.setIRLedCurrent(MAX30100_LED_CURR_7_6MA);

  // Register a callback for the beat detection
  pox.setOnBeatDetectedCallback(onBeatDetected);
}

void loop()
{
  // Make sure to call update as fast as possible
  pox.update();

  // Asynchronously dump heart rate and oxidation levels to the serial
  // For both, a value of 0 means "invalid"
  if (millis() - tsLastReport > REPORTING_PERIOD_MS)
  {
    Serial.print("Heart rate:");
    Serial.print(pox.getHeartRate());
    Serial.print("bpm / SpO2:");
    Serial.print(pox.getSpO2());
    Serial.println("%");

    tsLastReport = millis();
  }
}

*/
