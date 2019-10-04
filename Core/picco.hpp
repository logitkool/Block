#pragma once
#include <HTTPClient.h>

#include "block.hpp"

class PiccoRoboIoT
{
public:
    PiccoRoboIoT(const String& _uri)
        : picco_uri(_uri) {}

    // if
    bool IsBright()
    {
        double cds1 = getSensor(SENSOR_CDS1), cds2 = getSensor(SENSOR_CDS2);
        return max(cds1, cds2) >= THD_CDS;
    }

    bool HasDetectedObject()
    {
        double sum = 0.0, dist;
        int miss = 0;
        for(int i = 0; i < SAMPLE_COUNT_DIST; i++)
        {
            dist = getSensor(SENSOR_DIST);
            if (dist == 0) miss++;
            sum += dist;
        }
        if (miss >= SAMPLE_COUNT_DIST / 2) return false;
        return (sum / SAMPLE_COUNT_DIST) <= THD_DIST;
    }

    // ブロックの役割に応じて行動させる
    // ifやforの場合何もしない?
    void Action(Block::Role role)
    {
        String path = "", expected = "";
        switch (role)
        {
        case Block::Role::MoveFront:        path = "/go/"; expected = "Go"; break;
        case Block::Role::MoveBack:         path = "/back/"; expected = "Back"; break;

        case Block::Role::TurnLeft:         path = "/left/"; expected = "Left"; break;
        case Block::Role::TurnRight:        path = "/right/"; expected = "Right"; break;
        case Block::Role::TurnLeft90:       path = "/super_left/"; expected = "super_left"; break;
        case Block::Role::TurnRight90:      path = "/super_right/"; expected = "super_right"; break;

        case Block::Role::ShakeLeftHand:    path = "/left_hand/"; expected = "left_hand"; break;
        case Block::Role::ShakeRightHand:   path = "/right_hand/"; expected = "right_hand"; break;
        case Block::Role::ShakeBothHands:   path = "/hand/"; expected = "hand"; break;

        case Block::Role::ShakeLeftHead:    path = "/left_shake/"; expected = "left_shake"; break;
        case Block::Role::ShakeRightHead:   path = "/right_shake/"; expected = "right_shake"; break;

        default: break;
        }

        if (path != "")
        {
            String ret = get(path);
            // if (ret == expected)
        }
    }

    bool Stop()
    {
        return get("/stop/") == "Stop";
    }

    void PowerOff()
    {
        get("/poweroff/");
    }

private:
    HTTPClient client;
    const String picco_uri;

    const int THD_DIST = 250;
    const int SAMPLE_COUNT_DIST = 5;
    const int THD_CDS = 200;
    const int SENSOR_CDS1 = 0;
    const int SENSOR_CDS2 = 1;
    const int SENSOR_DIST = 3;

    // ピッコロボへgetリクエストを発行
    // path: "/xxx"のように頭に/をつける
    String get(const String& path)
    {

        client.begin(picco_uri + path);
        Serial.println("Start GET : " + picco_uri + path);
        client.setTimeout(1000);
        int code = client.GET();

        Serial.println("Response: " + code);
        Serial.println();
        if (code == HTTP_CODE_OK)
        {
            String body = client.getString();
            return body;
        }

        return "";
    }

    double getSensor(const int _sens_num)
    {
        String ret = get("/sens/");
        if (ret == "") return 0.0;
        String arr[5];
        int idx = 0;
        for(int i = 0; i < ret.length(); i++)
        {
            if (ret[i] == ',')
            {
                idx++;
                arr[idx] = "";
                continue;
            }
            arr[idx] += ret[i];
        }
        Serial.print("sens: ");
        Serial.println(arr[_sens_num].toDouble());
        return arr[_sens_num].toDouble();
    }

};
