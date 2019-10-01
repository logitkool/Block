#pragma once
#include "block.hpp"

struct Node
{
    Block::BlockId Id;

    Node* left = nullptr;
    Node* right = nullptr;
};