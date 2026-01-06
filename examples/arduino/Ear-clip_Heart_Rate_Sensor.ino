#include <FastLED.h>
// ★重要：Raspberry Pi Picoを強制的に書き込みモードにするためのライブラリ
#include <pico/bootrom.h> 

// ==========================================
//   ハードウェア設定（ここを変えるときは注意）
// ==========================================
#define LED_PIN     28      // LEDマトリクスの信号ピン (GPIO28)
#define NUM_LEDS    64      // LEDの数 (8x8 = 64個)
#define BRIGHTNESS  20      // LEDの明るさ (0〜255)
#define LED_TYPE    WS2812B // LEDテープの種類
#define COLOR_ORDER GRB     // 色データの並び順

// ボタンとブザーのピン設定
#define BTN_RIGHT   2       // 右ボタン (GPIO2)
#define BTN_LEFT    3       // 左ボタン (GPIO3)
#define BUZZER_PIN  15      // ブザー (GPIO15)

// 心拍センサー設定
#define HEART_SENSOR_PIN 1  // センサーをつなぐピン (GPIO1)
// https://wiki.seeedstudio.com/Grove-Ear-clip_Heart_Rate_Sensor/ を参照

// ==========================================
//   変数・定数の定義
// ==========================================

// LEDの色データを管理する配列
CRGB leds[NUM_LEDS];

// 脈拍計測用の変数
// volatileは「割り込み」で急に値が変わる変数につけるおまじないです
volatile unsigned long pulseTimes[21]; // 過去の脈拍時間を記録する配列
volatile unsigned char pulseCounter = 0; // 今、配列の何番目を使っているか
volatile bool pulseDetected = false;     // 「脈が来た！」というフラグ
volatile bool ignorePulse = false;       // ノイズを無視している最中か

unsigned int heartRate = 0;              // 計算された心拍数 (BPM)

// 計測のルール設定
const int MAX_PULSE_INTERVAL = 2000;     // 2秒以上空いたらリセット
volatile unsigned long lastPulseTime = 0; // 最後に脈が来た時間
const unsigned long MIN_PULSE_INTERVAL = 300; // 0.3秒未満の連打は無視
const unsigned long IGNORE_DURATION = 400;    // 検知後0.4秒はセンサーを見ない

// 心電図（ECG）ラインのアニメーション用
uint8_t ecgLine[8] = {3,3,3,3,3,3,3,3}; // 波形の高さデータ
unsigned long lastECGUpdate = 0;        // 最後に波形を動かした時間
const unsigned long ECG_INTERVAL = 100; // 波形が進むスピード (ms)
const uint8_t ECG_BRIGHTNESS = 10;      // ラインの明るさ

// ハートのアニメーション用
bool heartAnimating = false;    // ハートを表示中か
unsigned long lastHeartUpdate = 0;
uint8_t heartBrightness = 0;    // 現在のハートの明るさ
bool heartRising = true;        // 明るくなっている途中か

// 画面モード（心電図 ⇔ 数値表示）
enum DisplayMode { MODE_ECG, MODE_SHOW_BPM };
DisplayMode displayMode = MODE_ECG;
unsigned long modeStartTime = 0;
const unsigned long BPM_DISPLAY_TIME = 4000;  // 数値を表示する時間 (4秒)

// 機能設定
bool soundEnabled = true;  // 音を鳴らすか
uint8_t colorMode = 0;     // 色モード (0:赤, 1:青, 2:ピンク)

// ★緊急リセット用のタイマー変数
unsigned long bothPressedStart = 0;

// ==========================================
//   座標変換関数
// ==========================================
// LEDの配線がジグザグ（ヘビ型）になっているのを直す計算式
uint8_t xyToIndex(uint8_t x, uint8_t y){
  if(y % 2 == 0) return y * 8 + x;       // 偶数行は 左→右
  else return y * 8 + (7 - x);           // 奇数行は 右→左
}

// ==========================================
//   ハートの形データ
// ==========================================
const uint8_t heartCoords[][2] = {
  {1,1},{2,1},{5,1},{6,1},
  {0,2},{1,2},{2,2},{3,2},{4,2},{5,2},{6,2},{7,2},
  {0,3},{1,3},{2,3},{3,3},{4,3},{5,3},{6,3},{7,3},
  {0,4},{1,4},{2,4},{3,4},{4,4},{5,4},{6,4},{7,4},
  {1,5},{2,5},{3,5},{4,5},{5,5},{6,5},
  {2,6},{3,6},{4,6},{5,6},
  {3,7},{4,7}
};

// ==========================================
//   数字フォントデータ (4x7ドット)
// ==========================================
const byte digitFont4x7[10][7] = {
  {0b0110,0b1001,0b1001,0b1001,0b1001,0b1001,0b0110}, //0
  {0b0010,0b0110,0b0010,0b0010,0b0010,0b0010,0b0111}, //1
  {0b0110,0b1001,0b0001,0b0010,0b0100,0b1000,0b1111}, //2
  {0b1110,0b0001,0b0010,0b0001,0b0001,0b1001,0b0110}, //3
  {0b0001,0b0011,0b0101,0b1001,0b1111,0b0001,0b0001}, //4
  {0b1111,0b1000,0b1110,0b0001,0b0001,0b1001,0b0110}, //5
  {0b0110,0b1000,0b1110,0b1001,0b1001,0b1001,0b0110}, //6
  {0b1111,0b0001,0b0010,0b0100,0b0100,0b0100,0b0100}, //7
  {0b0110,0b1001,0b1001,0b0110,0b1001,0b1001,0b0110}, //8
  {0b0110,0b1001,0b1001,0b0111,0b0001,0b0010,0b1100}  //9
};

