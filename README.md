# Emoji-LED-display
「ｽﾀｯｸｻﾝの心臓」こと、EmojiLEDディスプレイの開発者用のサンプルコード集です。

RP2040を使用した自作のLEDマトリクスデバイスです。
現在はArduino IDE用のサンプルコードを公開しており、今後MicroPython版も追加予定です。

![名称未設定のデザイン (3)](https://github.com/user-attachments/assets/04c3c641-7176-4c2a-9f44-7c9f55dc5197)

![写真 2025-11-14 11 39 27](https://github.com/user-attachments/assets/4c1f6c57-f80f-40bd-b46a-d1c7ebbf3164)


## 🌟 特徴 (Features)
* ラズベリーパイpicoと同じように扱える(YD RP2040/RP2040 Zeroをコントローラとして使用)
* 8x8LEDマトリクス、２ボタン、ブザー、groveコネクタ２つ、電池ボックスがAll-in-One
* 3Dプリント製のオリジナルケースでウェアラブル可

## 🔌 ピン配置 (Pinout)

各機能に割り当てられているRP2040 Zeroのピン番号(GPIO)です。
バージョンによって異なる箇所があります。
お手持ちのものが**どちらのバージョンかは imagesフォルダにて**ご確認ください。

| 機能 (Function) | 旧版 (Ver1.0) | 新版 (Ver1.5) | 備考 (Note) |
| :--- | :---: | :---: | :--- |
| **LED Matrix Data** | GP27 | GP28 | LED信号線 (DIN) |
| **Button Left** | GP10 | GP3 | 左側のボタン |
| **Button Right** | GP21 | GP2 | 右側のボタン |
| **Buzzer** | GP15 | GP15| - |
| **Grove Left** | - | 3V3,GP0,1 | UART0,I2C0|
| **Grove Right** | - | 3V3,GP26,27 | Analog,I2C1 |

## 💻 サンプルコードの使い方 - Arduino版 (Usage - Arduino)

### 1. 環境設定
このコードを使用するには、Arduino IDEに以下のボードマネージャとライブラリをインストールしてください。

* **Board Manager:**
    * `Raspberry Pi Pico/RP2040` by Earle F. Philhower, III
* **Libraries:**
    * `FastLED 3.10.3` by Daniel Garcia

### 2. インストール手順
1. このリポジトリをダウンロード（Clone）します。
2. `samples/arduino/` フォルダ内の `.ino` ファイルをArduino IDEで開きます。
3. ソースコードの１０行目でお持ちのディスプレイのバージョンに合わせてコメントアウトしてください(デフォルトは新バージョン1.5用になっています)　※Ear-clip_Heart_Rate_Sensor　心拍計のプログラムはgroveのセンサー接続が必要なので新バージョンのみで使用可能です。
4. ツールメニューから以下のように設定します。
    * **Board:**　ver1.0 --> Raspberry Pi Pico , ver1.5--> Waveshare RP2040 Zero 
    * **Port:** (認識されているポートを選択)
5. 「書き込み」ボタンを押してアップロードします。

## 🐍 使い方 - MicroPython版 (Usage - MicroPython)
*🚧 Coming Soon...*
MicroPython用のコードも公開予定です。

## 📂 フォルダ構成
* `examples/arduino/` - Arduino IDE用ソースコード
* `examples/micropython/` - (予定) MicroPython用コード
* `images/` - 新旧バージョンの比較画像など
* `firmware/` - 購入時の初期ファームウェア

## 🛠️ 使用している主な部品 (Hardware Requirements)
* **MCU旧版:** YD-RP2040(raspberry pi picoのジェネリック上位互換版)
* **MCU新版:** Waveshare RP2040 Zero 互換品
* **LED:** WS2812B LEDテープ / 8x8 マトリクスパネル

## 🚀 初期ファームウェア (firmware)
書き換えたプログラムを元に戻したい場合など、こちらのファームウェアをご利用ください。
ver1.0をお持ちの方はこちらを一度書き込むと機能が最新版にアップデートされることになります。(絵文字の種類が５０個に増加。ランダム表示機能追加）

### インストール手順 (Drag & Drop)
特別なソフトは不要です。USBメモリと同じ感覚で書き込めます。

1. **ダウンロード:**
２種類のファームウェアがあります。バージョン1とバージョン1.5とでピン設定が異なるためです。それ以外は同様の機能となっています。
   このリポジトリの `firmware` フォルダにある該当するファイルをクリックし、ダウンロードボタン(Download raw file)を押してPCに保存します。
3. **接続モード:**
   RP2040 Zeroの **BOOTボタン** を押しながら、PCにUSB接続します。
   （PCに `RPI-RP2` という名前のドライブとして認識されます）
※2025年ご購入の方は最初だけネジを外して分解し、BOOTボタンへのアクセスが必要です。
※2026年以降ご購入の方、または当リポジトリ内のサンプルコードを一度でも書き込み済みの方につきましては、**デバイス起動中に左右のボタンを同時に５秒間以上押す**と強制リセット＆書き込みモードに入れるようにしております。分解せずに次の手順にお進みください。
4. **書き込み:**
   ダウンロードした `.uf2` ファイルを、その `RPI-RP2` ドライブの中に**ドラッグ・アンド・ドロップ**します。
5. **完了:**
   自動的にドライブが消え、再起動してデバイスが動作を開始します。

### 🕹️ 操作方法 (Controls)
書き込み後のデバイスの操作方法です。
製品版の操作マニュアル
https://youtu.be/zExkCggxYq8 

※左右ボタン同時押し1.5秒間でランダムモードON、同時押し５秒以上で強制リセット＆書き込みモードです。それぞれに対しブザー音が鳴りますので混同しないようにお願いします。

## 📜 ライセンス (License)
* Software (Firmware & Examples): MIT License

    * ファームウェア(.uf2)やサンプルコード(.ino)は、MITライセンスの下で自由に利用・改変・再配布が可能です。

* Hardware (Case Design): CC BY-SA 4.0

    * 筐体データ(.stl)は、[hexatron]の作品をベースに、リミックスを行ったものです。CC BY-SAライセンスが適用されます。

## 👤 作者 (Author)
* [らて]
* X(Twitter): [@cofelatte_latte]

