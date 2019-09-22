#pragma once

// communication lib
// for ESP32

#include <HardwareSerial.h>
#define __AVR__
#include <Packetizer.h>
#undef __AVR__

class Comm
{
public:
    // HardwareSerial(2) : RxD=16, TxD=17
    Comm(const unsigned long _baudrate)
        : BaudRate(_baudrate)
    {}

    inline void init()
    {
        com.begin(BaudRate, SERIAL_8N1, -1, -1, true);
    }

    void subscribe(void (*callback)(const uint8_t* data, uint8_t size))
    {
        unpacker.subscribe(0, callback);
    }

    void listen()
    {
        while (const int size = com.available())
        {
            uint8_t data[size];
            com.readBytes(data, size);
            unpacker.feed(data, size);
        }
    }

    template<typename ...Param>
    void writeToConSide(const uint8_t hex, Param ...params)
    {
        writeData(hex, params...);
    }

    void writeToConSide(const uint8_t* data, const uint8_t& size)
    {
        com.write(data, size);
    }

private:
    const unsigned long BaudRate;
    HardwareSerial com = HardwareSerial(2);

    Packetizer::Unpacker_<2, 16> unpacker;
    Packetizer::Packer_<16> packer;

    template<typename ...Param>
    void writeData(const uint8_t& hex, Param& ...params)
    {
        packer.init();
        
        pack(hex, params...);

        com.write(packer.data(), packer.size());
        com.flush();
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
