#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>

#include "block.hpp"
#include "commands.hpp"
#include "comm.hpp"
#include "graph.hpp"

// ふろっくコアブロック
// for ESP32

const unsigned long BAUDRATE = 19200;

const unsigned int MAX_BLOCK = 32;
const unsigned int TIMEOUT = 500; // ms
const unsigned int RESENT_COUNT = 3;
const unsigned int INTERVAL = 500; // ms

BlockComm comm(BAUDRATE, 2);

const Block::BlockId CORE_ID = { Block::Role::PureCore, 0x00, 0x01 };
Graph graph = Graph(CORE_ID);
bool isScanning = false;
bool askSent = false;
int askCount = 0;
int lastSent = 0;

// Wi-Fi
const char* hostname = "floc_core_810001";
// const char* ssid = "oykdnAPBansui_g";
// const char* password = "a9327362a86761a528e7696dc60bfab7ce81cd0285f46f8c34d8150fdbb17cd7"; // PSK
// const char* ssid = "floc_wifi";
// const char* password = "223dcaa88bd4985891ab332b26a9eb5f297ba0b2ba76d62442d104b374deb533"; // PSK
const char password[] = "nitscproclub";
const IPAddress ip(192, 168, 0, 1);
const IPAddress subnet(255, 255, 255, 0);
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

void notFound(AsyncWebServerRequest* request)
{
    request->send(404, "text/plain", "[404] Not found");
}

String processor(const String& var)
{
  if(var == "ID")
  {
      char buf[8];
      sprintf(buf, "%02X%02X%02X", static_cast<uint8_t>(CORE_ID.RoleId), CORE_ID.Uid_H, CORE_ID.Uid_L);
      return String(buf);
  }
  return String();
}

void onEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len)
{
    const char delimiter = ' ';

    if(type == WS_EVT_CONNECT)
    {
        Serial.printf("ws(url=%s,id=%u) connected.\r\n", server->url(), client->id());
    }
    else if(type == WS_EVT_DISCONNECT)
    {
        Serial.println("Client disconnected.");
    }
    else if(type == WS_EVT_DATA)
    {
        String msgs[8];
        int idx = 0;
        uint16_t id = client->id();
        AwsFrameInfo* info = (AwsFrameInfo*)arg;
        if(info->opcode == WS_TEXT)
        {
            for(size_t i = 0; i < info->len; i++)
            {
                if ((char)data[i] == delimiter)
                {
                    if (i < info->len - 1) idx++;
                } else
                {
                    msgs[idx] += (char)data[i];
                }
            }
        }
        if (idx == 0 && msgs[0].length() == 0) return;

        Serial.print("[");
        Serial.print(id);
        Serial.print("] ");
        Serial.println(msgs[0]);

        if (msgs[0] == "ids")
        {
            DynamicJsonDocument doc(1024);
            JsonArray arrIds = doc.createNestedArray("ids");

            /*
            for(unsigned int i = 0; i < MAX_BLOCK; i++)
            {
                if (ids[i].RoleId != Block::Role::None)
                {
                    JsonObject block = arrIds.createNestedObject();
                    block["type"] = String(static_cast<uint8_t>(ids[i].RoleId), HEX);
                    block["uid"] = String(((long)(ids[i].Uid_H) << 8) + ids[i].Uid_L, HEX);
                }
            }
            */

            char buffer[2048];
            serializeJsonPretty(doc, buffer);
            ws.text(id, buffer);
        } else if (msgs[0] == "setid")
        {
            if (idx + 1 < 4) return;
            uint8_t tid = strtol(msgs[1].c_str(), 0, 16);
            uint8_t uid_h = strtol(msgs[2].c_str(), 0, 16);
            uint8_t uid_l = strtol(msgs[3].c_str(), 0, 16);
            
            comm.sendToBlock(COM_CFG);
            comm.sendToBlock(COM_SET, 0x02, tid, uid_h, uid_l);
            comm.sendToBlock(COM_SET, 0x01, 0x01);
            comm.sendToBlock(COM_APL);

            ws.text(id, "ok");
        } else if (msgs[0] == "setmode")
        {
            if (idx + 1 < 2) return;
            uint8_t mode = strtol(msgs[1].c_str(), 0, 16);

            comm.sendToBlock(COM_CFG);
            comm.sendToBlock(COM_SET, 0x01, mode);
            comm.sendToBlock(COM_APL);

            ws.text(id, "ok");
        } else if (msgs[0] == "reset")
        {
            Serial.println("Resetting...");
            graph = Graph(CORE_ID);
            isScanning = false;
            askCount = 0;
            askSent = false;
            lastSent = 0;
            comm.sendToBlock(COM_RST);
            Serial.println("wrote : COM_RST");
            Serial.println("completed.");

            ws.text(id, "ok");
        } else if (msgs[0] == "scan")
        {
            Serial.println("Scanning...");
            isScanning = true;

            ws.text(id, "scan start");
        } else if (msgs[0] == "ask")
        {
            comm.sendToBlock(COM_ASK, static_cast<uint8_t>(CORE_ID.RoleId), CORE_ID.Uid_H, CORE_ID.Uid_L);
            Serial.println("wrote : COM_ASK");

            ws.text(id, "ok");
        } else if (msgs[0] == "setled")
        {
            if (idx + 1 < 4) return;
            uint8_t uid_h = strtol(msgs[1].c_str(), 0, 16);
            uint8_t uid_l = strtol(msgs[2].c_str(), 0, 16);
            uint8_t led = strtol(msgs[3].c_str(), 0, 16);

            comm.sendToBlock(COM_TXD, uid_h, uid_l, DAT_LED, led);
            Serial.println("wrote : COM_TXD");

            ws.text(id, "ok");
        } else
        {
            ws.text(id, "message doesn't match any command pattern.");
        }
    }
}

