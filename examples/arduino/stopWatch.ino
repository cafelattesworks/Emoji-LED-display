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

// ★★★ 画面の向き設定 ★★★
const bool MIRROR_X = true;    // 左右反転するか？
const bool ROTATE_180 = false; // 180度回転するか？

// 文字が流れる速さ
#define SCROLL_SPEED 100

// ==========================================
//  フォントデータ (3x5ドット)
// ==========================================
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
  {0x0, 0x2, 0x0, 0x2, 0x0}, // : (10)
  // A-Z (11~)
  {0x7, 0x5, 0x7, 0x5, 0x5}, // A
  {0x6, 0x5, 0x6, 0x5, 0x6}, // B
  {0x7, 0x4, 0x4, 0x4, 0x7}, // C
  {0x6, 0x5, 0x5, 0x5, 0x6}, // D
  {0x7, 0x4, 0x6, 0x4, 0x7}, // E
  {0x7, 0x4, 0x6, 0x4, 0x4}, // F
  {0x7, 0x4, 0x5, 0x5, 0x7}, // G
  {0x5, 0x5, 0x7, 0x5, 0x5}, // H
  {0x7, 0x2, 0x2, 0x2, 0x7}, // I
  {0x1, 0x1, 0x1, 0x5, 0x2}, // J
  {0x5, 0x5, 0x6, 0x5, 0x5}, // K
  {0x4, 0x4, 0x4, 0x4, 0x7}, // L
  {0x5, 0x7, 0x5, 0x5, 0x5}, // M
  {0x5, 0x5, 0x7, 0x5, 0x5}, // N
  {0x7, 0x5, 0x5, 0x5, 0x7}, // O
  {0x7, 0x5, 0x7, 0x4, 0x4}, // P
  {0x7, 0x5, 0x5, 0x6, 0x3}, // Q
  {0x7, 0x5, 0x6, 0x5, 0x5}, // R
  {0x3, 0x4, 0x7, 0x1, 0x6}, // S
  {0x7, 0x2, 0x2, 0x2, 0x2}, // T
  {0x5, 0x5, 0x5, 0x5, 0x7}, // U
  {0x5, 0x5, 0x5, 0x5, 0x2}, // V
  {0x5, 0x5, 0x5, 0x7, 0x5}, // W
  {0x5, 0x5, 0x2, 0x5, 0x5}, // X
  {0x5, 0x5, 0x2, 0x2, 0x2}, // Y
  {0x7, 0x1, 0x2, 0x4, 0x7}, // Z
  {0x0, 0x0, 0x0, 0x0, 0x0}, // スペース (37)
  {0x7, 0x1, 0x2, 0x0, 0x2}  // ? (38)
};

// ==========================================
//  変数管理
// ==========================================

enum Mode { MODE_STOPWATCH, MODE_TIMER };
Mode currentMode = MODE_STOPWATCH;

enum State { 
  ST_TITLE,       // タイトル
  ST_SW_STOPPED,  // (SW)停止
  ST_SW_RUNNING,  // (SW)計測中
  ST_TM_SET_MIN,  // (TM)分設定
  ST_TM_SET_SEC,  // (TM)秒設定
  ST_TM_READY,    // (TM)スタート待ち
  ST_TM_RUNNING,  // (TM)実行中
  ST_TM_ALARM     // (TM)時間切れ
};
State currentState = ST_TITLE;

// ストップウォッチ用
unsigned long swStartTime = 0;
unsigned long swElapsedTime = 0;
int lastNotifyMinute = 0;

// タイマー用
int tmSetMin = 0;
int tmSetSec = 0;
unsigned long tmTargetTime = 0;
unsigned long tmTotalDuration = 0; // セットした合計時間（ゲージ計算用）

// 演出用
uint8_t gHue = 0; 
String scrollMsg = "";
int scrollCursor = 8; 
bool isBlinking = true;

