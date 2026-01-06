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
#define BTN_LEFT 3   // 左ボタン
#define BTN_RIGHT 2  // 右ボタン
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

const bool MIRROR_X = true;    
const bool ROTATE_180 = false; 


// ==========================================
//   2. 変数・設定
// ==========================================
int hour = 12;    // 時
int minutes = 0;  // 分
int sec  = 0;     // 秒
unsigned long lastTickTime = 0; 

// モード管理
// 0: 時計表示
// 1: 時間設定 (数字が点滅)
// 2: 分設定 (数字が点滅)
int mode = 0; 

// 強制リセット用
unsigned long bothPressedStart = 0;
bool isBothPressed = false;

// フォントデータ
const byte FONT_MAP[][5] = {
  {0x7, 0x5, 0x5, 0x5, 0x7}, // 0
  {0x2, 0x6, 0x2, 0x2, 0x7}, // 1
  {0x7, 0x1, 0x7, 0x4, 0x7}, // 2
  {0x7, 0x1, 0x7, 0x1, 0x7}, // 3
  {0x5, 0x5, 0x7, 0x1, 0x1}, // 4
  {0x7, 0x4, 0x7, 0x1, 0x7}, // 5
  {0x7, 0x4, 0x7, 0x5, 0x7}, // 6
  {0x7, 0x1, 0x2, 0x4, 0x4}, // 7
  {0x7, 0x5, 0x7, 0x5, 0x7}, // 8
  {0x7, 0x5, 0x7, 0x1, 0x7}, // 9
  {0x0, 0x2, 0x0, 0x2, 0x0}  // : (10)
};

// ==========================================
//   3. セットアップ
// ==========================================
void setup() {
  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);

  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.clear();
  FastLED.show();

  tone(BUZZER_PIN, 2000, 100);
}

// ==========================================
//   4. メインループ
// ==========================================
void loop() {
  unsigned long now = millis();

  // --- 1. 強制リセット監視 (最優先) ---
  checkForceReset();
  if (isBothPressed) return; 

  // --- 2. 時計を動かす ---
  // 設定中(モード1,2)以外だけ時間を進める
  if (mode == 0) {
    if (now - lastTickTime >= 1000) {
      lastTickTime = now;
      sec++;
      if (sec >= 60) {
        sec = 0;
        minutes++;
        if (minutes >= 60) {
          minutes = 0;
          hour++;
          if (hour >= 24) hour = 0;
        }
      }
    }
  }

  // --- 3. ボタン操作と画面表示 ---
  
  // ボタンの状態読み取り
  bool clickLeft = (digitalRead(BTN_LEFT) == LOW);
  bool clickRight = (digitalRead(BTN_RIGHT) == LOW);

  // ==== モードごとの処理 ====
  
  if (mode == 0) {
    // ★モード0：時計表示
    
    // 左ボタンで「時間設定モード」へ移動
    if (clickLeft) {
      mode = 1; 
      tone(BUZZER_PIN, 1000, 100);
      delay(300); // ★重要：ボタン連打防止の待ち時間
      return;
    }

    // 時計をスクロール表示
    char timeStr[6];
    sprintf(timeStr, "%02d:%02d", hour, minutes);
    scrollText(timeStr, true);
  }
  
  else if (mode == 1) {
    // ★モード1：時間設定 (数字が点滅)
    
    // 右ボタン：時間を進める
    if (clickRight) {
      hour++; if (hour >= 24) hour = 0;
      tone(BUZZER_PIN, 2000, 50);
      delay(200); // 押しすぎ防止
    }
    
    // 左ボタン：決定して「分設定」へ
    if (clickLeft) {
      mode = 2; // 次へ
      tone(BUZZER_PIN, 1000, 100);
      delay(300); // ★重要：連打防止
    }

    // 画面描画
    FastLED.clear();
    
    // コロンは常に表示 (右端)
    leds[XY(7, 2)] = CRGB::White;
    leds[XY(7, 5)] = CRGB::White;

    // 数字は点滅させる (500msごとにON/OFF)
    if ((millis() / 500) % 2 == 0) {
      drawTwoDigits(hour, CRGB::Magenta, 0); // 表示ON
    }
    
    FastLED.show();
  }
  
  else if (mode == 2) {
    // ★モード2：分設定 (数字が点滅)

    // 右ボタン：分を進める
    if (clickRight) {
      minutes++; if (minutes >= 60) minutes = 0;
      tone(BUZZER_PIN, 2000, 50);
      delay(200);
    }

    // 左ボタン：決定して「時計表示」に戻る
    if (clickLeft) {
      mode = 0; // 完了！
      sec = 0;  // 秒を0リセット
      tone(BUZZER_PIN, 1000, 100);
      tone(BUZZER_PIN, 2000, 300); // 完了音
      delay(300); // ★重要：連打防止
    }

    // 画面描画
    FastLED.clear();
    
    // コロンは常に表示 (左端)
    leds[XY(0, 2)] = CRGB::White;
    leds[XY(0, 5)] = CRGB::White;

    // 数字は点滅させる
    if ((millis() / 500) % 2 == 0) {
      drawTwoDigits(minutes, CRGB::Cyan, 1); // 表示ON
    }

    FastLED.show();
  }
}

