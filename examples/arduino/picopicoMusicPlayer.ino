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
//   音階定義 (周波数 Hz)
// ==========================================
// 音の高さ（周波数）に名前をつけています。
// 数字のままだと分かりにくいので、楽譜のように扱えるようにします。
#define NOTE_G3 196  // ソ (低い)
#define NOTE_GS3 208
#define NOTE_A3 220  // ラ
#define NOTE_AS3 233
#define NOTE_B3 247  // シ
#define NOTE_C4 262  // ド (真ん中)
#define NOTE_CS4 277
#define NOTE_D4 294  // レ
#define NOTE_DS4 311
#define NOTE_E4 330  // ミ
#define NOTE_F4 349  // ファ
#define NOTE_FS4 370
#define NOTE_G4 392  // ソ
#define NOTE_GS4 415
#define NOTE_A4 440  // ラ (時報の音)
#define NOTE_AS4 466
#define NOTE_B4 494  // シ
#define NOTE_C5 523  // ド (高い)
#define NOTE_CS5 554
#define NOTE_D5 587  // レ
#define NOTE_DS5 622
#define NOTE_E5 659  // ミ
#define NOTE_F5 698  // ファ
#define NOTE_FS5 740
#define NOTE_G5 784  // ソ
#define NOTE_GS5 831
#define NOTE_A5 880  // ラ
#define NOTE_AS5 932
#define NOTE_B5 988   // シ
#define NOTE_C6 1047  // ド (もっと高い)
#define NOTE_D6 1175
#define NOTE_E6 1319
#define REST 0  // 休符 (無音)

// 音符の構造体：音の高さと、長さをセットで管理します
struct Note {
  int freq;      // 周波数 (Hz)
  int duration;  // 長さ (ミリ秒)
};

// ==========================================
//   曲データ
// ==========================================

// --- 1. トルコ行進曲 (Mozart) ---
Note song_turkish[] = {
  { NOTE_B4, 120 }, { NOTE_A4, 120 }, { NOTE_GS4, 120 }, { NOTE_A4, 120 },  // タラララ
  { NOTE_C5, 480 },
  { REST, 120 },  // ターン
  { NOTE_D5, 120 },
  { NOTE_C5, 120 },
  { NOTE_B4, 120 },
  { NOTE_C5, 120 },  // タラララ
  { NOTE_E5, 480 },
  { REST, 120 },  // ターン
  { NOTE_F5, 120 },
  { NOTE_E5, 120 },
  { NOTE_DS5, 120 },
  { NOTE_E5, 120 },  // タラララ
  { NOTE_B5, 120 },
  { NOTE_A5, 120 },
  { NOTE_GS5, 120 },
  { NOTE_A5, 120 },  // (高音パート)
  { NOTE_B5, 120 },
  { NOTE_A5, 120 },
  { NOTE_GS5, 120 },
  { NOTE_A5, 120 },
  { NOTE_C6, 240 },
  { NOTE_A5, 240 },
  { NOTE_C6, 240 },
  { NOTE_G5, 240 },  // チャーチャー
  { NOTE_A5, 480 },
  { REST, 120 },
  { NOTE_B4, 120 },
  { NOTE_A4, 120 },
  { NOTE_GS4, 120 },
  { NOTE_A4, 120 },  // 繰り返しへ
  { REST, 300 }
};

// --- 2. エンターテイナー (Scott Joplin) ---
Note song_entertainer[] = {
  { NOTE_D4, 150 }, { NOTE_DS4, 150 }, { NOTE_E4, 150 }, { NOTE_C5, 300 }, { NOTE_E4, 150 }, { NOTE_C5, 300 }, { NOTE_E4, 150 }, { NOTE_C5, 900 }, { NOTE_C5, 150 }, { NOTE_D5, 150 }, { NOTE_DS5, 150 }, { NOTE_E5, 150 }, { NOTE_C5, 150 }, { NOTE_D5, 150 }, { NOTE_E5, 300 }, { NOTE_B4, 150 }, { NOTE_D5, 300 }, { NOTE_C5, 300 }, { REST, 150 }, { NOTE_D4, 150 }, { NOTE_DS4, 150 }, { NOTE_E4, 150 }, { NOTE_C5, 300 }, { NOTE_E4, 150 }, { NOTE_C5, 300 }, { NOTE_E4, 150 }, { NOTE_C5, 900 }, { NOTE_A4, 150 }, { NOTE_G4, 150 }, { NOTE_FS4, 150 }, { NOTE_A4, 150 }, { NOTE_C5, 150 }, { NOTE_E5, 300 }, { NOTE_D5, 150 }, { NOTE_C5, 150 }, { NOTE_A4, 150 }, { NOTE_D5, 600 }, { REST, 300 }
};