// コンパイルエラー防止の宣言
void pulseISR();
void updateECG();
void drawECG();
void updateHeart(unsigned long now);
void drawHeart();
void drawNumber(uint8_t number, CRGB color);
void drawDigit(uint8_t digit, uint8_t offsetX, CRGB color);
void arrayInit();
void checkButtons();

// ==========================================
//   セットアップ (電源ON時に1回だけ実行)
// ==========================================
void setup(){
  Serial.begin(9600);
  
  pinMode(HEART_SENSOR_PIN, INPUT);
  pinMode(BTN_LEFT, INPUT_PULLUP);  // 抵抗なしでボタン接続OK
  pinMode(BTN_RIGHT, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);

  // センサー反応時に pulseISR関数 を強制実行する設定
  attachInterrupt(digitalPinToInterrupt(HEART_SENSOR_PIN), pulseISR, RISING);
  
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.clear();
  FastLED.show();
  
  arrayInit();
  randomSeed(millis());
}

// ==========================================
//   メインループ (ずっと繰り返す)
// ==========================================
void loop(){
  unsigned long now = millis();

  // ★ボタン操作のチェック（リセット機能含む）
  checkButtons();

  // ノイズ無視期間が終わったらフラグを下ろす
  if(ignorePulse && (now - lastPulseTime > IGNORE_DURATION)){
    ignorePulse = false;
  }

  // --- 表示モードの管理 ---
  if(displayMode == MODE_SHOW_BPM && now - modeStartTime >= BPM_DISPLAY_TIME){
    displayMode = MODE_ECG; // 時間が来たら心電図に戻る
    FastLED.clear();
  }

  // 数値表示中は、ここから下（心電図など）は処理しない
  if(displayMode == MODE_SHOW_BPM) return;

  // --- 脈拍検知時のアクション ---
  if(pulseDetected){
    pulseDetected = false;
    heartAnimating = true; // アニメ開始
    heartRising = true;
    heartBrightness = 0;
    if(soundEnabled) tone(BUZZER_PIN, 2000, 50); // ピッ
  }

  // --- 描画データの更新 ---
  if(now - lastECGUpdate >= ECG_INTERVAL){
    lastECGUpdate = now;
    updateECG();
  }
  if(heartAnimating){
    updateHeart(now);
  }

  // --- LEDへ反映 ---
  FastLED.clear();
  drawECG();
  drawHeart();
  FastLED.show();
}

// ==========================================
//   ボタン操作の処理
// ==========================================
void checkButtons(){
  // 今の状態を読み取る (LOW = 押されている)
  bool isRightPressed = (digitalRead(BTN_RIGHT) == LOW);
  bool isLeftPressed = (digitalRead(BTN_LEFT) == LOW);

  // ----------------------------------------------------
  // ★ 緊急リセット (左右同時押し)
  // ----------------------------------------------------
  if (isRightPressed && isLeftPressed) {
    // 押し始めの時間を記録
    if (bothPressedStart == 0) {
      bothPressedStart = millis();
    } else {
      // ★ 5秒以上押し続けたらリセット (10000 -> 5000に変更)
      if (millis() - bothPressedStart > 5000) {
        FastLED.clear(); FastLED.show();
        tone(BUZZER_PIN, 1500, 500); // 合図の音
        delay(500);
        reset_usb_boot(0, 0); // 強制的に書き込みモードへ！
      }
    }
    return; // 同時押し中は他の操作をさせない
  } else {
    bothPressedStart = 0; // 指を離したらタイマーリセット
  }

  // ----------------------------------------------------
  // ★ 単押し操作
  // ----------------------------------------------------
  static bool lastBtnRightState = HIGH;
  static bool lastBtnLeftState = HIGH;

  // 右ボタン (音 ON/OFF)
  // 「前は押してなくて(HIGH)、今押した(LOW)」瞬間だけ反応
  if(lastBtnRightState == HIGH && isRightPressed){
    soundEnabled = !soundEnabled;
    if(soundEnabled) tone(BUZZER_PIN, 1000, 100);
    
    // ★ 連続切り替えを防ぐため、少し待つ (200ms)
    delay(200); 
  }
  
  // 左ボタン (色変え)
  if(lastBtnLeftState == HIGH && isLeftPressed){
    colorMode++;
    if(colorMode > 2) colorMode = 0; // 赤→青→ピンク→赤...
    
    // ★ ここも少し待つ
    delay(200);
  }

  // 今の状態を保存（次回のチェック用）
  // delayの後なので、確実に押した後の状態が保存されます
  lastBtnRightState = digitalRead(BTN_RIGHT);
  lastBtnLeftState = digitalRead(BTN_LEFT);
}

