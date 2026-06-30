#include "packet.h"
#include "controlSum.h"
#include <SPI.h>        // Интерфейс SPI для работы с радиомодулем nRF24L01
#include <RF24.h>       // Библиотека для работы с радиомодулем nRF24L01
#include <nRF24L01.h>

// Определение пинов для подключения радиомодуля
#define PIN_CE 10     // Пин управления чипом радиомодуля (Chip Enable)
#define PIN_CS 9      // Пин выбора чипа радиомодуля по SPI (Chip Select)

// Настройка радиомодуля nRF24L01
RF24 radio(PIN_CE, PIN_CS);
byte address[6] = "1Node"; // Адрес для получения данных

// Буфер под входящий пакет данных
DataPacket data;

// Текущий режим передачи данных по последовательному порту
Command currentMode = CMD_BIN;

String formNmeaString(DataPacket data);
bool checkData(DataPacket data);
void sendBin(DataPacket data);
void sendNmea(DataPacket data);
void sendCommandEcho(CommandPacket commandPacket);
void checkSerialCommands();

void setup()
{
    // Инициализация последовательного порта
    Serial.begin(115200);
    delay(100);
    Serial.print("$START\r\n");         // Отправка команды начала работы

    // Настройка радиомодуля nRF24L01
    radio.begin();                      // Активация радиомодуля
    radio.setAutoAck(true);             // Включение автоматического подтверждения приема данных
    radio.openReadingPipe(1, address);  // Открытие канала на чтение
    radio.setPALevel(RF24_PA_MAX);      // Установка мощности передатчика (максимальная)
    radio.setDataRate(RF24_1MBPS);      // Установка скросоти передачи данных (1 Мбит/с)
    radio.powerUp();                    // Включение питания радиомодуля
    radio.startListening();             // Перевод радиомодуля в режим приемника

    delay(1000);                        // Задержка для стабилизации работы радиомодуля
}

void loop()
{
    // Проверка на команды по смене режима передачи данных
    checkSerialCommands();

    // Проверка на появление данных
    if (radio.available())
    {
        // Чтение пришедших данных
        radio.read(&data, sizeof(data));

        // Проверка целостности полученных данных
        if (checkData(data))
        {
            // Передача данных по последовательному порту в зависимотси от режима
            if (currentMode == CMD_BIN)
            {
                sendBin(data);
            }
            else if (currentMode == CMD_NMEA)
            {
                sendNmea(data);
            }
        }
        delay(10);
    }
}

// Функция формирования строки по протоколу NMEA 0183
String formNmeaString(DataPacket data)
{
    String body = "PMVS,";
    body += String(data.temperature, 2);
    body += ",";
    body += String(data.pressure, 2);
    body += ",";
    body += String(data.humidity, 2);
    body += ",";
    body += String(data.gasConcentration, 2);
    body += ",";
    body += String(data.waterLevel);
    body += ",";
    body += String(data.isLight ? 1 : 0);
    body += ",";
    body += String(data.isNoisy ? 1 : 0);

    unsigned char nmeaCrc = 0;
    for (unsigned char i = 0; i < body.length(); i++)
    {
        nmeaCrc ^= body[i];
    }

    String nmea = "$" + body + "*";
    if (nmeaCrc < 0x10)
    {
        nmea += "0";
    }
    nmea += String(nmeaCrc, HEX);
    nmea += "\r\n";

    return nmea;
}

// Функция проверки целостности пакета данных
bool checkData(DataPacket data)
{
    if (data.syncMark != 0xDADA)
    {
        return false;
    }

    if (data.id != DataType)
    {
        return false;
    }

    unsigned short calculatedCrc = countCrc((unsigned char*)&data, sizeof(data) - sizeof(data.controlSum));

    if (data.controlSum != calculatedCrc)
    {
        return false;
    }

    return true;
}

// Функция отправки пакета данных в бинарном формате
void sendBin(DataPacket data)
{
    Serial.write((unsigned char*)&data, sizeof(data));
}

// Функция отправки пакета данных в формате NMEA 0183
void sendNmea(DataPacket data)
{
    String nmea = formNmeaString(data);
    Serial.print(nmea);
}

// Функция отправки эхо-ответа об успешном приеме команды
void sendCommandEcho(CommandPacket command)
{
    command.controlSum = countCrc((unsigned char*)&command, sizeof(command) - sizeof(command.controlSum));
    Serial.write((unsigned char*)&command, sizeof(command));
}

// Функция приема пакетов команд о смене режима
void checkSerialCommands()
{
    // Проверка на достаточность накопленных данных
    if (Serial.available() >= sizeof(CommandPacket))
    {
        // Чтение пакета команды
        CommandPacket receivedCommand;
        Serial.readBytes((unsigned char*)&receivedCommand, sizeof(CommandPacket));

        // Проверка на корректоность синхрометки и идентификатора пакета
        if (receivedCommand.syncMark == 0xDADA && receivedCommand.id == CommandType)
        {
            unsigned short calculatedCrc = countCrc((unsigned char*)&receivedCommand, sizeof(receivedCommand) - sizeof(receivedCommand.controlSum));
            
            // Проверка на схождение контрольной суммы
            if (receivedCommand.controlSum == calculatedCrc)
            {
                // Проверка на известность команды
                if (receivedCommand.command == CMD_BIN || receivedCommand.command == CMD_NMEA)
                {
                    // Изменение режима вывода
                    currentMode = (Command)receivedCommand.command;

                    // Отправка подтверждения приема команды
                    sendCommandEcho(receivedCommand);
                }
            }
        }
    }
}