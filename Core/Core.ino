#include <Arduino.h>

#include "block.hpp"
#include "commands.hpp"
#include "comm.hpp"

// ふろっくコアブロック
// for ESP32

const unsigned long BAUDRATE = 19200;

const unsigned int MAX_BLOCK = 32;
const unsigned int TIMEOUT = 500; // ms
const unsigned int RESENT_COUNT = 3;
const unsigned int INTERVAL = 1500; // ms

BlockComm comm(BAUDRATE, 2);

const Block::BlockId BLOCK_ID = { 0x81, 0x00, 0x02 };
Block::BlockId ids [MAX_BLOCK];
int _index = 0;
bool isScanning = false;
bool askSent = false;
int askCount = 0;
int lastSent = 0;

void next()
{
    Block::BlockId dest_id = ids[_index];

    comm.sendToBlock(COM_TXD, dest_id.Uid_H, dest_id.Uid_L, DAT_LED, 0x01);

    _index++;
}

void onReceived(const uint8_t* data, uint8_t size);

void setup()
{
    Serial.begin(BAUDRATE);
    comm.init();

    for(unsigned int i = 0; i < MAX_BLOCK; i++)
    {
        ids[i] = Block::None;
    }

    comm.subscribe(onReceived);

    Serial.println("initialized.");
}

void onReceived(const uint8_t* data, uint8_t size)
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
            ids[i].TypeId = data[3];
            ids[i].Uid_H = data[4];
            ids[i].Uid_L = data[5];
            askSent = false;
            askCount = 0;
        }
        break;

    default:
        break;
    }
}

bool ledState = false;

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
        comm.sendToBlock(COM_ASK, BLOCK_ID.Uid_H, BLOCK_ID.Uid_L);
        Serial.println("wrote : COM_ASK");
        askSent = true;
        askCount++;
        lastSent = millis();
    }
    if (millis() - lastSent >= TIMEOUT)
    {
        askSent = false;
    }

    comm.listen();

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

        case 'i':
        {
            Serial.println("set id...");
            comm.sendToBlock(COM_CFG);
            // comm.sendToBlock(COM_SET, 0x01, 0x11);
            comm.sendToBlock(COM_SET, 0x02, 0x11, 0x01, 0x01);
            comm.sendToBlock(COM_SET, 0x01, 0x01);
            comm.sendToBlock(COM_APL);
            Serial.println("sent.");
        }
        break;

        case 'd':
        {
            Serial.println("set debug mode...");
            comm.sendToBlock(COM_CFG);
            comm.sendToBlock(COM_SET, 0x01, 0x12);
            comm.sendToBlock(COM_APL);
            Serial.println("sent.");
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
                    ids[i] = Block::None;
                }
                isScanning = false;
                askCount = 0;
                askSent = false;
                lastSent = 0;
                comm.sendToBlock(COM_RST);
                Serial.println("wrote : COM_RST");
                Serial.println("completed.");
            }
            break;

        case 'l':
            {
                if (ledState)
                {
                    comm.sendToBlock(COM_TXD, 0x00, 0x01, DAT_LED, 0x00);
                    Serial.println("wrote : COM_TXD");
                    Serial.println("led off");
                } else
                {
                    comm.sendToBlock(COM_TXD, 0x00, 0x01, DAT_LED, 0x01);
                    Serial.println("wrote : COM_TXD");
                    Serial.println("led on");
                }
                ledState = !ledState;
            }
            break;

        case 'w':
            {
                comm.sendToBlock(COM_ASK, BLOCK_ID.Uid_H, BLOCK_ID.Uid_L);
                Serial.println("wrote : COM_ASK");
            }
            break;
        
        default:
            Serial.print("unknown command: ");
            Serial.println(('a' <= data[0] && data[0] <= 'z') ? (char)data[0] : '?');
            break;
        }
    }

}
