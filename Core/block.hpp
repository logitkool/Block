#pragma once

namespace Block
{
    struct BlockId
    {
        // 1byte (Max: 0xFF)
        uint8_t TypeId;
        // Uid: 2byte (Max: 0xFFFF)
        uint8_t Uid_H;
        uint8_t Uid_L;
    };
    
    const BlockId None = { 0xFF, 0xFF, 0xFF };
    
};
