#pragma once

class Counter
{
public:
    bool IsLimit()
    {
        return (_count >= _limit - 1);
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