// --- 3. クシコス・ポスト (Hermann Necke) ---
Note song_race[] = {
  { NOTE_C5, 130 }, { NOTE_G4, 130 }, { NOTE_G4, 130 }, { NOTE_C5, 130 }, { NOTE_G4, 130 }, { NOTE_C5, 130 }, { NOTE_E5, 130 }, { NOTE_C5, 130 }, { NOTE_F5, 130 }, { NOTE_D5, 130 }, { NOTE_D5, 130 }, { NOTE_F5, 130 }, { NOTE_D5, 130 }, { NOTE_F5, 130 }, { NOTE_A5, 130 }, { NOTE_F5, 130 }, { NOTE_G5, 130 }, { NOTE_E5, 130 }, { NOTE_E5, 130 }, { NOTE_G5, 130 }, { NOTE_E5, 130 }, { NOTE_G5, 130 }, { NOTE_C6, 130 }, { NOTE_G5, 130 }, { NOTE_F5, 130 }, { NOTE_D5, 130 }, { NOTE_B4, 130 }, { NOTE_D5, 130 }, { NOTE_C5, 520 }, { REST, 200 }
};

// --- 4. コロブチカ (Russian Folk) ---
Note song_folk[] = {
  { NOTE_E5, 300 }, { NOTE_B4, 150 }, { NOTE_C5, 150 }, { NOTE_D5, 300 }, { NOTE_C5, 150 }, { NOTE_B4, 150 }, { NOTE_A4, 300 }, { NOTE_A4, 150 }, { NOTE_C5, 150 }, { NOTE_E5, 300 }, { NOTE_D5, 150 }, { NOTE_C5, 150 }, { NOTE_B4, 450 }, { NOTE_C5, 150 }, { NOTE_D5, 300 }, { NOTE_E5, 300 }, { NOTE_C5, 300 }, { NOTE_A4, 300 }, { NOTE_A4, 600 }, { REST, 200 }
};

// --- プレイリスト設定 ---
Note* playlist[] = { song_turkish, song_entertainer, song_race, song_folk };
int songSizes[] = {
  sizeof(song_turkish) / sizeof(Note),
  sizeof(song_entertainer) / sizeof(Note),
  sizeof(song_race) / sizeof(Note),
  sizeof(song_folk) / sizeof(Note)
};
int totalSongs = 4;  // 曲の総数

// --- システム変数 ---
int currentSongIdx = 0;          // 今の曲番号
int noteIndex = 0;               // 今の音符番号
unsigned long lastNoteTime = 0;  // 最後に音を鳴らした時間
bool isPlaying = true;           // 再生中かどうか
int currentFreq = 0;             // 現在鳴っている周波数

// ビジュアル用（各列のバーの高さ）
float columnHeight[8];

// ★追加：ボタン同時押し判定用の変数
unsigned long bothPressedStart = 0;  // 同時押し開始時間
bool isBothPressed = false;          // 同時押し中フラグ

// ==========================================
//   セットアップ (起動時1回のみ実行)
// ==========================================
void setup() {
  // LEDの初期化
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);

  // ピンモード設定
  pinMode(BTN_LEFT, INPUT_PULLUP);   // 押すとLOW
  pinMode(BTN_RIGHT, INPUT_PULLUP);  // 押すとLOW
  pinMode(BUZZER_PIN, OUTPUT);

  // 変数初期化
  for (int i = 0; i < 8; i++) columnHeight[i] = 0;

  // 起動音
  tone(BUZZER_PIN, NOTE_E5, 100);
}

