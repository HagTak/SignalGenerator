class RotaryEncoder {

public:

  // 結果
  static const uint8_t RESULT_NOP = 0; // 動作なし
  static const uint8_t RESULT_CW = 1; // 時計回り
  static const uint8_t RESULT_CCW = 2; // 反時計回り
  
  // 設定値
  int waitPeriodMsec = 1; // 読み取り周期[msec]
  int waitTimeoutMsec = 100; // タイムアウト[msec]
  unsigned long nopTimeoutMsec = 50; // 無操作タイムアウト[msec]

  // コンストラクタ
  RotaryEncoder(uint8_t pinCW, uint8_t pinCCW) {
    _pinCW = pinCW;
    _pinCCW = pinCCW;
  }

  // 読む
  // return: タイムアウト または 操作なしのときはRESULT_NONE
  uint8_t read() {
    uint8_t result = RESULT_NOP;
    int waitMsec = 0;
    bool timeout = false;
    delay(waitPeriodMsec);
    if (digitalRead(_pinCW) == LOW) {
      while (digitalRead(_pinCCW) == HIGH) {
        delay(waitPeriodMsec);
        if (waitMsec > waitTimeoutMsec) {
          timeout = true;
          break;
        }
        waitMsec += waitPeriodMsec;
      }
      if (!timeout) {
        result = RESULT_CW;
        while ((digitalRead(_pinCW) != HIGH) || (digitalRead(_pinCCW) != HIGH)) {
          delay(waitPeriodMsec);
        }
      }
    } else if (digitalRead(_pinCCW) == LOW) {
      while (digitalRead(_pinCW) == HIGH) {
        delay(waitPeriodMsec);
        if (waitMsec > waitTimeoutMsec) {
          timeout = true;
          break;
        }
        waitMsec += waitPeriodMsec;
      }
      if (!timeout) {
        result = RESULT_CCW;
        while ((digitalRead(_pinCW) != HIGH) || (digitalRead(_pinCCW) != HIGH)) {
          delay(waitPeriodMsec);
        }
      }
    }
    return result;
  }

  // 操作が終わるまでカウントを累積する(正: CW, 負: CCW)
  // 最後の操作からnopTimeoutMsec経過したら終了
  int read_until_nop() {
    unsigned long t0 = millis();
    int x = 0;
    while (true) {
      uint8_t reResult = read();
      if (reResult == RESULT_CW) {
        x += 1;
        t0 = millis();
      } else if (reResult == RESULT_CCW) {
        x -= 1;
        t0 = millis();
      } else {
        if (millis() - t0 > nopTimeoutMsec) {
          break;
        }
      }
    }
    return x;
  }

private:
  uint8_t _pinCW;
  uint8_t _pinCCW;
};