void next()
{
    Block::BlockId dest_id = ids[_index];

    comm.sendToBlock(COM_TXD, dest_id.Uid_H, dest_id.Uid_L, DAT_LED, 0x01);

    _index++;
}

void onReceived(const uint8_t* data, uint8_t size);

void setup()
{
    Serial.begin(BAUDRATE);

    // SPIFFS
    SPIFFS.begin();

    // Wi-Fi
    Serial.println();
    // Serial.print("Connecting to ");
    // Serial.print(ssid);
    // WiFi.begin(ssid, password);
    WiFi.softAP(hostname, password);
    delay(100);
    WiFi.softAPConfig(ip, ip, subnet);
    IPAddress serverIP = WiFi.softAPIP();

    // while (WiFi.status() != WL_CONNECTED)
    // {
    //     delay(500);
    //     Serial.print(".");
    // }

    // Serial.println("");
    // Serial.println("WiFi connected");
    // Serial.println("IP address: ");
    // Serial.println(WiFi.localIP());
    Serial.println("");
    Serial.print("[AP] ssid:");
    Serial.println(hostname);
    Serial.print("IP:");
    Serial.println(serverIP);

    if (!MDNS.begin(hostname))
    {
        Serial.println("Error setting up MDNS responder!");
        while(1)
        {
            delay(1000);
        }
    }
    Serial.println("mDNS responder started");
    Serial.print("Now, you can access this ESP32 by <http://");
    Serial.print(hostname);
    Serial.println(".local/> .");

    // Server
    ws.onEvent(onEvent);
    server.addHandler(&ws);

    server.on("/config", HTTP_GET, [](AsyncWebServerRequest* request)
    {
        request->redirect("/config.html");
    });
    
    server.serveStatic("/", SPIFFS, "/")
        .setDefaultFile("index.html")
        .setTemplateProcessor(processor);

    server.onNotFound(notFound);

    server.begin();

    // Block
    comm.init();
    graph = Graph(CORE_ID);
    comm.subscribe(onReceived);

    Serial.println("initialized.");
}

