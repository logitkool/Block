#pragma once
#include "node.hpp"
#include "edge.hpp"
#include "block.hpp"
#include "counter.hpp"
#include "picco.hpp"
#include <stack>
#include <set>
#include <ArduinoJson.h>

class Graph
{
public:
    Graph(Block::BlockId core)
    {
        _root = std::make_shared<Node>();
        _root->id = core;
        _current = _root;
    }
    
    ~Graph()
    {
        clear_tree(_root);
    }

public:
    void Insert(Edge edge)
    {
        std::shared_ptr<Node> node;
        Block::BlockId const& s = edge.self;

        std::shared_ptr<Node> self_node = find(_root, s.Uid_H, s.Uid_L);
        if (self_node != nullptr)
        {
            node = self_node;
        } else
        {
            node = std::make_shared<Node>();
            node->id = edge.self;
        }

        Block::BlockId const& p = edge.parent;
        std::shared_ptr<Node> parent_node = find(_root, p.Uid_H, p.Uid_L);

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

    Block::BlockId Next(PiccoRoboIoT& picco)
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

                return _current->id;
            }
            else return Block::None;
        }

        auto is_same_type
            = [&](Block::Type type, Block::Role role) { return Block::IsSameType(type, role); };
        if(is_same_type(Block::Type::For, _current->id.RoleType))
        {
            for_start.push(_current);

            int limit = static_cast<uint8_t>(_current->id.RoleType) & 0x0F;
            for_counter.push(Counter(0, limit));
            _current = _current->right;

            return _current->id;
        }
        else if(is_same_type(Block::Type::If, _current->id.RoleType))
        {
            bool flag = false;
            switch(_current->id.RoleType)
            {
            case Block::Role::IfBrightness:
                flag = picco.IsBright();
                break;
            case Block::Role::IfObject:
                flag = picco.HasDetectedObject();
                break;
            default:
                break;
            }

            if(flag) _current = _current->right;
            else _current = _current->left;

            return _current->id;
        }

        _current = _current->right;

        // 何もつながっていない場合
        if(_current == nullptr)
        {
            return Block::None;
        }

        return _current->id;
    }

    String ToString()
    {
        DynamicJsonDocument doc(1024);
        JsonArray arrNodes = doc.createNestedArray("nodes");

        if (_root == nullptr) return "{\"error\": \"_root is null.\"}";
        
        std::stack<std::shared_ptr<Node>> s;
        s.push(_root);
        std::shared_ptr<Node> p;
        auto cmp_id = [](Block::BlockId lhs, Block::BlockId rhs) { return (lhs.RoleId() < rhs.RoleId() || lhs.Uid_H < rhs.Uid_H || lhs.Uid_L < rhs.Uid_L); };
        std::set<Block::BlockId, decltype(cmp_id)> visited(cmp_id);
        while(s.size() > 0)
        {
            p = s.top();
            s.pop();

            if (visited.count(p->id) > 0) continue;
            visited.insert(p->id);

            JsonObject node = arrNodes.createNestedObject();
            node["type"] = String(p->id.RoleId(), HEX);
            node["uid"] = String(((long)(p->id.Uid_H) << 8) + p->id.Uid_L, HEX);
            node["left"] = p->left == nullptr ? "null" :  String(((long)(p->left->id.Uid_H) << 8) + p->left->id.Uid_L, HEX);
            node["right"] = p->right == nullptr ? "null" :  String(((long)(p->right->id.Uid_H) << 8) + p->right->id.Uid_L, HEX);
            
            if (p->right != nullptr) s.push(p->right);
            if (p->left != nullptr) s.push(p->left);
        }

        char buffer[2048];
        serializeJsonPretty(doc, buffer);

        return String(buffer);
    }

private:
    std::shared_ptr<Node> find(std::shared_ptr<Node> node, uint8_t id_h, uint8_t id_l)
    {
        Serial.println(node == nullptr ? "node is null" : "node is not null");
        if(node == nullptr)
        {
            Serial.print("H: ");
            Serial.print(id_h);
            Serial.print("L: ");
            Serial.println(id_l);
            return nullptr;
        }
        Serial.print("node->id.H: ");
        Serial.print(node->id.Uid_H);
        Serial.print(", node->id.L: ");
        Serial.println(node->id.Uid_L);
        Serial.print("H: ");
        Serial.print(id_h);
        Serial.print("L: ");
        Serial.println(id_l);
        
        if(node->id.Uid_H == id_h
        && node->id.Uid_L == id_l)
        {
            Serial.println("match!");
            return node;
        }

        std::shared_ptr<Node> right = find(node->right, id_h, id_l);
        if(right != nullptr)
        {
            return right;
        }

        std::shared_ptr<Node> left = find(node->left, id_h, id_l);
        if(left != nullptr)
        {
            return left;
        }

        return nullptr;
    }

    void clear_tree(std::shared_ptr<Node> node)
    {
        if(node == nullptr)
        {
            return;
        }

        clear_tree(node->right);
        clear_tree(node->left);

        node = nullptr;
    }

private:
    std::shared_ptr<Node> _root;
    std::shared_ptr<Node> _current;
    std::stack<std::shared_ptr<Node>> for_start;
    std::stack<Counter> for_counter;
};