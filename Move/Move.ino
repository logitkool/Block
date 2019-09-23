#include <Arduino.h>

#include "block.hpp"
#include "commands.hpp"
#include "comm.hpp"

// ふろっく 動作ブロック
// for ATtiny85

// 設定
const Block::BlockId BLOCK_ID = { 0x80, 0x00, 0x01 }; // ブロックID

const unsigned long BAUDRATE = 19200;
const int Rx_PIN = 3; // 共通Rx
const int Tx_CON_PIN = 1; // 接続方向Tx
const int Tx_BRD_PIN = 2; // 接続方向以外のTx (BRD : BRoaDcast)
const int LED_PIN = 0;

bool idSent = false;
bool isTerminal = true; // 終端ブロックかどうか
const int MAX_ID = 4;
Block::BlockId known_ids[MAX_ID];

BlockComm comm(BAUDRATE, Rx_PIN, Tx_BRD_PIN, Tx_CON_PIN);

void onReceived(const uint8_t* data, uint8_t size);

void setup()
{
    comm.init();

    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    for(unsigned int i = 0; i < MAX_ID; i++)
    {
        known_ids[i] = Block::None;
    }
    
    comm.subscribe(onReceived);
}

void onReceived(const uint8_t* data, uint8_t size)
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
                if (comm.getPrevCommand() == COM_ASK) break;
                comm.sendToConPort(COM_ASK, BLOCK_ID.Uid_H, BLOCK_ID.Uid_L);
            } else
            {
                comm.sendToAllPorts(COM_RET,
                    data[1], data[2],
                    BLOCK_ID.TypeId, BLOCK_ID.Uid_H, BLOCK_ID.Uid_L);
                
            }

            idSent = true;

        }
        break;

    case COM_RET:
        {
            if (!idSent) break;
            isTerminal = false;
            if(comm.getPrevCommand() == COM_RET) break;
            comm.writeToBrdPort(data, size);
        }
        break;

    case COM_TXD:
        {
            if (!idSent) break;
            if (comm.getPrevCommand() == COM_TXD) break;
            if (data[1] == BLOCK_ID.Uid_H && data[2] == BLOCK_ID.Uid_L)
            {
                switch (data[3])
                {
                case DAT_LED:
                    digitalWrite(LED_PIN, (data[4] == 0x01));
                    break;
                
                default:
                    break;
                }
            } else
            {
                comm.writeToConPort(data, size);
            }
        }
        break;

    case COM_RST:
        {
            if (comm.getPrevCommand() == COM_RST) break;

            idSent = false;
            isTerminal = true;
            comm.resetPrevCommand();
            
            for(unsigned int i = 0; i < MAX_ID; i++)
            {
                known_ids[i] = Block::None;
            }

            for(int i = 0; i < 3; i++)
            {
                comm.sendToConPort(COM_RST);
            }

        }
        break;

    default:
        break;
    }

}

// long time = millis();
// int state = LOW;

void loop()
{
    comm.listen();

    // if (idSent || millis() - time > 500)
    // {
    //     state = state == LOW ? HIGH : LOW;
    //     state = idSent ? HIGH : state;
    //     digitalWrite(LED_PIN, state);
    //     time = millis();
    // }

}
