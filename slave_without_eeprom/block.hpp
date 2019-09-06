#pragma once

struct BlockId
{
    unsigned int TypeId;  // 1byte (Max: 0xFF)
    // Uid: 2byte (Max: 0xFFFF)
    unsigned int Uid_H;
    unsigned int Uid_L;

    const BlockId None = {0xFF, 0xFF, 0xFF};
};
