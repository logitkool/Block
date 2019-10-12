#include <Arduino.h>

#include "block.hpp"
#include "commands.hpp"
#include "comm.hpp"
#include "config.hpp"

// ふろっく 分岐/繰り返しブロック
// for ATtiny85

// 設定
const unsigned long BAUDRATE = 19200;
const int Rx_PIN = 3; // 共通Rx
const int Tx_TRUE_PIN = 1; // 接続方向Tx (真, True)
const int Tx_FALSE_PIN = 4; // 接続方向Tx (偽, False)
const int Tx_BRD_PIN = 2; // 接続方向以外のTx (BRD : BRoaDcast)
const int LED_PIN = 0;

bool idSent = false;
bool isTerminal = true; // 終端ブロックかどうか
bool conSideIsTrue = true; // 探索(接続)方向がTrue方向かどうか
const int MAX_ID = 4;
Block::BlockId known_ids[MAX_ID];

BlockComm comm(BAUDRATE, Rx_PIN, Tx_BRD_PIN, Tx_TRUE_PIN, Tx_FALSE_PIN);
Config::Config config;

namespace impl_led
{
    const int BLINK[] PROGMEM = { 2000, 2, 0, 1000 };
    const int CONFIG[] PROGMEM = { 5000, 4, 0, 500, 1000, 1500 };
}
const int* const ledPattern[] PROGMEM =
{
    impl_led::BLINK, impl_led::CONFIG
};
int ledCurrent = 0;
uint8_t ledPrevTime = 0;
bool ledState = LOW;

void onReceived(const uint8_t* data, uint8_t size);

void setup()
{
    config.load();

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
        if (config.getMode() != Config::Mode::PRODUCTION && config.getMode() != Config::Mode::DEBUG) break;
        if (size < 3) break;
        {
            unsigned int idx = 0;
            while(idx < MAX_ID && known_ids[idx].Uid_H != 0xFF && known_ids[idx].Uid_H != 0xFF) idx++;
            if (idx == MAX_ID) break;

            bool isKnown = false;
            for(unsigned int i = 0; i < idx; i++)
            {
                if (known_ids[i].Uid_H == data[2] && known_ids[i].Uid_L == data[3])
                {
                    isKnown = true;
                }
            }

            if (isKnown)
            {
                if (comm.IsSentPrevious(data, size)) break;
                if (conSideIsTrue)
                {
                    comm.sendToTruePort(COM_ASK, config.getId().TypeId, config.getId().Uid_H, config.getId().Uid_L);
                } else
                {
                    comm.sendToFalsePort(COM_ASK, config.getId().TypeId, config.getId().Uid_H, config.getId().Uid_L);
                }
            } else
            {
                known_ids[idx] = {data[1], data[2], data[3]};
                comm.sendToAllPorts(COM_RET,
                    data[1], data[2], data[3],
                    config.getId().TypeId, config.getId().Uid_H, config.getId().Uid_L);
                
            }

            idSent = true;

        }
        break;

    case COM_RET:
        if (config.getMode() != Config::Mode::PRODUCTION && config.getMode() != Config::Mode::DEBUG) break;
        {
            if (!idSent) break;
            isTerminal = false;
            if (comm.IsSentPrevious(data, size)) break;
            comm.writeToBrdPort(data, size);
        }
        break;
    
    case COM_SWC:
        if (config.getMode() != Config::Mode::PRODUCTION && config.getMode() != Config::Mode::DEBUG) break;
        if (size > 4) break;
        {
            if (!idSent) break;
            if (config.getId().Uid_H == data[1] && config.getId().Uid_L == data[2])
            {
                conSideIsTrue = (data[3] == 0x01);
            } else
            {
                if (conSideIsTrue)
                {
                    comm.writeToTruePort(data, size);
                } else
                {
                    comm.writeToFalsePort(data, size);
                }
            }
        }
        break;
    
    case COM_CFG:
        {
            // ブロックを設定モードにする
            config.changeMode(Config::Mode::CONFIG);
        }
        break;

    case COM_APL:
        {
            // 設定の内容を適応・保存する
            config.apply();
        }
        break;

    case COM_SET:
        {
            // ブロックの設定を変更する
            switch (data[1])
            {
            case static_cast<int>(Config::SettingType::MODE):
                if (size < 3) break;
                config.changeMode(static_cast<Config::Mode>(data[2]));
                break;

            case static_cast<int>(Config::SettingType::ID):
                if (size < 5) break;
                config.setId(data[2], data[3], data[4]);
                break;
            
            default:
                break;
            }
        }
        break;

    case COM_TXD:
        if (config.getMode() != Config::Mode::PRODUCTION && config.getMode() != Config::Mode::DEBUG) break;
        {
            if (!idSent) break;
            if (size < 5) break;
            if (data[1] == config.getId().Uid_H && data[2] == config.getId().Uid_L)
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
                comm.writeToTruePort(data, size);
                comm.writeToFalsePort(data, size);
            }
        }
        break;

    case COM_RST:
        if (config.getMode() != Config::Mode::PRODUCTION && config.getMode() != Config::Mode::DEBUG) break;
        {
            if (comm.IsSentPrevious(data, size)) break;

            idSent = false;
            isTerminal = true;
            comm.resetPrevCommands();
            
            for(unsigned int i = 0; i < MAX_ID; i++)
            {
                known_ids[i] = Block::None;
            }

            for(int i = 0; i < 3; i++)
            {
                comm.sendToTruePort(COM_RST);
                comm.sendToFalsePort(COM_RST);
            }

        }
        break;

    default:
        break;
    }

}

void loop()
{
    comm.listen();

    int pattern = -1;
    switch (config.getMode())
    {
    case Config::Mode::PRODUCTION:
        {
            if (idSent)
            {
                if (!ledState)
                {
                    digitalWrite(LED_PIN, LOW);
                    ledState = false;
                }
            } else
            {
                pattern = 0;
            }
        }
        break;

    case Config::Mode::DEBUG:
        {
            if (idSent)
            {
                digitalWrite(LED_PIN, HIGH);
            } else
            {
                pattern = 0;
            }
            
        }
        break;

    case Config::Mode::CONFIG:
        pattern = 1;
        break;

    default:
        break;
    }
    if (pattern >= 0)
    {
        int* p = (int*)pgm_read_word(&ledPattern[pattern]);
        int cycle = pgm_read_word(p);
        int pattern_size = pgm_read_word(p + 1) + 2;
        int time = millis() % cycle;
        if (ledPrevTime > time)
        {
            ledState = LOW;
            ledCurrent = 2;
        }
        ledPrevTime = time;
        if (ledCurrent >= 2)
        {
            int t = pgm_read_word(p + ledCurrent);
            if (t <= time)
            {
                ledState = !ledState;
                ledCurrent++;
                if (ledCurrent >= pattern_size) ledCurrent = -1;
            }
        }
        digitalWrite(LED_PIN, ledState);
    }

}