// ==========================================
//   割り込み処理 (センサー反応時に自動実行)
// ==========================================
void pulseISR(){
  unsigned long now = millis();
  
  if(ignorePulse) return; // 無視期間なら何もしない
  if(now - lastPulseTime < MIN_PULSE_INTERVAL) return; // 早すぎる反応も無視

  ignorePulse = true;
  lastPulseTime = now;
  pulseTimes[pulseCounter] = now; // 時間を記録
  
  // もし指を離していて久しぶりの反応なら、計算をリセット
  unsigned long interval = (pulseCounter==0)? pulseTimes[pulseCounter]-pulseTimes[20] : pulseTimes[pulseCounter]-pulseTimes[pulseCounter-1];
  if(interval > MAX_PULSE_INTERVAL){
    pulseCounter=0;
    arrayInit();
    return;
  }

  // 20回データが溜まったら心拍数を計算
  if(pulseCounter == 20){
    pulseCounter = 0;
    // 平均間隔からBPM算出
    heartRate = 1200000UL / (pulseTimes[20] - pulseTimes[0]);
    Serial.print("BPM: "); Serial.println(heartRate);

    // 数値表示モードへ
    displayMode = MODE_SHOW_BPM;
    modeStartTime = millis();
    
    // 色を決めて表示
    CRGB numColor = (colorMode == 0) ? CRGB::Red : (colorMode == 1 ? CRGB::Cyan : CRGB::Magenta);
    drawNumber(heartRate, numColor);
  } else {
    pulseCounter++;
  }

  pulseDetected = true; // メインループに知らせる
}

// ==========================================
//   ECGラインの更新・描画
// ==========================================
void updateECG(){
  for(uint8_t i=0; i<7; i++) ecgLine[i] = ecgLine[i+1];
  ecgLine[7] = constrain(3 + random(0, 2), 3, 4); // ランダムに高さを変える
}

void drawECG(){
  for(uint8_t x=0; x<8; x++){
    leds[xyToIndex(x,ecgLine[x])] = CRGB(ECG_BRIGHTNESS, ECG_BRIGHTNESS, ECG_BRIGHTNESS);
  }
}

// ==========================================
//   ハートの更新・描画
// ==========================================
void updateHeart(unsigned long now){
  bool stable = (heartRate != 0);
  // 心拍数が速いときは明るく、遅いときは暗めに調整
  uint8_t maxVal = stable ? map(constrain(heartRate,30,180), 30, 180, 50, 255) : 150;

  if(now - lastHeartUpdate >= 10){
    lastHeartUpdate = now;
    if(heartRising){ // 明るくする
      heartBrightness += 15;
      if(heartBrightness >= maxVal){
        heartBrightness = maxVal;
        heartRising = false;
      }
    } else { // 暗くする
      if(heartBrightness > 10) heartBrightness -= 10;
      else {
        heartBrightness = 0;
        heartAnimating = false;
      }
    }
  }
}

void drawHeart(){
  if(heartBrightness == 0) return;
  uint8_t hue = (colorMode == 1) ? 110 : (colorMode == 2 ? 220 : 0);
  bool stable = (heartRate != 0);
  
  for(uint8_t i=0; i<sizeof(heartCoords)/sizeof(heartCoords[0]); i++){
    leds[xyToIndex(heartCoords[i][0], heartCoords[i][1])] = stable ? CHSV(hue, 255, heartBrightness) : CHSV(hue, 100, heartBrightness);
  }
}

// ==========================================
//   数値の表示処理 (ここを修正しました！)
// ==========================================
void drawNumber(uint8_t number, CRGB color){
  FastLED.clear();
  if(number > 99) number = 99; // 2桁制限

  uint8_t tens = number / 10; // 十の位
  uint8_t ones = number % 10; // 一の位

  // ★修正箇所：
  // 十の位は左(offset 0)、一の位は右(offset 4)という自然な指定に戻しました。
  // その代わり、drawDigitの中で反転処理を行います。
  if(tens > 0) drawDigit(tens, 0, color); 
  drawDigit(ones, 4, color);

  FastLED.show();
}

void drawDigit(uint8_t digit, uint8_t offsetX, CRGB color){
  if(digit > 9) return;
  
  for(uint8_t y=0; y<7; y++){
    byte line = digitFont4x7[digit][y];
    for(uint8_t x=0; x<4; x++){
      if(line & (1 << (3-x))){
        // ★ここが鏡文字直しのポイント
        // 「7 - ...」の計算を入れることで、左右を反転させて描画します。
        // 十の位(offset 0)は「7,6,5,4」番のLEDに、
        // 一の位(offset 4)は「3,2,1,0」番のLEDに描かれます。
        uint8_t drawX = 7 - (offsetX + x);
        
        if(drawX < 8){
           leds[xyToIndex(drawX, y)] = color;
        }
      }
    }
  }
}

// ==========================================
//   配列リセット
// ==========================================
void arrayInit(){
  for(unsigned char i=0; i<20; i++) pulseTimes[i] = 0;
  pulseTimes[20] = millis();
}