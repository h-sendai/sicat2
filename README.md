# NEUNET読み出しプログラム

delayed ack, nagleテストのためのベースにするための
再実装。

## プロトコル

1. まず読み出す長さを送る(長さリクエスト)
2. ボードから読み出せるデータ長（単位はワード(1ワード2バイト))が4バイトで返ってくる。
3. データ長が0以外ならデータが読めるのでそのぶん読む。
   データ長が0だったらデータはないのでなにも送られてこない。
4. 1.に戻る。

## フォーマット

### 長さリクエスト (PC -> ボード)
+------+------+------+------+------+------+------+------+
| 0xa3 | 0x00 | 0x00 | 0x00 |                  4096 (*) |
+------+------+------+------+------+------+------+------+
(*) ネットワークバイトオーダー

### データ長 (ボード -> PC)

+------+------+------+------+
|                           |
+------+------+------+------+

4バイト。 ネットワークバイトオーダー 単位はワード(1ワードは2バイト)

### データ

1データ8バイト。先頭1バイトは0x5a。
データがある場合は、データ長のパケットとおなじパケットで送られてくる。
(が長さを読むときに4バイト読めばいいので同一パケットでくるかどうかは
あまり気にする必要はない)。