// ==========================================
//   便利関数
// ==========================================

// --- 強制リセット監視 ---
void checkForceReset() {
  if (digitalRead(BTN_LEFT) == LOW && digitalRead(BTN_RIGHT) == LOW) {
    if (!isBothPressed) {
      bothPressedStart = millis();
      isBothPressed = true;
    } else {
      if (millis() - bothPressedStart > 5000) { // 5秒
        FastLED.clear();
        fill_solid(leds, NUM_LEDS, CRGB::White); 
        FastLED.show();
        tone(BUZZER_PIN, 1500, 500);
        delay(500);
        reset_usb_boot(0, 0); 
      }
    }
  } else {
    isBothPressed = false;
  }
}

// --- 座標変換 ---
int XY(int x, int y) {
  if (MIRROR_X) x = 7 - x;
  if (ROTATE_180) { x = 7 - x; y = 7 - y; }
  
  if (y % 2 == 0) return y * 8 + x;
  else            return y * 8 + (7 - x);
}

// --- 2桁の数字を描画 ---
// offsetX: 0なら左寄せ、1なら右寄せ
void drawTwoDigits(int num, CRGB color, int offsetX) {
  int tens = num / 10; 
  int ones = num % 10; 
  int startX = offsetX;

  for(int y=0; y<5; y++) {
    for(int x=0; x<3; x++) {
      if(bitRead(FONT_MAP[tens][y], 2-x)) leds[XY(startX + x, y+2)] = color;
    }
  }
  for(int y=0; y<5; y++) {
    for(int x=0; x<3; x++) {
      if(bitRead(FONT_MAP[ones][y], 2-x)) leds[XY(startX + 4 + x, y+2)] = color;
    }
  }
}

// --- 文字列スクロール ---
void scrollText(const char* text, bool rainbow) {
  int len = strlen(text);
  int pixelLen = len * 4 + 8; 

  for (int cur = 8; cur > -pixelLen; cur--) {
    
    // 左ボタンが押されたら即座に設定モードへ
    if (digitalRead(BTN_LEFT) == LOW) {
      mode = 1; // 時間設定へ
      tone(BUZZER_PIN, 1000, 100);
      delay(300); // 連打防止
      return; 
    }

    checkForceReset();
    if (isBothPressed) return;

    FastLED.clear();
    
    int cursor = cur;
    for (int i=0; i<len; i++) {
      char c = text[i];
      int idx = -1;
      if (c >= '0' && c <= '9') idx = c - '0';
      else if (c == ':') idx = 10;

      if (idx >= 0) {
        for(int x=0; x<3; x++) {
          int dx = cursor + x;
          if(dx >= 0 && dx < 8) { 
            for(int y=0; y<5; y++) {
              if(bitRead(FONT_MAP[idx][y], 2-x)) {
                if(rainbow) leds[XY(dx, y+2)] = CHSV((dx*10) + (millis()/5), 255, 255);
                else        leds[XY(dx, y+2)] = CRGB::White;
              }
            }
          }
        }
        cursor += 4; 
      }
    }
    FastLED.show();
    delay(100); 
  }
}