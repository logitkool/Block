#pragma once
#include "node.hpp"
#include "edge.hpp"
#include <vector>

class Graph
{
public:
    const static unsigned int MAX = 32;

public:
    void Add(Edge edge)
    {
        
    }

private:
    Node nodes[MAX];
    int index = 0;
};