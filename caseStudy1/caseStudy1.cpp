#include <WiFi.h>
#include <Firebase.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "UbidotsESPMQTT.h"

#define WIFI_SSID "Manga"
#define WIFI_PASSWORD "123456789"

#define FIREBASE_TOKEN "skBlVoQR0a64WFTA69q1Jrd7rG1Ym2N5GGkmmn54"
#define FIREBASE_URL "https://trial-a-48c2d-default-rtdb.firebaseio.com/"
Firebase fb(FIREBASE_URL, FIREBASE_TOKEN);

#define UBIDOTS_TOKEN "BBUS-vcGeZVMWFOw7BPJlEPiHINgbV1PhNs"  
Ubidots ubidots(UBIDOTS_TOKEN);

#define SS_PIN 2
#define RST_PIN 5
MFRC522 rfid(SS_PIN, RST_PIN);

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

#define LED_BLUE 12
#define BUZZER_PIN 27
#define LED_RED 14


void showMessage(String msg) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 20);
    display.println(msg);
    Serial.println(msg);
    display.display();
}


void ubidotsCallback(char* topic, byte* payload, unsigned int length) {
    Serial.print("Ubidots message arrived [");
    Serial.print(topic);
    Serial.print("]: ");
    for (int i = 0; i < length; i++) {
        Serial.print((char)payload[i]);
    }
    Serial.println();
}

String getUID() {
    String uid = "";
    for (byte i = 0; i < rfid.uid.size; i++) {
        uid += String(rfid.uid.uidByte[i]);
    }
    uid.toUpperCase();
    return uid;
}

void grantAccess(String uid) {
    showMessage("Access Granted");
    digitalWrite(LED_BLUE, HIGH);
    digitalWrite(BUZZER_PIN, HIGH);
    delay(1000);
    digitalWrite(BUZZER_PIN, LOW);

    fb.pushString("logs", "Access Granted: " + uid);

    ubidots.add("access", 1);
    ubidots.add("uid", uid.toInt());
    ubidots.ubidotsPublish("smart");

    delay(2000);
    digitalWrite(LED_BLUE, LOW);
}

void denyAccess(String uid) {
    showMessage("Access Dended");
    digitalWrite(LED_RED, HIGH);
    digitalWrite(BUZZER_PIN, HIGH);


    fb.pushString("logs", "Access Dended: " + uid);

    ubidots.add("access", 0);
    ubidots.add("uid", uid.toDouble());
    ubidots.ubidotsPublish("smart");

    delay(2000);
    digitalWrite(LED_RED, LOW);
    digitalWrite(BUZZER_PIN, LOW);
}

void setup() {
    Serial.begin(115200);

    SPI.begin();
    rfid.PCD_Init();
    pinMode(LED_BLUE, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(LED_RED, OUTPUT);

    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    display.clearDisplay();
    display.display();

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    showMessage("Connect to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connect!");
    showMessage("WiFi Connected!");
    ubidots.setDebug(true);
    ubidots.wifiConnection(WIFI_SSID, WIFI_PASSWORD);
    ubidots.begin(ubidotsCallback);
}

void loop() {
    if (!ubidots.connected()) {
        ubidots.reconnect();
    }
    ubidots.loop();

    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
        String uid = getUID();
        Serial.println("Scan UID: " + uid);
        showMessage("Check UID...");

        String path = "auth/" + uid;
        int result = fb.getInt(path);
        Serial.println("result: " + String(result));

        if (result == 1) {
            grantAccess(uid);
        }
        else {
            denyAccess(uid);
        }

        delay(2000);
        rfid.PICC_HaltA();
    }

}
