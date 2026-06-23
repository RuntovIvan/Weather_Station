#pragma once

#pragma once
enum Command : unsigned char
{
    CMD_BIN = 1,    // команда на передачу данных в бинарном формате
    CMD_NMEA = 2    // команда на передачу данных в формате NMEA
};

enum PacketType : unsigned char
{
    DataType = 1,
    CommandType = 2
};

#pragma pack(push, 1)
struct DataPacket
{
    unsigned short syncMark = 0xDADA;   // синхрометка
    unsigned char id = DataType;        // идентификатор пакета
    float temperature;                  // температура в *C (датчик BMP280)
    float pressure;                     // давление в Па (датчик BMP280)
    float humidity;                     // относительная влажность в % (датчик DHT11)
    float gasConcentration;             // концентрация CO2 в ppm (датчик MQ135)
    unsigned char waterLevel;           // уровень воды в % (датчик уровня воды)
    bool isLight;                       // освещенность светло(true)/темно(false) (датчик уровня света)
    bool isNoisy;                       // уровень шума шумно(true)/тихо(false) (датчик уровня шума)
    unsigned short controlSum;          // контрольная сумма 
};

struct CommandPacket
{
    unsigned short syncMark = 0xDADA;   // синхрометка
    unsigned char id = CommandType;     // идентификатор пакета
    unsigned char command;              // команда
    unsigned short controlSum;          // контрольная сумма
};
#pragma pack(pop)