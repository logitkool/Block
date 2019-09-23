#pragma once

// 0x0- : config
const uint8_t COM_RST = 0x01;

// 0xA- : block scan
const uint8_t COM_ASK = 0xA0;
const uint8_t COM_RET = 0xA1;

// 0xD- : data transfer
const uint8_t COM_DAT = 0xD0;
const uint8_t COM_TXD = 0xD1;

// data types
const uint8_t DAT_LED = 0x01;
