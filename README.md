# Block

ふろっくの各ブロックのファームウェアです。

- ブロックの種類について
  - [blocks.md](/blocks.md)
- ブロック間通信のプロトコル・コマンドについて
  - [protocol.md](/protocol.md)

## フォルダ階層

- Core/
  - コアブロック
- Move/
  - 動作ブロック
- Branch/
  - 分岐ブロック (if)
- Repeat/
  - 繰り返しブロック (for)

## 開発環境

|環境|-|
|:-:|:-|
|OS|Windows 10 Home|
|開発ツール|Arduino IDE, Visual Studio Code + Arduino Extension|

## ライブラリ等

- ATTinyCore
  - Arduino core for ATtiny
  - <https://github.com/SpenceKonde/ATTinyCore>
- Packetizer
  - binary data packer / unpacker
  - <https://github.com/hideakitai/Packetizer>
  - ブロック間通信部で使用
