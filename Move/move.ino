#include <Arduino.h>
#include <SoftwareSerial.h>
#define __AVR__
#include <Packetizer.h>
#undef __AVR__

#include "block.hpp"
#include "commands.hpp"

template<class T, size_t S>
constexpr size_t get_size(const T (&)[S])
{
  return S;
}

// ふろっく 動作ブロック
// for ATtiny85

// 設定
const Block::BlockId BLOCK_ID = { 0x80, 0x00, 0x01 }; // ブロックID

const unsigned long BAUDRATE = 19200;
const int Rx_PIN = 3; // 共通Rx
const int Tx_CON_PIN = 1; // 接続方向Tx
const int Tx_BRD_PIN = 2; // 接続方向以外のTx (BRD : BRoaDcast)
const int LED_PIN = 0;
const int LED2_PIN = 4;

SoftwareSerial comBrdSide(Rx_PIN, Tx_BRD_PIN, true);
SoftwareSerial comConSide(Rx_PIN, Tx_CON_PIN, true);

bool idSent = false;
bool isTerminal = true; // 終端ブロックかどうか
uint8_t prev_command = 0xFF;
const int MAX_ID = 4;
Block::BlockId known_ids[MAX_ID];

Packetizer::Packer_<16> packer;
Packetizer::Unpacker_<2, 16> unpacker;
void callback(const uint8_t* data, uint8_t size);

void setup()
{
  comBrdSide.begin(BAUDRATE);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  // pinMode(LED2_PIN, OUTPUT);
  // digitalWrite(LED2_PIN, LOW);

  for(unsigned int i = 0; i < MAX_ID; i++)
  {
    known_ids[i] = Block::None;
  }
  
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

  sendData(comConSide, data, size);

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

    switch (data[0])
    {
    case COM_ASK:
        {
            unsigned int idx = 0;
            while(idx < MAX_ID && known_ids[idx].Uid_H != 0xFF && known_ids[idx].Uid_H != 0xFF) idx++;
            if (idx == MAX_ID) break;

            known_ids[idx] = {0xFF, data[1], data[2]};

            bool isKnown = false;
            for(unsigned int i = 0; i < idx; i++)
            {
              if (known_ids[i].Uid_H == data[1] && known_ids[i].Uid_L == data[2])
              {
                isKnown = true;
              }
            }

            if (isKnown)
            {
              if (prev_command == COM_ASK) break;
              uint8_t coms[] = {COM_ASK, BLOCK_ID.Uid_H, BLOCK_ID.Uid_L };
              sendToConSide(coms, get_size(coms));
            } else
            {
              uint8_t coms[] = {COM_RET, data[1], data[2], BLOCK_ID.TypeId, BLOCK_ID.Uid_H, BLOCK_ID.Uid_L };
              sendToAll(coms, get_size(coms));
            }

            idSent = true;

        }

        break;
    case COM_RET:
        if (!idSent) break;

        {
            isTerminal = false;
            // digitalWrite(LED_PIN, HIGH);
            if(prev_command == COM_RET) break;
            sendToBrdSide(data, size);
        }
        break;

      case COM_RST:
      {
        if (!idSent) break;

        idSent = false;
        isTerminal = true;
        prev_command = 0xFF;
        // digitalWrite(LED_PIN, LOW);
        for(unsigned int i = 0; i < MAX_ID; i++)
        {
          known_ids[i] = Block::None;
        }

        for(int i = 0; i < 3; i++)
        {
          uint8_t coms[] = {COM_RST};
          sendToConSide(coms, get_size(coms));
        }

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
  }

  if (idSent || millis() - time > 500)
  {
    state = state == LOW ? HIGH : LOW;
    state = idSent ? HIGH : state;
    digitalWrite(LED_PIN, state);
    time = millis();
  }

}
