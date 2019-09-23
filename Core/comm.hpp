#pragma once

// inter-block communication library
// for ESP32

#include <HardwareSerial.h>
#define __AVR__
#include <Packetizer.h>
#undef __AVR__

class BlockComm
{
public:
    // ブロック間通信クラス
    // 
    // 使用前にinit()を呼び出す必要があります。
    // uart_nr: 0 -> Rx=3, Tx=1
    // uart_nr: 1 -> Rx=9, Tx=10
    // uart_nr: 2 -> Rx=16, Tx=17
    BlockComm(const unsigned long& _baudrate, const int& uart_nr)
        : BAUDRATE(_baudrate)
    {
        com = new HardwareSerial(uart_nr);
    }

    // ブロック間通信の初期化を行います。
    // setup()内で呼び出すこと
    void init()
    {
        com->begin(BAUDRATE, SERIAL_8N1, -1, -1, true);
    }

    // 受信コールバックを登録します。
    // index = 0x00 に登録されます。
    void subscribe(void (*callback)(const uint8_t* data, uint8_t size))
    {
        unpacker.subscribe(0x00, callback);
    }

    // 共通受信ポートでデータを待ち受け、受信データがある場合はコールバックを呼び出します。
    // loop()内で呼び出すこと。
    void listen()
    {
        while(const int size = com->available())
        {
            uint8_t data[size];
            com->readBytes(data, size);
            unpacker.feed(data, size);
        }
    }

    // 引数で与えられたデータをブロックに送信します。
    template<typename ...Param>
    void sendToBlock(int hex, Param ...params)
    {
        packer.init();
        pack(hex, params...);
        com->write(packer.data(), packer.size());
        com->flush();

        prev_command = hex;
    }

    // 配列で与えられたデータをブロックに送信します。
    void writeToBlock(const uint8_t* data, const uint8_t& size)
    {
        writeData(data, size);
    }

    // 直前に送信したコマンドの情報をリセットします。
    void resetPrevCommand()
    {
        prev_command = 0xFF;
    }
    
    // 直前に送信したコマンドを返します。
    uint8_t getPrevCommand()
    {
        return prev_command;
    }

private:
    HardwareSerial* com;
    const unsigned long BAUDRATE;
    uint8_t prev_command = 0xFF;
    Packetizer::Unpacker_<2, 16> unpacker;
    Packetizer::Packer_<16> packer;

    void writeData(const uint8_t* data, const uint8_t& size)
    {
        packer.init();
        for(uint8_t i = 0; i < size; i++)
        {
            packer << data[i];
        }
        packer << Packetizer::endp();

        com->write(packer.data(), packer.size());
        com->flush();

        prev_command = data[0];
    }

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

};
