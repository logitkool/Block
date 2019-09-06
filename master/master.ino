#include <Arduino.h>
#include <Packetizer.h>
// #include <SoftwareSerial.h>
#include <HardwareSerial.h>

#include "block.hpp" // ブロック
#include "commands.hpp" // コマンド

// floc master
// for Arduino UNO

// 設定
const unsigned long BAUDRATE = 19200;
const int Rx_PIN = 3;
const int Tx_PIN = 4;

// SoftwareSerial comBlock(Rx_PIN, Tx_PIN, true);
HardwareSerial comBlock(2); // Rx=GPIO16, Tx=GPIO17

const unsigned int MAX_BLOCK = 32;
const unsigned int TIMEOUT = 500; // ms
const unsigned int RESENT_COUNT = 3;
const unsigned int INTERVAL = 1500; // ms

BlockId ids [MAX_BLOCK];
int _index = 0;
bool isScanning = false;
bool askSent = false;
int askCount = 0;
int lastSent = 0;

Packetizer::Packer packer;

void next()
{
  BlockId dest_id = ids[_index];

  writeData(COM_LED, dest_id.Uid_H, dest_id.Uid_L);
  
  _index++;
}

template<typename ...Param>
void writeData(int hex, Param& ...params)
{
  packer.init();
  
  pack(hex, params...);

  comBlock.write(packer.data(), packer.size());
  comBlock.flush();
  
  Serial.println("wrote.");
}

// #pragma region 可変コマンドをパッカーに追加
template<typename ...Param>
void pack(int hex, Param ...params)
{
  append(hex);
  pack(params...);
}

void pack()
{
  packer << Packetizer::endp();
}

void append(int hex)
{
  packer << hex;
}
// #pragma endregion

Packetizer::Unpacker unpacker;
void callback(const uint8_t* data, uint8_t size);

void setup()
{
  Serial.begin(BAUDRATE);
  comBlock.begin(BAUDRATE, SERIAL_8N1, -1, -1, true);

  for(unsigned int i = 0; i < MAX_BLOCK; i++)
  {
    ids[i].TypeId = 0xFF;
    ids[i].Uid_H = 0xFF;
    ids[i].Uid_L = 0xFF;
  }
  
  unpacker.subscribe(0x00, callback);

  Serial.println("initialized.");

}

void callback(const uint8_t* data, uint8_t size)
{
  Serial.print("incoming bytes : [");
  for(int i = 0; i < size; i++)
  {
    Serial.print(String(data[i], HEX));
    if (i < size - 1) Serial.print(", ");
  }
  Serial.println("]");

  if (size == 0) return;

  switch (data[0])
  {
  case COM_RET:
    {
      int i = 0;
      while(ids[i].TypeId != 0xFF) i++;
      if (i >= MAX_BLOCK) break;
      ids[i].TypeId = data[1];
      ids[i].Uid_H = data[2];
      ids[i].Uid_L = data[3];
      askSent = false;
      askCount = 0;
    }
    break;

  default:
      break;
  }

}

void loop()
{
  if (askCount > RESENT_COUNT)
  {
    if (isScanning)
    {
      Serial.println("Scan completed.");
    }
    isScanning = false;
  }
  if (isScanning && !askSent)
  {
    writeData(COM_ASK);
    askSent = true;
    askCount++;
    lastSent = millis();
  }
  if (millis() - lastSent >= TIMEOUT)
  {
    askSent = false;
  }

  while (const int size = comBlock.available())
  {
    uint8_t data[size];
    comBlock.readBytes(data, size);

    unpacker.feed(data, size);
  }

  while (const int size = Serial.available())
  {
    uint8_t data[size];
    Serial.readBytes(data, size);

    switch (data[0])
    {
      case 's':
      {
        Serial.println("Scanning...");
        isScanning = true;
      }
      break;

      case 'p':
      {
        for(unsigned int i = 0; i < MAX_BLOCK; i++)
        {
          if (ids[i].TypeId != 0xFF)
          {
            Serial.print("[tid=");
            Serial.print(String(ids[i].TypeId, HEX));
            Serial.print(", uid=");
            Serial.print(String(ids[i].Uid_H, HEX));
            Serial.print(String(ids[i].Uid_L, HEX));
            Serial.print("] -> ");
          }
        }
        Serial.println("");
        Serial.println("completed.");
      }
      break;

      case 'r':
      {
        Serial.println("Resetting...");
        for(unsigned int i = 0; i < MAX_BLOCK; i++)
        {
          ids[i].TypeId = 0xFF;
          ids[i].Uid_H = 0xFF;
          ids[i].Uid_L = 0xFF;
        }
        writeData(COM_RST);
        Serial.println("completed.");
      }
      break;

      case 'w':
      {
        writeData(COM_ASK);
      }
      break;
    
    default:
      Serial.print("unknown command: ");
      Serial.println(('a' <= data[0] && data[0] <= 'z') ? data[0] : ' ');
      break;
    }
  }

}
