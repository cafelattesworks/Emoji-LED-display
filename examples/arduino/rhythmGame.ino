#include <FastLED.h>
// ★重要：Picoを強制リセットさせるために必要なライブラリです
#include <pico/bootrom.h>
// ==========================================
//   ハードウェア設定
// ==========================================
// ★設定: 使用する基板のバージョンを選んでください
// 自分の持っている基板の方の行の「//」を外し、もう片方はコメントアウトしてください。

#define VERSION_NEW  // 新版ver1.5を使う場合
// #define VERSION_OLD  // 旧版ver1.0を使う場合

#if defined(VERSION_NEW)
// 新版のピン配置
#define BTN_LEFT 3   // 左ボタン：
#define BTN_RIGHT 2  // 右ボタン：
#define LED_PIN 28   // LEDマトリクスの信号ピン

#elif defined(VERSION_OLD)
// 旧版のピン配置
#define BTN_LEFT 10   // 左ボタン：再生/停止
#define BTN_RIGHT 21  // 右ボタン：曲送り
#define LED_PIN 27    // LEDマトリクスの信号ピン
#else
#error "バージョンが選択されていません。冒頭の#defineを確認してください。"
#endif
// ==========================================

#define NUM_LEDS 64       // LEDの数 (8x8)
#define BRIGHTNESS 20     // 明るさ (0-255)
#define LED_TYPE WS2812B  // LEDの種類
#define COLOR_ORDER GRB   // 色の並び順
#define BUZZER_PIN 15     // ブザーのピン

CRGB leds[NUM_LEDS];  // LED制御用の配列

// ==========================================
//   設定・定数定義
// ==========================================

// --- 音程の定義 (周波数 Hz) ---
#define NOTE_C4  262 
#define NOTE_D4  294 
#define NOTE_E4  330 
#define NOTE_F4  349 
#define NOTE_G4  392 
#define NOTE_A4  440 
#define NOTE_B4  494 
#define NOTE_C5  523 
#define REST     0   

// ==========================================
//   曲データ (童謡メドレー)
// ==========================================
// 1. カエルの歌 -> 2. チューリップ -> 3. きらきら星
int melody[] = {
  // --- 1. カエルの歌 ---
  NOTE_C4, NOTE_D4, NOTE_E4, NOTE_F4, NOTE_E4, NOTE_D4, NOTE_C4, REST, 
  NOTE_E4, NOTE_F4, NOTE_G4, NOTE_A4, NOTE_G4, NOTE_F4, NOTE_E4, REST, 
  NOTE_C4, REST,    NOTE_C4, REST,    NOTE_C4, REST,    NOTE_C4, REST, 
  NOTE_C4, NOTE_C4, NOTE_D4, NOTE_D4, NOTE_E4, NOTE_E4, NOTE_F4, NOTE_F4, 
  NOTE_E4, NOTE_D4, NOTE_C4, REST, REST, REST, REST, REST, 

  // --- 2. チューリップ ---
  NOTE_C4, NOTE_D4, NOTE_E4, REST, NOTE_C4, NOTE_D4, NOTE_E4, REST, 
  NOTE_G4, NOTE_E4, NOTE_D4, NOTE_C4, NOTE_D4, NOTE_E4, NOTE_D4, REST, 
  NOTE_C4, NOTE_D4, NOTE_E4, REST, NOTE_C4, NOTE_D4, NOTE_E4, REST, 
  NOTE_G4, NOTE_E4, NOTE_D4, NOTE_C4, NOTE_D4, NOTE_E4, NOTE_C4, REST, 
  NOTE_G4, NOTE_G4, NOTE_E4, NOTE_G4, NOTE_A4, NOTE_A4, NOTE_G4, REST, 
  NOTE_E4, NOTE_E4, NOTE_D4, NOTE_D4, NOTE_C4, REST, REST, REST, REST, REST, 

  // --- 3. きらきら星 ---
  NOTE_C4, NOTE_C4, NOTE_G4, NOTE_G4, NOTE_A4, NOTE_A4, NOTE_G4, REST, 
  NOTE_F4, NOTE_F4, NOTE_E4, NOTE_E4, NOTE_D4, NOTE_D4, NOTE_C4, REST, 
  NOTE_G4, NOTE_G4, NOTE_F4, NOTE_F4, NOTE_E4, NOTE_E4, NOTE_D4, REST, 
  NOTE_G4, NOTE_G4, NOTE_F4, NOTE_F4, NOTE_E4, NOTE_E4, NOTE_D4, REST, 
  NOTE_C4, NOTE_C4, NOTE_G4, NOTE_G4, NOTE_A4, NOTE_A4, NOTE_G4, REST, 
  NOTE_F4, NOTE_F4, NOTE_E4, NOTE_E4, NOTE_D4, NOTE_D4, NOTE_C4, REST, 
  
  // ★フリーズ対策：最後に余白を十分に入れる
  REST, REST, REST, REST, REST, REST, REST, REST, REST, REST
};

