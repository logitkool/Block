#pragma once

#include <EEPROM.h>
#include "block.hpp"

namespace Config
{
    const uint8_t None = 0xFF;

    enum class RomAddress : uint8_t
    {
        FLAG = 0x00,
        TYPE_ID = 0x02,
        UID_H = 0x03,
        UID_L = 0x04,
        MODE = 0x06
    };

    enum class Mode : uint8_t
    {
        NOT_LOADED = 0x00,
        PRODUCTION = 0x01,
        CONFIG = 0x11,
        DEBUG = 0x12
    };

    enum class SettingType : uint8_t
    {
        MODE = 0x01,
        ID = 0x02
    };
    
    class Config
    {
    public:
        Config() {}

        void load()
        {
            if (loadFlag())
            {
                loadId();
                loadMode();
            }
        }

        const Block::BlockId getId()
        {
            return currentId;
        }

        void setId(const uint8_t& type_id, const uint8_t& uid_h, const uint8_t& uid_l)
        {
            if (currentMode != Mode::CONFIG) return;
            nextId = Block::BlockId{ type_id, uid_h, uid_l };
        }

        const Mode& getMode()
        {
            return currentMode;
        }

        void apply()
        {
            if (currentMode != Mode::CONFIG) return;
            if (!isNone(nextId))
            {
                currentId = nextId;
                saveId();
            } else if (romIsClear) return;

            romIsClear = false;

            if (currentMode != nextMode) saveMode(true);
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

        bool romIsClear = false;

        bool isNone(const Block::BlockId& id)
        {
            return (id.TypeId == Block::None.TypeId && id.Uid_L == Block::None.Uid_L && id.Uid_H == Block::None.Uid_H);
        }

        bool loadFlag()
        {
            uint8_t flag = EEPROM.read(static_cast<uint8_t>(RomAddress::FLAG));
            if (flag == 0xFF)
            {
                // eeprom is clear
                romIsClear = true;
                EEPROM.write(static_cast<uint8_t>(RomAddress::FLAG), 0x01);
                currentMode = Mode::CONFIG;
                EEPROM.write(static_cast<uint8_t>(RomAddress::MODE), static_cast<uint8_t>(currentMode));

                return false;
            }

            return true;
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

        void saveMode(bool saveNext = false)
        {
            uint8_t value = static_cast<uint8_t>(saveNext ? nextMode : currentMode);
            EEPROM.write(static_cast<uint8_t>(RomAddress::MODE), value);
        }

    };

}
