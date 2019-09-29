# Notice

- 実装時に注意したり、行き詰まったりした点
  - 苦労した点とも言える
- 気づいたら追記する

## ATtiny85

- `int`は2bytes
  - shortじゃん
  - 当時のBAUDRATE(38400)は、int幅(-32768 ～ 32767)を超えていた
  - 4bytes幅は`long`

## ESP32

- SPIFFSのパス名は`/`とかも合わせて、32Bytes以内にしなければならない
  - さもなくば、ESP32 Sketch Data Uploadで`SPIFFS_write error(-10010): unknown`エラーが吐かれる
