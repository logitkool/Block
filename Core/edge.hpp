#pragma once
#include "block.hpp"

struct Edge
{
    Block::BlockId parent, self;
};