void onReceived(const uint8_t* data, uint8_t size)
{
    Serial.print("incoming bytes : [");
    for(int i = 0; i < size; i++)
    {
        Serial.print(String(data[i], HEX));
        if (i < size - 1) Serial.print(", ");
    }
    Serial.println("]");

    if (size == 0) return;

    switch (data[0])
    {
    case COM_RET:
        {
            Block::BlockId parent = Block::BlockId(data[1], data[2], data[3]);
            Block::BlockId self = Block::BlockId(data[4], data[5], data[6]);
            
            graph.Insert(Edge(parent, self));
            
            askSent = false;
            askCount = 0;
        }
        break;

    default:
        break;
    }
}

bool ledState = false;

void loop()
{
    if (askCount > RESENT_COUNT)
    {
        if (isScanning)
        {
        Serial.println("Scan completed.");
        }
        isScanning = false;
    }
    if (isScanning && !askSent)
    {
        comm.sendToBlock(COM_ASK, static_cast<uint8_t>(CORE_ID.RoleId), CORE_ID.Uid_H, CORE_ID.Uid_L);
        delay(50);
        Serial.println("wrote : COM_ASK");
        askSent = true;
        askCount++;
        lastSent = millis();
    }
    if (millis() - lastSent >= TIMEOUT)
    {
        askSent = false;
    }

    comm.listen();

    while (const int size = Serial.available())
    {
        uint8_t data[size];
        Serial.readBytes(data, size);

        switch (data[0])
        {
        case 's':
        {
            Serial.println("Scanning...");
            isScanning = true;
        }
        break;

        case 'i':
        {
            Serial.println("set id...");
            comm.sendToBlock(COM_CFG);
            // comm.sendToBlock(COM_SET, 0x01, 0x11);
            comm.sendToBlock(COM_SET, 0x02, 0x11, 0x01, 0x01);
            comm.sendToBlock(COM_SET, 0x01, 0x01);
            comm.sendToBlock(COM_APL);
            Serial.println("sent.");
        }
        break;

        case 'd':
        {
            Serial.println("set debug mode...");
            comm.sendToBlock(COM_CFG);
            comm.sendToBlock(COM_SET, 0x01, 0x12);
            comm.sendToBlock(COM_APL);
            Serial.println("sent.");
        }
        break;

        // case 'p':
        //     {
        //         for(unsigned int i = 0; i < MAX_BLOCK; i++)
        //         {
        //             if (static_cast<uint8_t>(ids[i].RoleId) != 0xFF)
        //             {
        //                 Serial.print("[tid=");
        //                 Serial.print(String(static_cast<uint8_t>(ids[i].RoleId), HEX));
        //                 Serial.print(", uid=");
        //                 Serial.print(String(ids[i].Uid_H, HEX));
        //                 Serial.print(String(ids[i].Uid_L, HEX));
        //                 Serial.print("] -> ");
        //             }
        //         }
        //         Serial.println("");
        //         Serial.println("completed.");
        //     }
        //     break;

        case 'r':
            {
                Serial.println("Resetting...");
                graph = Graph(CORE_ID);
                isScanning = false;
                askCount = 0;
                askSent = false;
                lastSent = 0;
                comm.sendToBlock(COM_RST);
                Serial.println("wrote : COM_RST");
                Serial.println("completed.");
            }
            break;

        case 'l':
            {
                if (ledState)
                {
                    comm.sendToBlock(COM_TXD, 0x00, 0x01, DAT_LED, 0x00);
                    Serial.println("wrote : COM_TXD");
                    Serial.println("led off");
                } else
                {
                    comm.sendToBlock(COM_TXD, 0x00, 0x01, DAT_LED, 0x01);
                    Serial.println("wrote : COM_TXD");
                    Serial.println("led on");
                }
                ledState = !ledState;
            }
            break;

        case 'w':
            {
                comm.sendToBlock(COM_ASK, static_cast<uint8_t>(CORE_ID.RoleId), CORE_ID.Uid_H, CORE_ID.Uid_L);
                Serial.println("wrote : COM_ASK");
            }
            break;
        
        default:
            Serial.print("unknown command: ");
            Serial.println(('a' <= data[0] && data[0] <= 'z') ? (char)data[0] : '?');
            break;
        }
    }

}
