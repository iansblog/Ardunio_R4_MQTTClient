#include <NTPClient.h>
#include <WiFiS3.h>
#include "arduino_secrets.h" 

///////please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)

int wifiStatus = WL_IDLE_STATUS;
WiFiUDP Udp; // A UDP instance to let us send and receive packets over UDP
NTPClient timeClient(Udp);

#include "arduino_functions.h" 

long lastMQTTSubmittion;
long delayMQTTSubmittionBy = 60; // Submit to the MQTT server every X seconds.

long lastMQTTpoll;
long delayMQTTpoll = 30; // Poll the MQTT server every X seconds.

#include <DHT11.h>
DHT11 dht11(4);

#include <ArduinoMqttClient.h>
WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

const char broker[] = "iotserver.local";
int        port     = 1883;
const char topic[]  = "ardunioTesting/r4";

void setup(){
  Serial.begin(9600);
  while (!Serial);

  connectToWiFi();

  Serial.print("Attempting to connect to the MQTT broker: ");
  Serial.println(broker);

  if (!mqttClient.connect(broker, port)) {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClient.connectError());

    while (1);
  }

  Serial.println("You're connected to the MQTT broker!");
  Serial.println();


  Serial.println("\nStarting connection to time server...");
  timeClient.begin();
  timeClient.update();

  Serial.print("Unix time = ");
  Serial.println(currentEpochTime());


}

void loop(){
  //Keeping the MQTT server connection alive. 
  if( (lastMQTTpoll + delayMQTTpoll) <=  currentEpochTime() ){
      // call poll() regularly to allow the library to send MQTT keep alives which
      // avoids being disconnected by the broker
      Serial.println("Keepijng the MQTT connection alive.");
      mqttClient.poll();

      lastMQTTpoll = currentEpochTime();
  }

  //Do not flood the MQTT server, delay by the number of seconds set in the delayMQTTSubmittionBy.   
  //This will also protect the dht11 from being overused. 
  if( (lastMQTTSubmittion + delayMQTTSubmittionBy) <=  currentEpochTime() ){
    int temperature = 0;
    int humidity = 0;

    // Attempt to read the temperature and humidity values from the DHT11 sensor.
    int result = dht11.readTemperatureHumidity(temperature, humidity);

    // Check the results of the readings.
    // If the reading is successful, print the temperature and humidity values.
    // If there are errors, print the appropriate error messages.
    if (result == 0) {
        Serial.print("Temperature: ");
        Serial.print(temperature);
        Serial.print(" °C\tHumidity: ");
        Serial.print(humidity);
        Serial.println(" %");
        
        //Time to sent the MQTT Information. 
        Serial.println("Sending the MQTT informaiton ");
        
        // send message
        mqttClient.beginMessage(topic);
        String jSonMessage = "{\"type\": \"ArdunioR4\",  \"ID\": \"Office\", \"humidity\": \" "+ String(humidity) +"\",  \"temperatureC\": \""+ String(temperature)  +"\", \"EpochTime\": \""+ String(timeClient.getEpochTime()) +"\" }";
        mqttClient.print(jSonMessage);
        mqttClient.endMessage();

        //update the lastr sent time.
        lastMQTTSubmittion = currentEpochTime();
    } else {
        // Print error message based on the error code.
        Serial.println(DHT11::getErrorString(result));
    }
  }
  
  //delay by 1/2 second. 
  delay(500);
}
