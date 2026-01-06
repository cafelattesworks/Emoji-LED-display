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
#define BTN_LEFT 3   // 左ボタン：再生/停止
#define BTN_RIGHT 2  // 右ボタン：曲送り
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
//   2. 変数 (数字や状態を入れておく箱)
// ==========================================
int dotCount = 0; // 今、光っているドットの数

// 強制リセット機能用の変数
unsigned long bothPressedStart = 0; 
bool isBothPressed = false;         

// ==========================================
//   3. セットアップ (起動時に1回だけ動く)
// ==========================================
void setup() {
  // ボタンの設定 (INPUT_PULLUP: 押すとLOWになる仕組み)
  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);

  // LEDの準備
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.clear(); // 画面を消す
  FastLED.show();

  // 起動音 (ピロリン♪)
  tone(BUZZER_PIN, 1000, 100); delay(100);
  tone(BUZZER_PIN, 2000, 200);
}

// ==========================================
//   4. メインループ (ここをずっと繰り返す)
// ==========================================
void loop() {
  // ボタンの状態をチェック (押すとLOW、離すとHIGH)
  bool isLeft = (digitalRead(BTN_LEFT) == LOW);
  bool isRight = (digitalRead(BTN_RIGHT) == LOW);

  // ------------------------------------------------
  // ★最優先：強制リセット機能 (左右同時5秒押し)
  // ------------------------------------------------
  if (isLeft && isRight) {
    if (!isBothPressed) {
      bothPressedStart = millis(); // 時間計測スタート
      isBothPressed = true;
    } else {
      // 5秒(5000ms)経過したらリセット実行
      if (millis() - bothPressedStart > 5000) {
        fill_solid(leds, NUM_LEDS, CRGB::Black); FastLED.show(); // 真っ暗
        tone(BUZZER_PIN, 1500, 500); delay(500); // 合図の音
        reset_usb_boot(0, 0); // ★強制的に書き込みフォルダを開く
      }
    }
    return; // 同時押し中は以下の処理をしない
  } else {
    isBothPressed = false;
  }

  // ------------------------------------------------
  // 左ボタンの処理：ドットを増やす
  // ------------------------------------------------
  if (isLeft) {
    if (dotCount < NUM_LEDS) {
      dotCount++; // 数を増やす
      updateDisplay(); // 画面更新
      tone(BUZZER_PIN, 1000, 50); // 音 (高め)
    }
    delay(150); // 連続入力を防ぐための待ち時間
  }

  // ------------------------------------------------
  // 右ボタンの処理：ドットを減らす
  // ------------------------------------------------
  if (isRight) {
    if (dotCount > 0) {
      dotCount--; // 数を減らす
      updateDisplay(); // 画面更新
      tone(BUZZER_PIN, 500, 50); // 音 (低め)
    }
    delay(150); // 連続入力を防ぐための待ち時間
  }
}

// ==========================================
//   画面描画の関数
// ==========================================
void updateDisplay() {
  FastLED.clear(); // まず画面を真っ暗にする

  // dotCountの数だけLEDを光らせる
  for (int i = 0; i < dotCount; i++) {
    // 順番に色を変える (虹色)
    // CHSV(色相, 彩度, 明度) を使っています
    leds[i] = CHSV(i * 5, 255, 255); 
  }

  FastLED.show(); // 実際に光らせる命令
}