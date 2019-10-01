#pragma once
#include "node.hpp"
#include "edge.hpp"
#include "block.hpp"
#include <vector>

class Graph
{
public:
    Graph(Block::BlockId core)
    {
        _root = new Node();
        _root->Id = core;
    }
    
    ~Graph()
    {
        clear_tree(_root);
    }

public:
    void Insert(Edge edge)
    {
        Node* node = new Node();
        node->Id = edge.self;

        Block::BlockId const& p = edge.parent;
        Node* parent_node = find(_root, p.Uid_H, p.Uid_L);

        // ノードを木にぶら下げる
        if(parent_node->right == nullptr)
        {
            parent_node->right = node;
        }
        else
        {
            parent_node->left = node;
        }
    }
    
private:
    Node* find(Node* node, uint8_t id_h, uint8_t id_l)
    {
        if(node == nullptr) return nullptr;

        if(node->Id.Uid_H == id_h
        && node->Id.Uid_L == id_l)
        {
            return node;
        }

        Node* right = find(node->right, id_h, id_l);
        if(right != nullptr)
        {
            return right;
        }

        Node* left = find(node->left, id_h, id_l);
        if(left != nullptr)
        {
            return left;
        }

        return nullptr;
    }

    void clear_tree(Node* node)
    {
        if(node == nullptr)
        {
            return;
        }

        clear_tree(node->right);
        clear_tree(node->left);

        delete node;
    }

private:
    Node* _root;
};