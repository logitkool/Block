#pragma once
#include "node.hpp"
#include "edge.hpp"
#include "block.hpp"
#include "counter.hpp"
#include <stack>

class Graph
{
public:
    Graph(Block::BlockId core)
    {
        _root = new Node();
        _root->Id = core;
        _current = _root;
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

    Block::BlockId Next()
    {
        if(_current == nullptr)
        {
            if(!for_start.empty())
            {
                if(for_counter.top().IsLimit())
                {
                    _current = for_start.top()->left;
                    for_start.pop();
                    for_counter.pop();
                }
                else
                {
                    for_counter.top().CountUp();
                    _current = for_start.top()->right;
                }

                return _current->Id;
            }
            else return Block::None;
        }

        // TODO: leftに行く条件を追加して分岐に対応
        auto is_for 
            = [&](Block::Role role) { return Block::IsSameType(Block::Type::For, role); };
        if(is_for(_current->Id.RoleId))
        {
            while(is_for(_current->Id.RoleId))
            {
                for_start.push(_current);

                int limit = static_cast<uint8_t>(_current->Id.RoleId) & 0x0F;
                for_counter.push(Counter(0, limit));
                _current = _current->right;
            }

            return _current->Id;
        }

        _current = _current->right;

        // 何もつながっていない場合
        if(_current == nullptr)
        {
            return Block::None;
        }

        return _current->Id;
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
    Node* _current;
    std::stack<Node*> for_start;
    std::stack<Counter> for_counter;
};