// 救済措置用
unsigned long bothPressedStart = 0; 

// ==========================================
//  ボタン管理クラス
// ==========================================
class Button {
    int pin;
    bool lastState;
    unsigned long pressTime;
    bool processedLong;
  public:
    Button(int p) : pin(p), lastState(HIGH), pressTime(0), processedLong(false) {}
    void init() { pinMode(pin, INPUT_PULLUP); }
    
    int check() {
      int res = 0;
      bool curr = digitalRead(pin);
      unsigned long now = millis();
      
      if (curr == LOW && lastState == HIGH) { pressTime = now; processedLong = false; }
      else if (curr == LOW && lastState == LOW) {
        if (!processedLong && (now - pressTime > 1000)) { res = 2; processedLong = true; } 
      }
      else if (curr == HIGH && lastState == LOW) {
        if (!processedLong && (now - pressTime > 50)) res = 1; 
      }
      lastState = curr;
      return res;
    }
};

Button btnRight(BTN_RIGHT);
Button btnLeft(BTN_LEFT);

// プロトタイプ宣言
void setPixelXY(int x, int y, CRGB c);
void drawTwoDigits(int num, CRGB color, bool rainbow, int offsetX);
void scrollTextLoop(String text, CRGB color, bool rainbow);
void playTone(int freq, int duration);

// ==========================================
//  セットアップ
// ==========================================
void setup() {
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.clear();
  btnRight.init(); btnLeft.init();
  pinMode(BUZZER_PIN, OUTPUT);
  playTone(2000, 100);
  currentState = ST_TITLE;
}

