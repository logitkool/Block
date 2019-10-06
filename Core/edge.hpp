#pragma once
#include "block.hpp"

struct Edge
{
    Block::BlockId parent, self;

    Edge(Block::BlockId p, Block::BlockId s)
        : parent(p), self(s) {}
};