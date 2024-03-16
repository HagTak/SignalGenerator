/*
信号発生器
*/
#include "LiquidCrystal_I2C.h"
#include "AD9833.h"
#include "RotaryEncoder.h"

// ピン設定 ----------------------------
const uint8_t PIN_RE_A = 4;  // ロータリエンコーダ
const uint8_t PIN_RE_B = 16;
const uint8_t PIN_RE_SW = 17;
const uint8_t PIN_ONOFF = 35;  // スイッチ類
const uint8_t PIN_WAVE = 32;
const uint8_t PIN_DOWN = 33;
const uint8_t PIN_UP = 25;
const uint8_t PIN_SCLK = 14;  // AD9833 SPI
const uint8_t PIN_SDATA = 12;
const uint8_t PIN_FSYNC = 15;
const uint8_t PIN_SCL = 22;  // LCD I2C
const uint8_t PIN_SDA = 5;

// 他定数類 ------------------------------
const uint8_t ADDR_LCD = 0x27;  // LCD
const double F_MIN = 1; // 最低周波数[Hz]
const double F_MAX = 1 * 1000 * 1000; // 最高周波数[Hz]

// グローバルオブジェクト -----------------
LiquidCrystal_I2C lcd(ADDR_LCD, 16, 2);  // CLD
AD9833 ad9833(HSPI); // AD9833
RotaryEncoder re(PIN_RE_A, PIN_RE_B);
double f_target = 1000;
double f_scale = 1000;
double f_resolution = 0.1;
int waveform = AD9833::WAVEFORM_SINUSOIDAL;
char wavename[5] = "SIN ";
bool on_off = false;
char f_text[13] = "   1000.0 Hz";
char on_off_text[5] = "OFF ";
char is_fine[9] = "        ";

/*
setup
*/
void setup() {
  Serial.begin(115200);
  Serial.println("Start");

  // ピン入出力設定
  pinMode(PIN_RE_A, INPUT);
  pinMode(PIN_RE_B, INPUT);
  pinMode(PIN_RE_SW, INPUT);
  pinMode(PIN_ONOFF, INPUT);
  pinMode(PIN_DOWN, INPUT);
  pinMode(PIN_UP, INPUT);
  pinMode(PIN_WAVE, INPUT);
  Wire.setPins(PIN_SDA, PIN_SCL);  // I2C
  Serial.println("PinMode init");

  // AD9833
  ad9833.begin(PIN_SCLK, -1, PIN_SDATA, PIN_FSYNC);
  ad9833.output_off(); // とにかく出力OFF
  ad9833.set_frequency(f_target);
  ad9833.set_waveform(waveform);
  Serial.println("AD9833 Start");

  // LCD
  lcd.init();
  lcd.backlight();
  lcd.cursor_off();
  lcd.print("Signal Generator\n");
  Serial.println("LCD Start");
  delay(3000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(wavename);
  lcd.setCursor(4, 0);
  sprintf(f_text, "%9.1f Hz", f_target);
  lcd.print(f_text);
  lcd.setCursor(0, 1);
  lcd.print(on_off_text);
  lcd.setCursor(8, 1);
  lcd.print(is_fine);
}

/*
スイッチ入力チェック
pushed: 押下長さ[msec], 0は押下なし
*/
int check_input(uint8_t pin, uint8_t edge = LOW) {
  int pushed = 0;
  if (digitalRead(pin) == edge) {
    delay(10);
    while (digitalRead(pin) == edge) {
      delay(10);
      pushed += 10;
    }
    // Serial.printf("%dpin pushed=%d\n", pin, pushed);
  }
  return pushed;
}

void loop() {

  // ロータリースイッチ -------------------------
  int reResult = re.read_until_nop();
  if (reResult != 0) {
    f_target += f_scale * f_resolution * reResult;
    if (f_target >= f_scale * 10) {
      if (f_scale < 100000) {
        f_scale *= 10;
      } else {
        f_target = 1000000;
      }
    } else if (f_target < f_scale) {
      if (f_scale > 1) {
        f_scale *= 0.1;
      } else {
        f_target = 1;
      }
    }
    ad9833.set_frequency(f_target);
    sprintf(f_text, "%9.1f Hz", f_target);
    lcd.setCursor(4, 0);
    lcd.print(f_text);
    Serial.printf("reResult=%d, f_target=%.1f f_scale=%.1f\n", reResult, f_target, f_scale);
  }
  if (check_input(PIN_RE_SW) > 0) {
    // COARSE/FINE切り替え
    if (f_resolution == 0.1) {
      f_resolution = 0.01;
      sprintf(is_fine, "FINE    ");
    } else {
      f_resolution = 0.1;
      sprintf(is_fine, "        ");
    }
    lcd.setCursor(8, 1);
    lcd.print(is_fine);
    Serial.printf("f_resolution=%.3f\n", f_resolution);
  }

  // 操作スイッチ類 -------------------------
  if (check_input(PIN_ONOFF) > 0) {
    // 出力ON/OFF
    on_off = !on_off;
    if (on_off) {
      ad9833.output_on();
      sprintf(on_off_text, "ON      ");
      printf("output on\n");
    } else {
      ad9833.output_off();
      sprintf(on_off_text, "OFF     ");
      printf("output off\n");
    }
    lcd.setCursor(0, 1);
    lcd.print(on_off_text);
  }
  if (check_input(PIN_DOWN) > 0) {
    // スケールダウン
    if (f_target * 0.1 >= F_MIN) {
      f_target *= 0.1;
      ad9833.set_frequency(f_target);
      sprintf(f_text, "%9.1f Hz", f_target);
      lcd.setCursor(4, 0);
      lcd.print(f_text);
      printf("scale down, f_target=%.1f\n", f_target);
    }
  }
  if (check_input(PIN_UP) > 0) {
    // スケールアップ
    if (f_target * 10 <= F_MAX) {
      f_target *= 10;
      ad9833.set_frequency(f_target);
      sprintf(f_text, "%9.1f Hz", f_target);
      lcd.setCursor(4, 0);
      lcd.print(f_text);
      printf("scale up, f_target=%.1f\n", f_target);
    }
  }
  if (check_input(PIN_WAVE) > 0) {
    // 波形の変更
    if (waveform == AD9833::WAVEFORM_SINUSOIDAL) {
      waveform = AD9833::WAVEFORM_TRIANGLE;
      sprintf(wavename, "TRI ");
    } else if (waveform == AD9833::WAVEFORM_TRIANGLE) {
      waveform = AD9833::WAVEFORM_SQUARE;
      sprintf(wavename, "SQU ");
    } else {
      waveform = AD9833::WAVEFORM_SINUSOIDAL;
      sprintf(wavename, "SIN ");
    }
    ad9833.set_waveform(waveform);
    lcd.setCursor(0, 0);
    lcd.print(wavename);
    printf("waveform=%s, f_target=%.1f\n", wavename, f_target);
  }

}