#pragma once
#include "block.hpp"

struct Node
{
    Block::BlockId id;

    Node* left = nullptr;
    Node* right = nullptr;
};