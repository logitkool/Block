#include <Arduino.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>
#define __AVR__
#include <Packetizer.h>
#undef __AVR__

// floc slave
// for ATtiny85
// 汎用ファームウェア
// ※ID設定の為、EEPROMの書き換えが必須

// 設定
// const unsigned char TYPE_ID = 0x80; // 1byte (Max: 0xFF)
// const unsigned int SERIAL_NO = 0x0001; // 2byte (Max: 0xFFFF)

const unsigned int BAUDRATE = 38400;
const int Rx_PIN = 3; // 共通Rx
const int Tx_CON_PIN = 1; // 接続方向Tx
const int Tx_BRD_PIN = 2; // 接続方向以外のTx (BRD : BRoaDcast)
const int LED_PIN = 0;
const int LED2_PIN = 4;

SoftwareSerial comBrdSide(Rx_PIN, Tx_BRD_PIN, true);
SoftwareSerial comConSide(Rx_PIN, Tx_CON_PIN, true);

// state
uint8_t TYPE_ID = 0xFF;
uint8_t UID_L = 0xFF; // 下位1Byte
uint8_t UID_H = 0xFF; // 上位1Byte
bool idSent = false;
bool isTerminal = true; // 終端ブロックかどうか

Packetizer::Packer_<16> packer;
Packetizer::Unpacker_<2, 16> unpacker;
void callback(const uint8_t* data, uint8_t size);

void setup()
{
  comBrdSide.begin(BAUDRATE);

  TYPE_ID = EEPROM.read(0x00);
  UID_H = EEPROM.read(0x02);
  UID_L = EEPROM.read(0x03);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  pinMode(LED2_PIN, OUTPUT);
  digitalWrite(LED2_PIN, LOW);
  
  unpacker.subscribe(0x00, callback);

}

void sendToConSide(const uint8_t* data, uint8_t size)
{
  comBrdSide.end();
  comConSide.begin(BAUDRATE);
  comConSide.write(data, size);
  comConSide.end();
  comBrdSide.begin(BAUDRATE);
}

void callback(const uint8_t* data, uint8_t size)
{
    if (size == 0) return;
    digitalWrite(LED_PIN, HIGH);

    switch (data[0])
    {
    case 0xFF: // 尋ねるコマンド
        {
            // who are you?
            if (idSent)
            {
                delay(250);

                packer.init();
                packer << 0xFF << Packetizer::endp();
                sendToConSide(packer.data(), packer.size());
            } else
            {
                packer.init();
                packer << 0xFE << TYPE_ID << UID_H << UID_L << Packetizer::endp();
                comBrdSide.write(packer.data(), packer.size());
                idSent = true;
            }
        }

        break;
    case 0xFE:
        {
            // I'm ***
            if (size != 4) return;
            isTerminal = false;
            
            // return to child
            packer.init();
            packer << 0xFD << Packetizer::endp();
            sendToConSide(packer.data(), packer.size());
            // send child's TYPE_ID to parent
            packer.init();
            for(int i = 0; i < size; i++)
            {
              packer << data[i];
            }
            packer << Packetizer::endp();
            comBrdSide.write(packer.data(), packer.size());
        }
        break;

    default:
        break;
    }

}

void loop()
{
  if (TYPE_ID == 0xFF || UID_L == 0xFF || UID_H == 0xFF)
  {
    digitalWrite(LED2_PIN, HIGH);
    return;
  }

  while (const int size = comBrdSide.available())
  {
    // buffering
    char data[size];
    comBrdSide.readBytes(data, size);

    unpacker.feed(data, size);
    digitalWrite(LED_PIN, LOW);
  }

}
