#include "packet.h"
#include "controlSum.h"
#include <GyverBME280.h>
#include <DHT.h>
#include <MQUnifiedsensor.h>

#include <SPI.h>        // Интерфейс SPI для работы с радиомодулем nRF24L01
#include <RF24.h>       // Библиотека для работы с радиомодулем nRF24L01
#include <nRF24L01.h>

// Определение пинов для подключения датчиков и радиомодуля
#define PIN_DHT 2     // Пин для подключения датчика DHT11
#define PIN_SOUND 3   // Пин для подключения датчика звука
#define PIN_LIGHT 4   // Пин для подключения датчика света
#define PIN_MQ135 A0  // Пин для подключения датчика MQ135
#define PIN_WATER A1  // Пин для подключения датчика уровня воды
#define PIN_POWER 5   // Пин для подачи питания на датчик уровня воды
#define PIN_CE 10     // Пин управления чипом радиомодуля (Chip Enable)
#define PIN_CS 9      // Пин выбора чипа радиомодуля по SPI (Chip Select)

// Инициализация объектов датчиков
GyverBME280 bme;
DHT dht(PIN_DHT, DHT11);
MQUnifiedsensor MQ135("Arduino Nano", 5, 10, PIN_MQ135, "MQ-135");

// Настройка радиомодуля nRF24L01
RF24 radio(PIN_CE, PIN_CS);
byte address[6] = "1Node"; // Адрес для передачи данных

unsigned char readWaterSensor();
bool readSoundSensor();
bool readLightSensor();
void readAllSensors(DataPacket& data);
void fillDataPacket(DataPacket& data);
void printData(DataPacket data);

void setup()
{
    // Инициализация последовательного порта для отладки
    Serial.begin(115200);

    // Конфигурация режимов работы пинов
    pinMode(PIN_DHT, INPUT);
    pinMode(PIN_MQ135, INPUT);
    pinMode(PIN_SOUND, INPUT);
    pinMode(PIN_LIGHT, INPUT);
    pinMode(PIN_WATER, INPUT);
    pinMode(PIN_POWER, OUTPUT);

    // Настройка и начало работы датчиков
    bme.begin();
    dht.begin();
    MQ135.setRL(2);
    MQ135.setR0(9.65);
    MQ135.init();

    // Настройка радиомодуля nRF24L01
    radio.begin();                    // Активация радиомодуля
    radio.setAutoAck(true);           // Включение автоматического подтверждения приема данных
    radio.openWritingPipe(address);   // Открытие канала для передачи данных
    radio.setPALevel(RF24_PA_MAX);    // Установка мощности передатчика (максимальная)
    radio.setDataRate(RF24_1MBPS);    // Установка скросоти передачи данных (1 Мбит/с)
    radio.powerUp();                  // Включение питания радиомодуля
    radio.stopListening();            // Перевод радиомодуля в режим передатчика

    delay(1000);
}

void loop()
{
    // Создание экземпляра структуры пакета данных для передачи
    DataPacket data;

    // Заполнение пакета данных
    fillDataPacket(data);

    // Отправка пакета данных по радиоканалу
    radio.write(&data, sizeof(data));

    // Дублирование отправленных данных в монитор последовательного порта для отладки
    printData(data);

    // Интервал между измерениями
    delay(1000);
}

// Функция чтения с датчика уровня воды
unsigned char readWaterSensor()
{
    digitalWrite(PIN_POWER, HIGH);
    delay(10);
    int val = analogRead(PIN_WATER);
    digitalWrite(PIN_POWER, LOW);
    return (unsigned char)(map(val, 10, 700, 0, 100));
}

// Функция чтения с датчика звука
bool readSoundSensor()
{
    return digitalRead(PIN_SOUND);
}

// Функция чтения с датчика света
bool readLightSensor()
{
    return !digitalRead(PIN_LIGHT);
}

// Функция чтения чтения со всех датчиков с заполнение полей пакета данных
void readAllSensors(DataPacket& data)
{
    data.temperature = bme.readTemperature();
    data.pressure = bme.readPressure();
    data.humidity = dht.readHumidity();

    MQ135.update();
    MQ135.setA(110.47);
    MQ135.setB(-2.862);
    data.gasConcentration = MQ135.readSensor();

    data.waterLevel = readWaterSensor();
    data.isLight = readLightSensor();
    data.isNoisy = readSoundSensor();
}

// Функция заполнения пакета данных
void fillDataPacket(DataPacket& data)
{
    readAllSensors(data);
    data.controlSum = countCrc((unsigned char*)&data, sizeof(data) - sizeof(data.controlSum));
}

// Функция вывода данных в монитор последовательного порта
void printData(DataPacket data)
{
    Serial.println(F("\n--- DATA PACKET ---"));
    
    Serial.print(F("Sync Mark:\t0x")); 
    Serial.println(data.syncMark, HEX); 

    Serial.print(F("Id:\t")); 
    Serial.println(data.id); 

    Serial.print(F("Temperature:\t")); 
    Serial.print(data.temperature); 
    Serial.println(F(" *C"));
    
    Serial.print(F("Pressure:\t")); 
    Serial.print(data.pressure); 
    Serial.println(F(" Pa"));
    
    Serial.print(F("Humidity:\t")); 
    Serial.print(data.humidity); 
    Serial.println(F(" %"));
    
    Serial.print(F("Gas Conc:\t")); 
    Serial.print(data.gasConcentration); 
    Serial.println(F(" ppm"));
    
    Serial.print(F("Water Level:\t")); 
    Serial.print(data.waterLevel); 
    Serial.println(F(" %"));
    
    Serial.print(F("Light State:\t")); 
    Serial.println(data.isLight ? F("LIGHT") : F("DARK"));
    
    Serial.print(F("Sound State:\t")); 
    Serial.println(data.isNoisy ? F("NOISY") : F("QUIET"));
    
    Serial.print(F("Control Sum:\t0x")); 
    Serial.println(data.controlSum, HEX);
    Serial.println(F("-------------------"));
}