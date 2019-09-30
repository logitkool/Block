#pragma once
#include "block.hpp"

struct Node
{
    Block::BlockId Id;

    Node* left;
    Node* right;
};