// ==========================================
//   メインループ (繰り返し実行)
// ==========================================
void loop() {
  unsigned long currentTime = millis();  // 現在時刻の取得

  // ボタンの状態を読み取る
  bool leftState = digitalRead(BTN_LEFT);
  bool rightState = digitalRead(BTN_RIGHT);

  // ----------------------------------------------------
  // ★最優先：書き込みエラー対策 (同時長押しリセット)
  // ----------------------------------------------------
  if (leftState == LOW && rightState == LOW) {
    // 両方のボタンが押されている時
    if (isBothPressed == false) {
      // 押された瞬間：計測スタート
      bothPressedStart = currentTime;
      isBothPressed = true;
    } else {
      // 押し続けている間：時間をチェック
      unsigned long duration = currentTime - bothPressedStart;

      //  5秒(5000ms)以上経過したら...
      if (duration > 5000) {
        FastLED.clear();
        FastLED.show();               // LEDを消す
        tone(BUZZER_PIN, 1500, 500);  // 合図の音 (ピロロッ)
        delay(500);

        // ★ここで強制的に書き込みモードへ移行します
        reset_usb_boot(0, 0);
      }
    }
  } else {
    // どちらか片方でも離したらリセット
    isBothPressed = false;
  }

  // ----------------------------------------------------
  // 1. 曲送り (右ボタン)
  // ----------------------------------------------------
  // 同時押し中でない、かつ右ボタンだけ押された場合
  if (!isBothPressed && rightState == LOW) {
    currentSongIdx = (currentSongIdx + 1) % totalSongs;
    noteIndex = 0;
    isPlaying = true;
    currentFreq = 0;
    noTone(BUZZER_PIN);

    // フラッシュ演出 (画面を白く光らせる)
    fill_solid(leds, NUM_LEDS, CRGB::White);
    FastLED.show();
    delay(50);
    FastLED.clear();
    delay(200);  // チャタリング防止の待機
  }

  // ----------------------------------------------------
  // 2. 再生/停止 (左ボタン)
  // ----------------------------------------------------
  if (!isBothPressed && leftState == LOW) {
    isPlaying = !isPlaying;  // 状態を反転
    if (!isPlaying) {
      noTone(BUZZER_PIN);
      currentFreq = 0;
    }
    delay(300);  // チャタリング防止
  }

  // ----------------------------------------------------
  // 3. 音楽再生処理
  // ----------------------------------------------------
  if (isPlaying) {
    Note* currentSong = playlist[currentSongIdx];

    // 前の音符の時間が終わったかチェック
    if (currentTime - lastNoteTime >= currentSong[noteIndex].duration) {
      lastNoteTime = currentTime;

      int freq = currentSong[noteIndex].freq;
      int duration = currentSong[noteIndex].duration;

      if (freq > 0) {
        // スタッカート気味に鳴らす (長さの90%だけ鳴らす)
        tone(BUZZER_PIN, freq, duration * 0.9);
      } else {
        noTone(BUZZER_PIN);  // 休符
      }

      currentFreq = freq;

      // ビジュアル更新：音階に対応する列を最大高さにする
      int col = getColumnFromFreq(freq);
      if (col >= 0 && col < 8) {
        columnHeight[col] = 8.0;
      }

      // 次の音符へ
      noteIndex++;
      if (noteIndex >= songSizes[currentSongIdx]) {
        noteIndex = 0;  // 曲が終わったら最初に戻る
      }
    }
  }

  // ----------------------------------------------------
  // 4. ビジュアル描画 (10msごとに更新)
  // ----------------------------------------------------
  drawEQVisualizer();
  delay(10);
}

// --- ビジュアライザー描画関数 ---
void drawEQVisualizer() {
  FastLED.clear();

  for (int x = 0; x < 8; x++) {
    // バーの高さを少しずつ下げる (重力表現)
    if (columnHeight[x] > 0) {
      columnHeight[x] -= 0.3;  // 下がるスピード
    }
    if (columnHeight[x] < 0) columnHeight[x] = 0;

    int h = (int)columnHeight[x];

    // 色の計算 (虹色)
    byte hue = map(x, 0, 7, 0, 224);

    for (int i = 0; i < h; i++) {
      // ★上下反転の描画ロジック
      // 通常 y=0 が下ですが、y=7 を始点にすることで上からぶら下がるように見せます
      int y = 7 - i;

      leds[XY(x, y)] = CHSV(hue, 255, 255);
    }

    // ピーク（先端）を少し白っぽく光らせる
    if (h > 0 && h <= 8) {
      int peakY = 7 - (h - 1);
      leds[XY(x, peakY)] = CHSV(hue, 100, 255);
    }
  }

  // 停止中は左上に赤色を表示
  if (!isPlaying) leds[XY(0, 7)] = CRGB::Red;

  FastLED.show();
}

// --- 音の高さ(Hz)から、LEDの列番号(0-7)を決める関数 ---
int getColumnFromFreq(int freq) {
  if (freq == 0) return -1;  // 無音

  float f = freq;
  // オクターブ計算：すべての音を C4(ド)〜B4(シ) の範囲に変換して扱います
  while (f >= 520) f /= 2.0;
  while (f < 260) f *= 2.0;

  // 音程ごとの振り分け
  if (f < 270) return 0;  // ド
  if (f < 300) return 1;  // レ
  if (f < 340) return 2;  // ミ
  if (f < 380) return 3;  // ファ
  if (f < 430) return 4;  // ソ
  if (f < 480) return 5;  // ラ
  if (f < 510) return 6;  // シ
  return 7;               // 高いド
}

// --- 座標変換関数 (ジグザグ配線対応) ---
int XY(int x, int y) {
  if (y % 2 == 0) return (y * 8) + x;  // 偶数行は左→右
  else return (y * 8) + (7 - x);       // 奇数行は右→左
}