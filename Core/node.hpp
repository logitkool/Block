#pragma once
#include "block.hpp"

struct Node
{
    Block::BlockId id = Block::None;

    Node* left = nullptr;
    Node* right = nullptr;
};