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
//   2. ゲーム変数 (状態管理)
// ==========================================
int playerX = 3;       // プレイヤーの横位置 (0〜7)
int enemyX = 4;        // 敵の横位置
int enemyY = -1;       // 敵の縦位置 (-1は画面外、0〜7が画面内)

// 敵の種類を管理する変数
// 0: 彗星(尾を引く), 1: 壁(穴あき), 2: 巨大な塊(3マス幅)
int enemyType = 0;     

// 難易度・進行管理
int score = 0;         // 現在のスコア
int level = 1;         // 現在のレベル
int loopCounter = 0;   // 時間カウント用
int moveThreshold = 7; // 敵が動くまでのカウント数 (小さいほど速い)

// 書き込みエラー対策 (同時押し判定用)
unsigned long bothPressedStart = 0; 
bool isBothPressed = false;         

// ==========================================
//   3. セットアップ (起動時に1回だけ動く)
// ==========================================
void setup() {
  // ボタンの設定 (INPUT_PULLUP: 押すとLOWになる)
  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);

  // LEDの準備
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);

  // スタート音 (ド・ミ・ソ♪)
  tone(BUZZER_PIN, 1000, 100); delay(150);
  tone(BUZZER_PIN, 1500, 100); delay(150);
  tone(BUZZER_PIN, 2000, 300);
}

// ==========================================
//   4. メインループ (ここを繰り返す)
// ==========================================
void loop() {
  unsigned long currentTime = millis(); // 今の時間を取得
  FastLED.clear(); // 毎回、画面を一度消して描き直します

  // ボタンの状態をチェック
  bool leftState = digitalRead(BTN_LEFT);
  bool rightState = digitalRead(BTN_RIGHT);

  // ----------------------------------------------------
  // ★救済機能: 強制リセット (左右ボタン10秒長押し)
  // ----------------------------------------------------
  if (leftState == LOW && rightState == LOW) {
    if (isBothPressed == false) {
      bothPressedStart = currentTime; // 押し始め時間を記録
      isBothPressed = true;
    } else {
      // 5秒(5000ms)経過したらリセット実行
      if (currentTime - bothPressedStart > 5000) {
        FastLED.clear(); FastLED.show();
        tone(BUZZER_PIN, 1500, 500); // 合図の音
        delay(500);
        reset_usb_boot(0, 0); // ★強制的に書き込みフォルダを開く
      }
    }
    return; // リセット待機中はゲームを止める
  } else {
    isBothPressed = false;
  }

  // ----------------------------------------------------
  // プレイヤーの操作
  // ----------------------------------------------------
  // ★左ボタン (Pin 3)
  if (leftState == LOW) {
    if (playerX < 7) { // 左端(7)でなければ
      playerX++;       // 左へ (++で左に見える配線)
      tone(BUZZER_PIN, 800, 20); 
    }
  }
  // ★右ボタン (Pin 2)
  if (rightState == LOW) {
    if (playerX > 0) { // 右端(0)でなければ
      playerX--;       // 右へ (--で右に見える配線)
      tone(BUZZER_PIN, 800, 20); 
    }
  }

  // ----------------------------------------------------
  // 敵の動き & レベル管理
  // ----------------------------------------------------
  loopCounter++; // カウンターを1つ進める
  
  // 設定したスピード(moveThreshold)になったら敵を動かす
  if (loopCounter > moveThreshold) {
    enemyY++;        // 敵を1マス下に
    loopCounter = 0; // カウントリセット

    // ★敵を回避成功！ (画面外へ抜けた)
    if (enemyY > 7) {
      score++;                    // スコア加算
      tone(BUZZER_PIN, 1500, 50); // キラッと音
      
      // ★★★ 全クリア判定 (64点) ★★★
      if (score >= 64) {
        gameClear(); // クリア演出へ
        return;      
      }

      // === レベルと難易度の調整 ===
      
      if (score < 10) {
        // 【レベル1】 彗星モード
        // スピード: 普通 (7)
        // 見た目: 尾を引く光
        level = 1;
        moveThreshold = 7; 
        enemyType = 0; // 彗星
      } 
      else if (score < 25) {
        // 【レベル2】 巨大隕石モード
        // スピード: 少し速い (6)
        // 見た目: 幅3マスの巨大な塊 (ご要望反映)
        level = 2;
        moveThreshold = 6; 
        enemyType = 2; // 3マス幅
      } 
      else if (score < 45) {
        // 【レベル3】 壁モード
        // スピード: 速い (5) ※前回より少し遅くしました
        // 見た目: 穴あきの壁
        level = 3;
        moveThreshold = 5; 
        enemyType = 1; // 壁
      }
      else {
        // 【レベル4】 限界突破
        // スピード: 超高速 (3)
        // 見た目: 壁
        level = 4;
        moveThreshold = 3; 
        enemyType = 1; // 壁
      }

      // 次の敵をセット
      enemyY = 0; // 上に戻す
      
      // 敵の種類に合わせて出現位置をランダム決定
      if (enemyType == 2) {
        // 3マス幅の場合、画面からはみ出ないように 0〜5 の範囲にする
        enemyX = random(0, 6); 
      } else {
        enemyX = random(0, 8); 
      }
    }
  }

  // ----------------------------------------------------
  // 当たり判定 (ぶつかったかチェック)
  // ----------------------------------------------------
  bool isHit = false;

  // 敵が一番下(7)に来たときだけ判定
  if (enemyY == 7) {
    
    if (enemyType == 0) {
      // 0: 彗星 (1マス)
      // 同じ場所にいたらアウト
      if (enemyX == playerX) isHit = true;
    }
    else if (enemyType == 2) {
      // 2: 巨大塊 (3マス幅)
      // 敵は enemyX, enemyX+1, enemyX+2 の3箇所に存在
      if (playerX >= enemyX && playerX <= enemyX + 2) isHit = true;
    }
    else if (enemyType == 1) {
      // 1: 壁 (穴あき)
      // enemyX は「穴」の場所。それ以外の場所にいたらアウト
      if (playerX != enemyX) isHit = true;
    }
  }

  if (isHit) {
    gameOver(); // ゲームオーバーへ
  }

  // ----------------------------------------------------
  // 描画処理 (LEDを光らせる)
  // ----------------------------------------------------
  
  // ■敵の描画
  if (enemyY >= -3 && enemyY <= 7) { // 画面外の尾も計算するため範囲を広めに
    
    // レベルごとの色設定
    CRGB color = CRGB::Red;          // レベル1: 赤
    if (level == 2) color = CRGB::Purple; // レベル2: 紫
    if (level == 3) color = CRGB::Green;  // レベル3: 緑
    if (level == 4) color = CRGB::Orange; // レベル4: オレンジ

    // --- タイプ0: 彗星 (尾を引く) ---
    if (enemyType == 0) {
      // 本体
      if(enemyY >= 0) leds[XY(enemyX, enemyY)] = color;
      
      // 尾っぽ1 (少し暗く)
      if (enemyY - 1 >= 0) leds[XY(enemyX, enemyY - 1)] = color.nscale8(100);
      // 尾っぽ2 (もっと暗く)
      if (enemyY - 2 >= 0) leds[XY(enemyX, enemyY - 2)] = color.nscale8(40);
    }
    
    // --- タイプ2: 巨大塊 (3マス幅) ---
    else if (enemyType == 2) {
      if (enemyY >= 0) {
        leds[XY(enemyX,     enemyY)] = color;
        leds[XY(enemyX + 1, enemyY)] = color;
        leds[XY(enemyX + 2, enemyY)] = color;
      }
    }
    
    // --- タイプ1: 壁 (穴あき) ---
    else if (enemyType == 1) {
      if (enemyY >= 0) {
        for (int x = 0; x < 8; x++) {
          if (x != enemyX) { // 穴以外の場所を塗る
             leds[XY(x, enemyY)] = color;
          }
        }
      }
    }
  }

  // ■プレイヤーの描画 (青)
  leds[XY(playerX, 7)] = CRGB::Blue;

  FastLED.show(); // 光らせる！
  
  // ゲーム速度の微調整
  delay(50); 
}

