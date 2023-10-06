/*********
  Rui Santos
  Complete instructions at https://RandomNerdTutorials.com/esp32-ble-server-client/
  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files.
  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
*********/

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Wire.h>
#include <EEPROM.h>
#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>

// Default Temperature is in Celsius
// Comment the next line for Temperature in Fahrenheit
#define temperatureCelsius

// BLE server name
#define bleServerName "BME280_ESP32"
#define SEALEVELPRESSURE_HPA (1013.25)
#define RECV_PIN 14
#define IR_LED 19
// IRrecv irrecv(RECV_PIN);
IRsend irsend(IR_LED);

float temp;
float tempF;
float hum;
const int LED = 19;

// Timer variables
unsigned long lastTime = 0;
unsigned long timerDelay = 1000;

bool deviceConnected = false;

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/
#define SERVICE_UUID "91bad492-b950-4226-aa2b-4ede9fa42f59"

// Setup callbacks onConnect and onDisconnect
class MyServerCallbacks : public BLEServerCallbacks
{
    void onConnect(BLEServer *pServer)
    {
        deviceConnected = true;
        digitalWrite(LED, HIGH);
    };
    void onDisconnect(BLEServer *pServer)
    {
        deviceConnected = false;
        pServer->getAdvertising()->start();
        digitalWrite(LED, LOW);
    }
};

class IRCallbacks : public BLECharacteristicCallbacks
{
    void onWrite(BLECharacteristic *characteristic)
    {
        // retorna ponteiro para o registrador contendo o valor atual da caracteristica
        std::string rxValue = characteristic->getValue();
        // verifica se existe dados (tamanho maior que zero)
        if (rxValue.length() > 0)
        {
            uint16_t signal_data[100];
            int current_data_length = 0;
            int ns = 0;
            for (unsigned int i = 0; i <= rxValue.length(); i++)
            {
                if (i == rxValue.length() || rxValue.at(i) == 32)
                {
                    String sub = "";
                    for (int f = ns; f <= i; f++)
                        sub = sub + rxValue[f];
                    ns = i + 1;
                    signal_data[current_data_length] = sub.toInt();
                    current_data_length++;
                }
            }
            irsend.sendRaw(signal_data, current_data_length, 38);
        }
    } // onWrite
};

class VolumeCallbacks : public BLECharacteristicCallbacks
{
    void onWrite(BLECharacteristic *characteristic)
    {
        // retorna ponteiro para o registrador contendo o valor atual da caracteristica
        std::string rxValue = characteristic->getValue();
        // verifica se existe dados (tamanho maior que zero)
        if (rxValue.length() > 0)
        {

            for (int i = 0; i < rxValue.length(); i++)
            {
                Serial.print(rxValue[i]);
            }
            Serial.println();
            // L1 liga o LED | L0 desliga o LED
            if (rxValue.find("L1") != -1)
            {
                // subsitituri para comando I2C
                digitalWrite(LED, HIGH);
            }
            else if (rxValue.find("L0") != -1)
            {
                digitalWrite(LED, LOW);
            }
            else if (rxValue.find("volup") != -1)
            {
                Serial.print("entrou up");
                int volume = EEPROM.read(0);
                Serial.println(volume);
                volume--;
                if (volume > 64)
                    volume = 63;
                if (volume < 0)
                {
                    Serial.println(volume);
                    return;
                }
                Wire.beginTransmission(0x28);
                Wire.write(0xAF);
                Wire.write(int(volume));
                Wire.endTransmission();
                EEPROM.write(0, volume);
                EEPROM.commit();
                char volumeChar[3] = {char(volume)};
            }
            else if (rxValue.find("voldown") != -1)
            {
                Serial.print("entrou down");
                // Aqui tenho que pegar o valor da eeprom e somar 1 e depois salvar a soma
                int volume = EEPROM.read(0);
                Serial.println(volume);
                volume++;
                if (volume > 64)
                {
                    Serial.println(volume);
                    return;
                }
                Wire.beginTransmission(0x28);
                Wire.write(0xAF);
                Wire.write(int(volume));
                Wire.endTransmission();
                EEPROM.write(0, volume);
                EEPROM.commit();
                char volumeChar[3] = {char(volume)};
            }
        }
    } // onWrite
};

void setup()
{
    // Start serial communication
    Serial.begin(9600);
    Wire.begin(21, 22);
    EEPROM.begin(32);
    pinMode(LED, OUTPUT);
    // Create the BLE Device
    BLEDevice::init(bleServerName);

    // Create the BLE Server
    BLEServer *pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());
    // Create the BLE Service
    BLEService *bmpService = pServer->createService(SERVICE_UUID);

    BLECharacteristic *IRCharacteristic = bmpService->createCharacteristic("7de27b1b-1a3b-4f7b-b712-e9a630e165da", BLECharacteristic::PROPERTY_WRITE);
    IRCharacteristic->setCallbacks(new IRCallbacks());

    BLECharacteristic *VolumeCharacteristic = bmpService->createCharacteristic("a9a76a4e-92dd-4150-939d-a9d9f31c5c2e", BLECharacteristic::PROPERTY_WRITE);
    VolumeCharacteristic->setCallbacks(new VolumeCallbacks());
    // Start the service
    bmpService->start();

    // Start advertising
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pServer->getAdvertising()->start();
    Serial.println("Waiting a client connection to notify...");

    // iniciando potenciometro com o valor que esta na eeprom
    Wire.beginTransmission(0x28);
    Wire.write(0xBD); // ativando zero crossing
    Wire.endTransmission();
    Wire.beginTransmission(0x28);
    Wire.write(0xAF);
    Wire.write(int8_t(EEPROM.read(0)));
    Wire.endTransmission();
}

void loop()
{
    if (deviceConnected)
    {
    }
}
