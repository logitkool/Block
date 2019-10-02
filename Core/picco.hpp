#pragma once
#include "block.hpp"

class PiccoRoboIoT
{
public:
    PiccoRoboIoT() {}

    // if
    bool IsBright();
    bool HasDetectedObject();

    // ブロックの役割に応じて行動させる
    // ifやforの場合何もしない?
    void Action(Block::Role role);

private:

};
