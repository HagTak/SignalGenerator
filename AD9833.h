#include <SPI.h>

// コントロール・レジスタ上位
// 各ビット = 0, 0, B28, HLB, FSELECT, PSELECT, 0, RESET
// B28=1: 周波数レジスタ28ビット
// HLB=0: B28=1なので無視
// FSELECT=0: 使用する位相アキュームレータ=FREQ0
// PSELECT=0: 位相アキュームレータ出力への加算用レジスタ=PHASE0
// RESET=0: リセットなし
// コントロール・レジスタ下位
// SLEEP1, SLEEP12, OPBITEN, 0, DIV2, 0, MODE, 0
// SLEEP1=0, SLEEP12=0: パワーダウンなし
// OPBITEN=0, MODE=0, DIV2=X: サイン波出力

class AD9833 {

public:

  // 波形
  static const int WAVEFORM_SINUSOIDAL = 0b00000000;
  static const int WAVEFORM_TRIANGLE = 0b00000010;
  static const int WAVEFORM_SQUARE = 0b00101000;
  // モード
  static const uint8_t FREQX_0 = 0;
  static const uint8_t FREQX_1 = 1;

  // コンストラクタ
  AD9833(uint8_t spi_bus)
  {
    _spi = SPIClass(spi_bus);
    CTRL[0] = 0b00100000;
    CTRL[1] = 0b00000000;
  }

  void begin(int8_t sck, int8_t miso, int8_t mosi, int8_t ss)
  {
    _ss = ss;
    pinMode(ss, OUTPUT);
    digitalWrite(ss, HIGH);
    _spi.begin(sck, miso, mosi, ss);
  }

  void output_on() {
    // 送信データ
    CTRL[0] = (CTRL[0] & 0b11111110) | 0b00000000;
    byte data[6] = { CTRL[0], CTRL[1], FREQ28[0], FREQ28[1], FREQ28[2], FREQ28[3] };
    send_data(data);
  }

  void output_off() {
    // 送信データ
    CTRL[0] = (CTRL[0] & 0b11111110) | 0b00000001;
    byte data[6] = { CTRL[0], CTRL[1], FREQ28[0], FREQ28[1], FREQ28[2], FREQ28[3] };
    send_data(data);
  }

  void set_frequency(double f_target)
  {
    // レジスタ値算出
    get_FREQX_register(f_target);
    // 送信データ
    byte data[6] = { CTRL[0], CTRL[1], FREQ28[0], FREQ28[1], FREQ28[2], FREQ28[3] };
    send_data(data);
  }

  void set_waveform(int waveform)
  {
    CTRL[1] = (CTRL[1] & 0b11010101) | waveform;
    // 送信データ
    byte data[6] = { CTRL[0], CTRL[1], FREQ28[0], FREQ28[1], FREQ28[2], FREQ28[3] };
    send_data(data);
  }

  void send_data(byte data[])
  {
    digitalWrite(_ss, LOW);
    _spi.writeBytes(data, 6);
    digitalWrite(_ss, HIGH);
  }

private:

  static const unsigned long F_MSCK = 25 * 1000 * 1000;

  SPIClass _spi;
  int8_t _ss = -1;
  uint8_t FREQX = FREQX_0;
  int waveform = WAVEFORM_SINUSOIDAL;
  uint8_t CTRL[2] = {0b00100000, 0b00000000}; // コントロールレジスタ
  uint8_t FREQ28[4] = {0x00, 0x00, 0x00, 0x00}; // 周波数レジスタ

  // FREQXレジスタ値を算出
  // f_target: 目標周波数[Hz](12.5MHzまで)
  // FREQX: 0=FREQ0, 1=FREQ1
  void get_FREQX_register(double f_target) {
      
      uint8_t FREQX_select;
      if (FREQX == 0) {
          FREQX_select = 0x40; // 0100 0000
      } else {
          FREQX_select = 0x80; // 1000 0000
      }
      unsigned long LSB = (unsigned long) (0x10000000 * f_target / F_MSCK);
      unsigned long LSBs14 = LSB % 0x4000;
      unsigned long MSBs14 = LSB / 0x4000;
      
      FREQ28[0] = (LSBs14 / 0x0100) | FREQX_select;
      FREQ28[1] = LSBs14 % 0x0100;
      FREQ28[2] = (MSBs14 / 0x0100) | FREQX_select;
      FREQ28[3] = MSBs14 % 0x0100;
      
  }

};