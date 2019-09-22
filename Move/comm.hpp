#pragma once

// communication lib
// for ATtiny85

#include <SoftwareSerial.h>
#define __AVR__
#include <Packetizer.h>
#undef __AVR__

class Comm
{
public:
    Comm(const unsigned long _baudrate, const uint8_t _rxpin, const uint8_t _brdpin, const uint8_t _conpin)
        : BaudRate(_baudrate)
    {
        comConTrue = new SoftwareSerial(_rxpin, _conpin, true);
        comBroadcast = new SoftwareSerial(_rxpin, _brdpin, true);

        isTwoCon = false;
    }

    Comm(const unsigned long _baudrate, const uint8_t _rxpin, const uint8_t _brdpin, const uint8_t _truepin, const uint8_t _falsepin)
        : BaudRate(_baudrate)
    {
        comConTrue = new SoftwareSerial(_rxpin, _truepin, true);
        comConFalse = new SoftwareSerial(_rxpin, _falsepin, true);
        comBroadcast = new SoftwareSerial(_rxpin, _brdpin, true);

        isTwoCon = true;
    }

    inline void init()
    {
        comBroadcast->begin(BaudRate);
        prev_command = 0xFF;
    }

    void subscribe(void (*callback)(const uint8_t* data, uint8_t size))
    {
        unpacker.subscribe(0, callback);
    }

    void listen()
    {
        while (const int size = comBroadcast->available())
        {
            uint8_t data[size];
            comBroadcast->readBytes(data, size);
            unpacker.feed(data, size);
        }
    }

    // Con/True side
    template<typename ...Param>
    void writeToConSide(const uint8_t& hex, Param& ...params)
    {
        writeToTrueSide(hex, params...);
    }

    void writeToConSide(const uint8_t* data, const uint8_t& size)
    {
        writeToTrueSide(data, size);
    }

    template<typename ...Param>
    void writeToTrueSide(const uint8_t& hex, Param& ...params)
    {
        comBroadcast->end();
        comConTrue->begin(BaudRate);

        writeData(comConTrue, hex, params...);

        comConTrue->end();
        comBroadcast->begin(BaudRate);
    }

    void writeToTrueSide(const uint8_t* data, const uint8_t& size)
    {
        comBroadcast->end();
        comConTrue->begin(BaudRate);

        comConTrue->write(data, size);

        comConTrue->end();
        comBroadcast->begin(BaudRate);
    }

    // False side
    template<typename ...Param>
    void writeToFalseSide(const uint8_t& hex, Param& ...params)
    {
        if (!isTwoCon)
        {
            return;
        }

        comBroadcast->end();
        comConFalse->begin(BaudRate);

        writeData(comConFalse, hex, params...);

        comConFalse->end();
        comBroadcast->begin(BaudRate);
    }

    void writeToFalseSide(const uint8_t* data, const uint8_t& size)
    {
        if (!isTwoCon)
        {
            return;
        }

        comBroadcast->end();
        comConFalse->begin(BaudRate);

        comConFalse->write(data, size);

        comConFalse->end();
        comBroadcast->begin(BaudRate);
    }

    // Broadcast sides
    template<typename ...Param>
    void writeToBrdSide(const uint8_t& hex, Param& ...params)
    {
        writeData(comBroadcast, hex, params...);
    }

    void writeToBrdSide(const uint8_t* data, const uint8_t& size)
    {
        comBroadcast->write(data, size);
    }

    // All 4 sides
    template<typename ...Param>
    void writeToAll(const uint8_t& hex, Param& ...params)
    {
        writeToBrdSide(hex, params...);
        writeToConSide(hex, params...);
        if (isTwoCon)
        {
            writeToFalseSide(hex, params...);
        }
    }

    void writeToAll(const uint8_t* data, const uint8_t& size)
    {
        writeToBrdSide(data, size);
        writeToConSide(data, size);
        if (isTwoCon)
        {
            writeToFalseSide(data, size);
        }
    }

    inline const uint8_t getPrevCommand()
    {
        return prev_command;
    }

private:
    const unsigned long BaudRate;
    bool isTwoCon;
    SoftwareSerial* comConTrue;
    SoftwareSerial* comConFalse;
    SoftwareSerial* comBroadcast;

    Packetizer::Unpacker_<2, 16> unpacker;
    Packetizer::Packer_<16> packer;
    uint8_t prev_command;

    template<typename ...Param>
    void writeData(SoftwareSerial* com, const uint8_t& hex, Param& ...params)
    {
        packer.init();
        
        pack(hex, params...);

        com->write(packer.data(), packer.size());
        com->flush();

        prev_command = hex;
    }

    template<typename ...Param>
    void pack(const uint8_t& hex, Param ...params)
    {
        append(hex);
        pack(params...);
    }

    void pack()
    {
        packer << Packetizer::endp();
    }

    void append(const uint8_t& hex)
    {
        packer << hex;
    }

};
