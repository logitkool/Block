# Block

- ブロックには下記の種類があります。
  - 動作ブロック
  - 分岐ブロック
  - 繰り返しブロック
  - 無/延長(仮称) ブロック
  - コアブロック

※ 無ブロックはATtinyが搭載されていないため、コアブロックには識別されずRoleIDも割り当てられていない

## ブロックの種類とRoleIDの1byte目の関係 (TypeID)

|RoleID_H|ブロックの種類|
|:-:|:-|
|0|未割り当て(管理用)|
|1|動作 (前後)|
|2|動作 (左右)|
|3|動作 (手)|
|4|動作 (首)|
|5|動作|
|6|動作|
|7|動作|
|8|コアブロック|
|9|分岐 (連番)|
|A|分岐|
|B|分岐|
|C|繰り返し (連番)|
|D|繰り返し|
|E|繰り返し|
|F|未割り当て(テスト用)|

## 動作ブロック (0x10 - 0x7F)

|動作|RoleID|
|:-:|:-|
|**(0x1-)**|**前/後 移動**|
|前|0x11|
|後|0x19|
|**(0x2-)**|**左/右 転回**|
|左に(少し)回る|0x21|
|右に(少し)回る|0x29|
|左に大きく回る (左折)|0x22|
|右に大きく回る (右折)|0x2A|
|**(0x3-)**|**手を振る**|
|左手を振る|0x31|
|右手を振る|0x35|
|両手を振る|0x39|
|**(0x4-)**|**首を振る**|
|首を左に振る|0x41|
|首を右に振る|0x49|

## コアブロック (0x80 - 0x8F)

|種類|RoleID|
|:-:|:-|
|ふろっく純正コアブロック|0x81|

## 分岐ブロック (0x90 - 0xBF)

|種類|RoleID|
|:-:|:-|
|明るさ|0x91|
|モノの有無 (遠/近)|0x92|

## 繰り返しブロック (0xC0 - 0xEF)

|種類|RoleID|
|:-:|:-|
|**(0xC-)**|**回数指定繰り返し**|
|回数:1|0xC1|
|回数:2|0xC2|
|回数:3|0xC3|
|回数:4|0xC4|
|回数:5|0xC5|
