#pragma once

#include <EEPROM.h>
#include "block.hpp"

namespace Config
{
    const uint8_t None = 0xFF;

    enum class RomAddress : uint8_t
    {
        TYPE_ID = 0x00,
        UID_H = 0x02,
        UID_L = 0x03,
        MODE = 0x05
    };

    enum class Mode : uint8_t
    {
        NOT_LOADED = 0x00,
        PRODUCTION = 0x01,
        CONFIG = 0x11,
        DEBUG = 0x12
    };

    class Config
    {
    public:
        Config() {}

        void load()
        {
            loadId();
            loadMode();
        }

        const Block::BlockId getId()
        {
            return currentId;
        }

        void setId(const Block::BlockId& id)
        {
            if (currentMode != Mode::CONFIG) return;
            nextId = id;
        }

        const Mode& getMode()
        {
            return currentMode;
        }

        void applyMode(const Block::BlockId block)
        {
            if (isNone(block) && isNone(nextId))
            {
                if (block.TypeId == nextId.TypeId
                    && block.Uid_H == nextId.Uid_H
                    && block.Uid_L == nextId.Uid_L)
                {
                    currentId = nextId;
                    saveId();
                }
            }

            if (currentMode != nextMode) saveMode();
            currentMode = nextMode;
        }

        void changeMode(const Mode& mode)
        {
            if (currentMode == Mode::CONFIG)
            {
                nextMode = mode;
            } else
            {
                if (mode != Mode::CONFIG) return;
                nextMode = currentMode;
                currentMode = mode;
            }
        }

    private:
        Block::BlockId currentId = Block::None;
        Block::BlockId nextId = Block::None;

        Mode currentMode = Mode::NOT_LOADED;
        Mode nextMode = Mode::NOT_LOADED;

        bool isNone(const Block::BlockId& id)
        {
            return (id.TypeId == Block::None.TypeId && id.Uid_L == Block::None.Uid_L && id.Uid_H == Block::None.Uid_H);
        }

        void loadId()
        {
            uint8_t type = EEPROM.read(static_cast<uint8_t>(RomAddress::TYPE_ID));
            uint8_t uid_h = EEPROM.read(static_cast<uint8_t>(RomAddress::UID_H));
            uint8_t uid_l = EEPROM.read(static_cast<uint8_t>(RomAddress::UID_L));

            if (type == None && uid_h == None && uid_l == None)
            {
                return;
            }

            currentId = Block::BlockId{ type, uid_h, uid_l };
        }

        void saveId()
        {
            EEPROM.write(static_cast<uint8_t>(RomAddress::TYPE_ID), currentId.TypeId);
            EEPROM.write(static_cast<uint8_t>(RomAddress::UID_H), currentId.Uid_H);
            EEPROM.write(static_cast<uint8_t>(RomAddress::UID_L), currentId.Uid_L);
        }

        void loadMode()
        {
            uint8_t mode = EEPROM.read(static_cast<uint8_t>(RomAddress::MODE));
            currentMode = static_cast<Mode>(mode);
        }

        void saveMode()
        {
            EEPROM.write(static_cast<uint8_t>(RomAddress::MODE), static_cast<uint8_t>(currentMode));
        }

    };

}
