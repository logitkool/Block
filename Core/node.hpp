#pragma once
#include "block.hpp"

struct Node
{
    Block::BlockId id = Block::None;

    std::shared_ptr<Node> left = nullptr;
    std::shared_ptr<Node> right = nullptr;
};