// ==========================================
//  メインループ
// ==========================================
void loop() {
  unsigned long now = millis();

  // --------------------------------------------------------
  // ★救済措置: 左右同時5秒押しでBOOTSELモードへ
  // --------------------------------------------------------
  if (digitalRead(BTN_LEFT) == LOW && digitalRead(BTN_RIGHT) == LOW) {
    if (bothPressedStart == 0) bothPressedStart = now; 
    else {
      if (now - bothPressedStart > 5000) {
        tone(BUZZER_PIN, 1500, 500);
        FastLED.clear(); FastLED.show();
        delay(500);
        reset_usb_boot(0, 0); 
      }
    }
  } else {
    bothPressedStart = 0;
  }
  // --------------------------------------------------------

  int actRight = btnRight.check();
  int actLeft  = btnLeft.check();
  EVERY_N_MILLISECONDS(20) { gHue++; }

  // ------------------------------------
  //  【タイトル画面】
  // ------------------------------------
  if (currentState == ST_TITLE) {
    while(digitalRead(BTN_LEFT) == LOW) { delay(10); } 
    delay(200);

    if (currentMode == MODE_STOPWATCH) scrollTextLoop(" STOPWATCH ", CRGB::Cyan, true);
    else scrollTextLoop(" TIMER ", CRGB::Orange, true);

    if (currentMode == MODE_STOPWATCH) {
      currentState = ST_SW_STOPPED;
      swElapsedTime = 0;
      scrollMsg = "00:00"; 
    } else {
      currentState = ST_TM_SET_MIN;
      tmSetMin = 0; tmSetSec = 0;
    }
    scrollCursor = 8;
    return;
  }

  // ------------------------------------
  //  【共通】モード切替
  // ------------------------------------
  if (actLeft == 2) { 
    playTone(1500, 100); playTone(2500, 100);
    FastLED.clear(); FastLED.show(); 
    currentMode = (currentMode == MODE_STOPWATCH) ? MODE_TIMER : MODE_STOPWATCH;
    currentState = ST_TITLE; 
    return;
  }

  // ==========================================
  //  モード1: ストップウォッチ
  // ==========================================
  if (currentMode == MODE_STOPWATCH) {
    
    if (actRight == 1) { // Start/Stop
      if (currentState == ST_SW_STOPPED) {
        currentState = ST_SW_RUNNING;
        swStartTime = now - swElapsedTime;
        lastNotifyMinute = (swElapsedTime / 1000) / 60;
        playTone(2000, 50);
      } else {
        currentState = ST_SW_STOPPED;
        swElapsedTime = now - swStartTime;
        playTone(1000, 50);
        
        int m = (swElapsedTime / 1000) / 60;
        int s = (swElapsedTime / 1000) % 60;
        char buf[12]; sprintf(buf, "%02d:%02d", m, s);
        scrollMsg = String(buf);
        scrollCursor = 8;
      }
    }
    if (actLeft == 1 && currentState == ST_SW_STOPPED) { // Reset
      swElapsedTime = 0;
      scrollMsg = "00:00";
      scrollCursor = 8;
      playTone(3000, 50);
    }

    if (currentState == ST_SW_RUNNING) {
      unsigned long t = now - swStartTime;
      int m = (t / 1000) / 60;
      int s = (t / 1000) % 60;

      if (m > lastNotifyMinute) {
        lastNotifyMinute = m;
        char buf[12]; sprintf(buf, "%02d:00", m);
        playTone(2000, 100);
        scrollTextLoop(String(buf), CRGB::Magenta, true);
      }

      FastLED.clear();
      drawTwoDigits(s, CRGB::White, true, 0); 
      int msec = (t / 100) % 10;
      setPixelXY(map(msec, 0, 9, 0, 7), 7, CRGB::Red);
      FastLED.show();
    } 
    else { 
      EVERY_N_MILLISECONDS(SCROLL_SPEED) {
        FastLED.clear();
        int cursor = scrollCursor;
        for(int i=0; i<scrollMsg.length(); i++) {
           char c = scrollMsg.charAt(i);
           int idx = -1;
           if(c>='0' && c<='9') idx = c-'0';
           else if(c==':') idx = 10;
           
           if(idx>=0) {
             for(int x=0; x<3; x++) {
               int dx = cursor + x;
               if(dx>=0 && dx<8) {
                 for(int y=0; y<5; y++) {
                   if(bitRead(FONT_MAP[idx][y], 2-x)) 
                     setPixelXY(dx, y+2, CHSV(gHue+dx*10, 200, 255));
                 }
               }
             }
             cursor += 4;
           }
        }
        FastLED.show();
        scrollCursor--;
        if(scrollCursor < -(int)(scrollMsg.length()*4)) scrollCursor=8;
      }
    }
  }

  // ==========================================
  //  モード2: タイマー
  // ==========================================
  else if (currentMode == MODE_TIMER) {
    
    // --- 分設定 ---
    if (currentState == ST_TM_SET_MIN) {
      if (actRight == 1) { tmSetMin++; if(tmSetMin>99) tmSetMin=0; playTone(2000, 20); }
      if (actLeft == 1) { currentState = ST_TM_SET_SEC; playTone(1000, 50); }
      
      EVERY_N_MILLISECONDS(300) { isBlinking = !isBlinking; }
      FastLED.clear();
      if(isBlinking) drawTwoDigits(tmSetMin, CRGB::Magenta, false, 0); 
      setPixelXY(7, 2, CRGB::White); setPixelXY(7, 5, CRGB::White);
      FastLED.show();
    }
    // --- 秒設定 ---
    else if (currentState == ST_TM_SET_SEC) {
      if (actRight == 1) { tmSetSec+=10; if(tmSetSec>=60) tmSetSec=0; playTone(2000, 20); }
      if (actLeft == 1) { 
        currentState = ST_TM_READY; 
        playTone(1000, 50);
        char buf[12]; sprintf(buf, "%02d:%02d?", tmSetMin, tmSetSec);
        scrollMsg = String(buf);
        scrollCursor = 8;
      }
      
      EVERY_N_MILLISECONDS(300) { isBlinking = !isBlinking; }
      FastLED.clear();
      if(isBlinking) drawTwoDigits(tmSetSec, CRGB::Cyan, false, 1);
      setPixelXY(0, 2, CRGB::White); setPixelXY(0, 5, CRGB::White);
      FastLED.show();
    }
    // --- スタート待ち ---
    else if (currentState == ST_TM_READY) {
      if (actRight == 1) { // スタート
        if(tmSetMin==0 && tmSetSec==0) { playTone(500, 200); } 
        else {
          currentState = ST_TM_RUNNING;
          tmTotalDuration = (tmSetMin * 60000) + (tmSetSec * 1000); 
          tmTargetTime = now + tmTotalDuration; 
          playTone(2000, 100);
        }
      }
      if (actLeft == 1) { currentState = ST_TM_SET_MIN; playTone(1000, 50); }

      // 白文字でスクロール（？は赤）
      EVERY_N_MILLISECONDS(SCROLL_SPEED) {
        FastLED.clear();
        int cursor = scrollCursor;
        for(int i=0; i<scrollMsg.length(); i++) {
           char c = scrollMsg.charAt(i);
           int idx = -1;
           if(c>='0' && c<='9') idx = c-'0';
           else if(c==':') idx = 10;
           else if(c=='?') idx = 38; 
           
           if(idx>=0) {
             for(int x=0; x<3; x++) {
               int dx = cursor + x;
               if(dx>=0 && dx<8) {
                 for(int y=0; y<5; y++) {
                   if(bitRead(FONT_MAP[idx][y], 2-x)) {
                     if (idx == 38) setPixelXY(dx, y+2, CRGB::Red); // ?は赤
                     else setPixelXY(dx, y+2, CRGB::White); 
                   }
                 }
               }
             }
             cursor += 4;
           }
        }
        FastLED.show();
        scrollCursor--;
        if(scrollCursor < -(int)(scrollMsg.length()*4)) scrollCursor=8;
      }
    }
    // --- タイマー動作中 ---
    else if (currentState == ST_TM_RUNNING) {
      if (actRight == 1) { // 中断
        currentState = ST_TM_READY; 
        playTone(1000, 50); 
        char buf[12]; sprintf(buf, "%02d:%02d?", tmSetMin, tmSetSec);
        scrollMsg = String(buf);
        scrollCursor = 8;
      }
      
      long rem = tmTargetTime - now; // 残り時間(ミリ秒)
      if (rem <= 0) { currentState = ST_TM_ALARM; rem = 0; }
      
      int rm = (rem / 1000) / 60;
      int rs = (rem / 1000) % 60;
      
      FastLED.clear();

      // --------------------------------------------------
      // ★バーゲージの描画 (一番上の行: y=0)
      // --------------------------------------------------
      // ★修正ポイント: 単純な map() だと切り捨てられてズレるので、
      // 「少しでも残り時間があればLEDを点ける（切り上げ）」計算式にしました。
      // 計算式: (残り時間 * 8 + (合計時間 - 1)) / 合計時間
      // これにより、スタート直後は満タン(8)、終了直前は必ず1、0秒で0になります。
      long barWidth = 0;
      if (rem > 0 && tmTotalDuration > 0) {
        barWidth = (rem * 8 + (tmTotalDuration - 1)) / tmTotalDuration;
      }
      
      // 色の決定 (残り時間に応じて)
      CRGB barColor = CRGB::Green;
      if (rm == 0 && rs < 20) barColor = CRGB::Red;       // 残り20秒未満で赤
      else if (rm == 0 && rs < 40) barColor = CRGB::Yellow; // 残り40秒未満で黄

      // 横一列に描画
      for (int x = 0; x < 8; x++) {
         if (x < barWidth) {
            setPixelXY(x, 0, barColor); // y=0 (一番上) に点を打つ
         }
      }
      // --------------------------------------------------

      // 数字表示 (y=2〜6に表示されるので、y=0のバーとは被りません)
      if (rm > 0) { // 1分以上
        EVERY_N_MILLISECONDS(1000) { isBlinking = !isBlinking; }
        if (isBlinking) drawTwoDigits(rm, CRGB::White, false, 1);
      } else { // 1分切った
        drawTwoDigits(rs, CRGB::White, true, 1);
      }
      FastLED.show();
    }
    // --- アラーム ---
    else if (currentState == ST_TM_ALARM) {
      if ((now / 100) % 2) fill_rainbow(leds, NUM_LEDS, gHue, 7); else FastLED.clear();
      if ((now / 500) % 2 == 0) playTone(2000, 20);
      
      if (actRight || actLeft) { 
        currentState = ST_TM_READY; 
        noTone(BUZZER_PIN);
        char buf[12]; sprintf(buf, "%02d:%02d?", tmSetMin, tmSetSec);
        scrollMsg = String(buf);
        scrollCursor = 8;
      }
      FastLED.show();
    }
  }
}