// ==========================================
//   全クリア演出 (64点達成！)
// ==========================================
void gameClear() {
  // 勝利のメロディ
  int melody[] = {262, 330, 392, 523, 659, 784, 1047}; 
  
  // 音に合わせて虹色花火
  for (int i = 0; i < 7; i++) {
    tone(BUZZER_PIN, melody[i], 150);
    fill_rainbow(leds, NUM_LEDS, i * 30, 7); 
    FastLED.show();
    delay(150);
  }
  
  // 祝福の回転
  for (int j = 0; j < 500; j++) {
    fill_rainbow(leds, NUM_LEDS, j, 7); 
    FastLED.show();
    delay(10);
  }

  resetGame(); // 最初から
}

// ==========================================
//   ゲームオーバー処理
// ==========================================
void gameOver() {
  // 赤く点滅
  for (int i = 0; i < 3; i++) {
    fill_solid(leds, NUM_LEDS, CRGB::Red);
    FastLED.show();
    tone(BUZZER_PIN, 200, 200);
    delay(200);
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    FastLED.show();
    delay(200);
  }

  // スコア表示 (最大64個まで)
  int showScore = score;
  if (showScore > NUM_LEDS) showScore = NUM_LEDS;

  delay(500);
  for (int i=0; i < showScore; i++) {
     tone(BUZZER_PIN, 1000, 50); // 短い音
     leds[XY(i % 8, i / 8)] = CRGB::White; 
     FastLED.show();
     delay(50); 
  }
  delay(2000);

  resetGame();
}

// ==========================================
//   リセット処理
// ==========================================
void resetGame() {
  playerX = 3;
  enemyY = -1;
  loopCounter = 0;
  score = 0;      
  level = 1;      
  enemyType = 0;
  moveThreshold = 7;
}

// ==========================================
//   座標変換 (ジグザグ配線対応)
// ==========================================
int XY(int x, int y) {
  if (y % 2 == 0) {
    return (y * 8) + x;       // 偶数行
  } else {
    return (y * 8) + (7 - x); // 奇数行
  }
}