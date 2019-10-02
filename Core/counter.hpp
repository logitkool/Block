#pragma once

class Counter
{
public:
    bool IsLimit()
    {
        return _count < _limit;
    }

    void CountUp()
    {
        _count++;
    }

    Counter(int count, int _limit) : _count(count), _limit(_limit)
    {}

private:
    int _count, _limit;
};