// ==========================================
//  便利関数
// ==========================================

void scrollTextLoop(String text, CRGB color, bool rainbow) {
  while(digitalRead(BTN_RIGHT)==LOW || digitalRead(BTN_LEFT)==LOW) delay(10);

  int len = text.length() * 4 + 8;
  for (int cur = 8; cur > -len; cur--) {
    FastLED.clear();
    int cursor = cur;
    for(int i=0; i<text.length(); i++) {
       char c = text.charAt(i);
       int idx = -1;
       if(c>='0' && c<='9') idx = c-'0';
       else if(c==':') idx = 10;
       else if(c>='A' && c<='Z') idx = c-'A' + 11;
       else if(c==' ') idx = 37;
       else if(c=='?') idx = 38;

       if(idx!=-1) {
         for(int x=0; x<3; x++) {
           int dx = cursor + x;
           if(dx>=0 && dx<8) {
             for(int y=0; y<5; y++) {
               if(bitRead(FONT_MAP[idx][y], 2-x)) {
                 if(rainbow) setPixelXY(dx, y+2, CHSV(gHue+dx*10, 200, 255));
                 else setPixelXY(dx, y+2, color);
               }
             }
           }
         }
         cursor += 4;
       }
    }
    FastLED.show();
    delay(SCROLL_SPEED);
    if(digitalRead(BTN_RIGHT)==LOW || digitalRead(BTN_LEFT)==LOW) break; 
  }
}

