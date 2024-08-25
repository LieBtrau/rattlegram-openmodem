// https://github.com/vanbwodonk/es8388arduino
//
#include <Arduino.h>
#include <Wire.h>
#pragma once
#include <stdint.h>

typedef enum
{
  MIXIN1, // direct line 1
  MIXIN2, // direct line 2
  MIXRES, // reserverd es8388
  MIXADC  // Select from ADC/ALC
} mixsel_t;

typedef enum
{
  IN1,     // Select Line IN L/R 1
  IN2,     // Select Line IN L/R 2
  IN1DIFF, // differential IN L/R 1
  IN2DIFF  // differential IN L/R 2
} insel_t;

typedef enum
{
  DACOUT,    // Select Sink From DAC
  SRCSELOUT, // Select Sink From SourceSelect()
  MIXALL,    // Sink ALL DAC & SourceSelect()
} mixercontrol_t;

typedef enum
{
  DISABLE, // Disable ALC
  GENERIC, // Generic Mode
  VOICE,   // Voice Mode
  MUSIC    // Music Mode
} alcmodesel_t;

class ES8388
{
public:
  enum class OutSel : uint8_t
  {
    OUT1,   // Select Line OUT L/R 1
    OUT2,   // Select Line OUT L/R 2
    OUTALL, // Enable ALL
  };
  ES8388(uint8_t _sda, uint8_t _scl, uint32_t _speed = 400000);
  ~ES8388();
  bool init();
  bool identify(int sda, int scl, uint32_t frequency);
  uint8_t *readAllReg();
  bool outputSelect(OutSel sel);
  bool inputSelect(insel_t sel);
  bool DACmute(bool mute);
  uint8_t getOutputVolume();
  uint8_t getMaxVolume() const;
  bool setOutputVolume(OutSel outSel, uint8_t vol);
  uint8_t getInputGain();
  bool setInputGain(uint8_t gain);
  bool setALCmode(alcmodesel_t alc);
  bool mixerSourceSelect(mixsel_t LMIXSEL, mixsel_t RMIXSEL);
  bool mixerSourceControl(bool LD2LO, bool LI2LO, uint8_t LI2LOVOL, bool RD2RO,
                          bool RI2RO, uint8_t RI2LOVOL);
  bool mixerSourceControl(mixercontrol_t mix);
  bool analogBypass(bool bypass);

private:
  enum class ES8388_REG : uint8_t
  {
    /* ES8388 register */
    CONTROL1 = 0,
    CONTROL2 = 1,
    CHIPPOWER = 2,
    ADCPOWER = 3,
    DACPOWER = 4,
    CHIPLOPOW1 = 5,
    CHIPLOPOW2 = 6,
    ANAVOLMANAG = 7,
    MASTERMODE = 8,
    /* ADC */
    ADCCONTROL1 = 9,
    ADCCONTROL2 = 10,
    ADCCONTROL3 = 11,
    ADCCONTROL4 = 12,
    ADCCONTROL5 = 13,
    ADCCONTROL6 = 14,
    ADCCONTROL7 = 15,
    ADCCONTROL8 = 16,
    ADCCONTROL9 = 17,
    ADCCONTROL10 = 18,
    ADCCONTROL11 = 19,
    ADCCONTROL12 = 20,
    ADCCONTROL13 = 21,
    ADCCONTROL14 = 22,
    /* DAC */
    DACCONTROL1 = 23,
    DACCONTROL2 = 24,
    DACCONTROL3 = 25,
    DACCONTROL4 = 26,
    DACCONTROL5 = 27,
    DACCONTROL6 = 28,
    DACCONTROL7 = 29,
    DACCONTROL8 = 30,
    DACCONTROL9 = 31,
    DACCONTROL10 = 32,
    DACCONTROL11 = 33,
    DACCONTROL12 = 34,
    DACCONTROL13 = 35,
    DACCONTROL14 = 36,
    DACCONTROL15 = 37,
    DACCONTROL16 = 38,
    DACCONTROL17 = 39,
    DACCONTROL18 = 40,
    DACCONTROL19 = 41,
    DACCONTROL20 = 42,
    DACCONTROL21 = 43,
    DACCONTROL22 = 44,
    DACCONTROL23 = 45,
    DACCONTROL24 = 46,
    DACCONTROL25 = 47,
    DACCONTROL26 = 48,
    DACCONTROL27 = 49,
    DACCONTROL28 = 50,
    DACCONTROL29 = 51,
    DACCONTROL30 = 52,
    _COUNT
  };
  insel_t _inSel = IN1;
  TwoWire i2c = TwoWire(0);
  uint8_t _pinsda, _pinscl;
  uint32_t _i2cspeed;
  bool write_reg(ES8388_REG reg_add, uint8_t data);
  bool read_reg(ES8388_REG reg_add, uint8_t &data);
};