#pragma once

// inter-block communication library
// for ATtiny85

#include <SoftwareSerial.h>
#define __AVR__
#include <Packetizer.h>
#undef __AVR__

class BlockComm
{
public:
    // ブロック間通信クラス
    // 
    // 動作ブロック向けコンストラクタです。
    // 使用前にinit()を呼び出す必要があります。
    BlockComm(const unsigned long& _baudrate, const uint8_t& _rxpin, const uint8_t& _tx_brd, const uint8_t& _tx_con)
        : BAUDRATE(_baudrate)
    {
        comBrd = new SoftwareSerial(_rxpin, _tx_brd, true);
        comTrue = new SoftwareSerial(_rxpin, _tx_con, true);
        useTwoPorts = false;
    }

    // ブロック間通信クラス
    // 
    // 分岐/繰り返しブロック向けコンストラクタです。
    // 使用前にinit()を呼び出す必要があります。
    // 繰り返しブロックでは、Trueを繰り返し方向、Falseを終了方向に読み変えてください。
    BlockComm(const unsigned long& _baudrate, const uint8_t& _rxpin, const uint8_t& _tx_brd, const uint8_t& _tx_conTrue, const uint8_t& _tx_conFalse)
        : BAUDRATE(_baudrate)
    {
        comBrd = new SoftwareSerial(_rxpin, _tx_brd, true);
        comTrue = new SoftwareSerial(_rxpin, _tx_conTrue, true);
        comFalse = new SoftwareSerial(_rxpin, _tx_conFalse, true);
        useTwoPorts = true;
    }

    // ブロック間通信の初期化を行います。
    // setup()内で呼び出すこと
    void init()
    {
        comBrd->begin(BAUDRATE);
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
        while(const int size = comBrd->available())
        {
            uint8_t data[size];
            comBrd->readBytes(data, size);
            unpacker.feed(data, size);
        }
    }

    // 引数で与えられたデータをブロードキャストポートに送信します。
    template<typename ...Param>
    void sendToBrdPort(int hex, Param ...params)
    {
        packer.init();
        pack(hex, params...);
        comBrd->write(packer.data(), packer.size());
        comBrd->flush();

        prev_command = hex;
    }

    // 配列で与えられたデータをブロードキャストポートに送信します。
    void writeToBrdPort(const uint8_t* data, const uint8_t& size)
    {
        writeData(comBrd, data, size);
    }

    // 引数で与えられたデータを真側(繰り返し側)ポートに送信します。
    template<typename ...Param>
    void sendToTruePort(int hex, Param ...params)
    {
        comBrd->end();
        comTrue->begin(BAUDRATE);

        packer.init();
        pack(hex, params...);
        comTrue->write(packer.data(), packer.size());
        comTrue->flush();

        comTrue->end();
        comBrd->begin(BAUDRATE);

        prev_command = hex;
    }

    // 引数で与えられたデータを接続方向ポートに送信します。
    template<typename ...Param>
    void sendToConPort(int hex, Param ...params)
    {
        sendToTruePort(hex, params...);
    }

    // 配列で与えられたデータを真側(繰り返し側)ポートに送信します。
    void writeToTruePort(const uint8_t* data, const uint8_t& size)
    {
        comBrd->end();
        comTrue->begin(BAUDRATE);

        writeData(comTrue, data, size);

        comTrue->end();
        comBrd->begin(BAUDRATE);
    }

    // 配列で与えられたデータを接続方向ポートに送信します。
    void writeToConPort(const uint8_t* data, const uint8_t& size)
    {
        writeToTruePort(data, size);
    }

    // 引数で与えられたデータを偽側(繰り返し終了側)ポートに送信します。
    template<typename ...Param>
    void sendToFalsePort(int hex, Param ...params)
    {
        if (!useTwoPorts) return;

        comBrd->end();
        comFalse->begin(BAUDRATE);

        packer.init();
        pack(hex, params...);
        comFalse->write(packer.data(), packer.size());
        comFalse->flush();

        comFalse->end();
        comBrd->begin(BAUDRATE);

        prev_command = hex;
    }

    // 配列で与えられたデータを偽側(繰り返し終了側)ポートに送信します。
    void writeToFalsePort(const uint8_t* data, const uint8_t& size)
    {
        if (!useTwoPorts) return;

        comBrd->end();
        comFalse->begin(BAUDRATE);

        writeData(comFalse, data, size);

        comFalse->end();
        comBrd->begin(BAUDRATE);
    }

    // 引数で与えられたデータを全てのポートに送信します。
    template<typename ...Param>
    void sendToAllPorts(int hex, Param ...params)
    {
        sendToBrdPort(hex, params...);
        sendToTruePort(hex, params...);
        if (useTwoPorts)
        {
            sendToFalsePort(hex, params...);
        }
    }

    // 配列で与えられたデータを全てのポートに送信します。
    void writeToAllPorts(const uint8_t* data, const uint8_t& size)
    {
        writeToBrdPort(data, size);
        writeToTruePort(data, size);
        if (useTwoPorts)
        {
            writeToFalsePort(data, size);
        }
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
    SoftwareSerial* comBrd;
    SoftwareSerial* comTrue;
    SoftwareSerial* comFalse;

    bool useTwoPorts;

    const unsigned long BAUDRATE;
    uint8_t prev_command = 0xFF;
    Packetizer::Unpacker_<2, 16> unpacker;
    Packetizer::Packer_<16> packer;

    void writeData(const SoftwareSerial* com, const uint8_t* data, const uint8_t& size)
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
