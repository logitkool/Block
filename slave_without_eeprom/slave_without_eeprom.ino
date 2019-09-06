#include <Arduino.h>
#include <SoftwareSerial.h>
#define __AVR__
#include <Packetizer.h>
#undef __AVR__

#include "block.hpp"
#include "../commands.hpp"

template<class T, size_t S>
constexpr size_t get_size(const T (&)[S])
{
  return S;
}

// floc slave
// for ATtiny85
// 汎用ファームウェア

// 設定
const BlockId BLOCK_ID = { 0x80, 0x00, 0x01 }; // ブロックID

// const unsigned char TYPE_ID = 0x80; // 1byte (Max: 0xFF)
// UID: 2byte (Max: 0xFFFF)
// const unsigned int UID_H = 0x0000;
// const unsigned int UID_L = 0x0001;

// const unsigned int BAUDRATE = 115200;
const unsigned long BAUDRATE = 19200;
const int Rx_PIN = 3; // 共通Rx
const int Tx_CON_PIN = 1; // 接続方向Tx
const int Tx_BRD_PIN = 2; // 接続方向以外のTx (BRD : BRoaDcast)
const int LED_PIN = 0;
const int LED2_PIN = 4;

SoftwareSerial comBrdSide(Rx_PIN, Tx_BRD_PIN, true);
SoftwareSerial comConSide(Rx_PIN, Tx_CON_PIN, true);

// state
bool idSent = false;
bool isTerminal = true; // 終端ブロックかどうか
uint8_t prev_command = 0xFF;
const int MAX_ID = 4;
BlockId known_ids[MAX_ID] = {BlockId::None, BlockId::None, BlockId::None, BlockId::None};

Packetizer::Packer_<16> packer;
Packetizer::Unpacker_<2, 16> unpacker;
void callback(const uint8_t* data, uint8_t size);

void setup()
{
  comBrdSide.begin(BAUDRATE);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  pinMode(LED2_PIN, OUTPUT);
  digitalWrite(LED2_PIN, LOW);
  
  unpacker.subscribe(0x00, callback);

}

void sendData(const SoftwareSerial& com, const uint8_t* data, size_t size)
{
  packer.init();

  for(auto i = 0;i < size;i++)
  {
    packer << data[i];
  }
  packer << Packetizer::endp();

  com.write(packer.data(), packer.size());
  prev_command = data[0];
}

void sendToConSide(const uint8_t* data, size_t size)
{
  comBrdSide.end();
  comConSide.begin(BAUDRATE);

  sendData(conConSide, data, size);

  comConSide.end();
  comBrdSide.begin(BAUDRATE);
}

void sendToBrdSide(const uint8_t* data, size_t size)
{
  sendData(comBrdSide, data, size);
}

void sendToAll(const uint8_t* data, size_t size)
{
  sendToBrdSide(data, size);
  sendToConSide(data, size);
}

void callback(const uint8_t* data, uint8_t size)
{
    if (size == 0) return;
    // digitalWrite(LED_PIN, HIGH);

    switch (data[0])
    {
    case COM_ASK:
        {
            int idx = 0;
            while(known_ids[idx].Uid_H != 0xFF && known_ids[idx].Uid_H != 0xFF) idx++;
            if (idx > 4) break;
            
            // リストが空の場合
            if(idx == 0)
            {
              uint8_t coms[] = {COM_RET, data[1] ,BLOCK_ID.TypeId, BLOCK_ID.Uid_H, BLOCK_ID.Uid_L };
              sendToAll(coms, get_size(coms));
            } else
            {
              // TODO: 09/03 known_idsに送られてきたidがあるか判定し、知らなかったらsendToAll、知っていて1回目のCOM_ASKならsendToConSide、2回目ならbreak
            }
            idSent = true;
            
            // who are you?
            if (idSent)
            {
                // delay(250);
                uint8_t coms[] = {COM_ASK};
                sendToConSide(coms, get_size(coms));
            } else
            {
              // delay(250);
                idSent = true;

                uint8_t coms[] = {COM_RET, BLOCK_ID.TypeId, BLOCK_ID.Uid_H, BLOCK_ID.Uid_L };
                sendToBrdSide(coms, get_size(coms));
            }
        }

        break;
    case COM_RET:
        if (!idSent) break;

        {
            // I'm ***
            // if (size != 4) return;
            isTerminal = false;

            if(prev_command == COM_RET) break;

            // if (data[1] == latest.TypeId && data[2] == latest.Uid_H && data[3] == latest.Uid_L)
            // {
            //   break;
            // }
            // latest = { data[1], data[2], data[3] };
            
            // return to child
            // packer.init();
            // packer << 0xFD << Packetizer::endp();
            // sendToConSide(packer.data(), packer.size());
            // send child's TYPE_ID to parent
            sendToBrdSide(data, size);
        }
        break;

      case COM_RST:
      {
        idSent = false;
        isTerminal = true;

        uint8_t coms[] = {COM_RST};
        sendToConSide(coms, get_size(coms));
      }
      break;

    default:
        break;
    }

}

long time = millis();
int state = LOW;

void loop()
{
  while (const int size = comBrdSide.available())
  {
    // buffering
    char data[size];
    comBrdSide.readBytes(data, size);

    unpacker.feed(data, size);
    digitalWrite(LED_PIN, LOW);
  }

  if (idSent || millis() - time > 500)
  {
    state = state == LOW ? HIGH : LOW;
    state = idSent ? HIGH : state;
    digitalWrite(LED2_PIN, state);
    time = millis();
  }

}