int totalNotes = sizeof(melody) / sizeof(int);

// ==========================================
//   ゲーム用変数
// ==========================================
byte laneType[8];  
int  laneTone[8];  

int scoreIndex = 0;       
long lastMoveTime = 0;    
int gameSpeed = 350; 

bool lastLeftState = HIGH;
bool lastRightState = HIGH;

// ★追加機能用の変数（同時押し時間の計測用）
unsigned long bothPressedStart = 0; // 同時押しを始めた時間
bool isBothPressed = false;         // 今、同時押し中かどうか

// ==========================================
//   セットアップ
// ==========================================
void setup() {
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  
  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);

  // スタート合図
  tone(BUZZER_PIN, 1000, 100); delay(150);
  tone(BUZZER_PIN, 1500, 100); delay(150);
  tone(BUZZER_PIN, 2000, 300);
}

// ==========================================
//   メインループ
// ==========================================
void loop() {
  unsigned long currentTime = millis();

  // ------------------------------------------
  // ★ボタン入力の読み込み
  // ------------------------------------------
  bool rawLeft = digitalRead(BTN_LEFT);
  bool rawRight = digitalRead(BTN_RIGHT);

  // ------------------------------------------
  // ★追加機能：10秒同時押しで強制書き込みモードへ
  // ------------------------------------------
  if (rawLeft == LOW && rawRight == LOW) {
    // 両方押されている時
    if (isBothPressed == false) {
      // 押された瞬間なら、時間を記録開始
      bothPressedStart = currentTime;
      isBothPressed = true;
    } else {
      // 押し続けている場合、時間を計算
      unsigned long duration = currentTime - bothPressedStart;
      
      // 5秒（5000ミリ秒）を超えたらリセット発動
      if (duration > 5000) {
        FastLED.clear(); FastLED.show(); // LEDを消す
        tone(BUZZER_PIN, 1500, 500);     // 合図の音
        delay(500);                      // 音が鳴るのを少し待つ
        
        // ★ここでBOOTSELモード（書き込みモード）に強制ジャンプ！
        reset_usb_boot(0, 0); 
      }
    }
  } else {
    // 片方でも離したらカウントリセット
    isBothPressed = false;
  }

  // ------------------------------------------
  // 1. 時間進行処理 (ゲーム画面の更新)
  // ------------------------------------------
  if (currentTime - lastMoveTime > gameSpeed) {
    lastMoveTime = currentTime;

    for (int i = 0; i < 7; i++) {
      laneType[i] = laneType[i+1];
      laneTone[i] = laneTone[i+1];
    }

    if (scoreIndex < totalNotes) {
      int nextNote = melody[scoreIndex];
      if (nextNote > 0) {
        laneTone[7] = nextNote;
        laneType[7] = (scoreIndex % 2 == 0) ? 1 : 2;
      } else {
        laneType[7] = 0;
        laneTone[7] = 0;
      }
      scoreIndex++;
    } else {
      scoreIndex = 0;
      laneType[7] = 0;
      delay(2000); 
    }
    updateDisplay();
  }

  // ------------------------------------------
  // 2. ゲームのボタン判定（押しっぱなし対策版）
  // ------------------------------------------
  
  // ★左右入れ替え処理（配線が逆の場合の対策）
  bool currentLeft = rawRight; 
  bool currentRight = rawLeft;  

  // 左ボタン判定（押した瞬間だけ反応）
  if (lastLeftState == HIGH && currentLeft == LOW) {
    if (laneType[0] == 1) { 
      tone(BUZZER_PIN, laneTone[0], 120); 
      leds[XY(2, 0)] = CRGB::White;       
      FastLED.show();
      laneType[0] = 0; 
    }
  }
  
  // 右ボタン判定
  if (lastRightState == HIGH && currentRight == LOW) {
    if (laneType[0] == 2) {
      tone(BUZZER_PIN, laneTone[0], 120);
      leds[XY(5, 0)] = CRGB::White;
      FastLED.show();
      laneType[0] = 0;
    }
  }

  lastLeftState = currentLeft;
  lastRightState = currentRight;
}

// ==========================================
//   画面描画
// ==========================================
void updateDisplay() {
  FastLED.clear();
  for (int y = 0; y < 8; y++) {
    byte type = laneType[y];
    if (type == 1) leds[XY(2, y)] = CRGB::Red;
    if (type == 2) leds[XY(5, y)] = CRGB::Blue;
  }
  // 判定ライン
  leds[XY(1, 0)] = CRGB(10, 10, 10);
  leds[XY(6, 0)] = CRGB(10, 10, 10);
  FastLED.show();
}

// ==========================================
//   ジグザグ対応座標変換
// ==========================================
int XY(int x, int y) {
  if (y % 2 == 0) return (y * 8) + x;
  else            return (y * 8) + (7 - x);
}