void drawTwoDigits(int num, CRGB color, bool rainbow, int offsetX) {
  int tens = num / 10;
  int ones = num % 10;
  int startX = offsetX; 

  // 数字は y=2 から y=6 の範囲に描画されます (y=0,1 は空くのでバーと被らない)
  for(int x=0; x<3; x++) {
    for(int y=0; y<5; y++) {
      if(bitRead(FONT_MAP[tens][y], 2-x)) {
        if(rainbow) setPixelXY(startX+x, y+2, CHSV(gHue+(startX+x)*10+y*10, 255, 255));
        else setPixelXY(startX+x, y+2, color);
      }
    }
  }
  for(int x=0; x<3; x++) {
    for(int y=0; y<5; y++) {
      if(bitRead(FONT_MAP[ones][y], 2-x)) {
        if(rainbow) setPixelXY(startX+4+x, y+2, CHSV(gHue+(startX+4+x)*10+y*10, 255, 255));
        else setPixelXY(startX+4+x, y+2, color);
      }
    }
  }
}

void setPixelXY(int x, int y, CRGB c) {
  if (MIRROR_X) x = 7 - x;
  if (ROTATE_180) { x = 7 - x; y = 7 - y; }
  if (x < 0 || x > 7 || y < 0 || y > 7) return;
  int i = (y % 2 == 0) ? (y * 8 + x) : (y * 8 + (7 - x));
  leds[i] = c;
}

void playTone(int freq, int duration) {
  tone(BUZZER_PIN, freq, duration);
}