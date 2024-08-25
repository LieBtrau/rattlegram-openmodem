// https://github.com/vanbwodonk/es8388arduino
// https://github.com/maditnerd/es8388/
// https://github.com/pschatzmann/arduino-audiokit/blob/main/src/audio_driver/es8388/es8388.c

#include "ES8388.h"

#include <Arduino.h>
#include <Wire.h>

static const int I2C_ADDR = 0x10;

ES8388::ES8388(uint8_t _sda, uint8_t _scl, uint32_t _speed): _pinsda(_sda), _pinscl(_scl), _i2cspeed(_speed)
{
  i2c.begin(_sda, _scl, _speed);
}

ES8388::~ES8388() { i2c.~TwoWire(); }

bool ES8388::write_reg(ES8388_REG reg_add, uint8_t data)
{
  i2c.beginTransmission(I2C_ADDR);
  i2c.write(static_cast<uint8_t>(reg_add));
  i2c.write(data);
  return i2c.endTransmission() == 0;
}

bool ES8388::read_reg(ES8388_REG reg_add, uint8_t &data)
{
  bool retval = false;
  i2c.beginTransmission(I2C_ADDR);
  i2c.write(static_cast<uint8_t>(reg_add));
  i2c.endTransmission(false);
  i2c.requestFrom((uint16_t)I2C_ADDR, (uint8_t)1, true);
  if (i2c.available() >= 1)
  {
    data = i2c.read();
    retval = true;
  }
  return retval;
}

bool ES8388::identify(int sda, int scl, uint32_t frequency)
{
  i2c.begin(sda, scl, frequency);
  i2c.beginTransmission(I2C_ADDR);
  return i2c.endTransmission() == 0;
}

uint8_t *ES8388::readAllReg()
{
  static uint8_t reg[53];
  for (ES8388_REG i = ES8388_REG::CONTROL1; i < ES8388_REG::_COUNT; i = static_cast<ES8388_REG>((size_t)i + 1))
  {
    read_reg(i, reg[static_cast<uint8_t>(i)]);
  }
  return reg;
}

bool ES8388::init()
{

  bool res = identify(_pinsda, _pinscl, _i2cspeed);
  if (!res)
  {
    Serial.println("ES8388 not found");
    return false;
  }
  res &= write_reg(ES8388_REG::DACCONTROL3, 0x04);  // mute analog outputs for both channels
  res &= write_reg(ES8388_REG::CONTROL2, 0x50); // not sure what this really does.  It sets the undocumented bit 6.
  res &= write_reg(ES8388_REG::CHIPPOWER, 0x00);  // power up all parts of the chip
  res &= write_reg(ES8388_REG::MASTERMODE, 0x00); // set to slave mode (receive MCLK from external source)

  res &= write_reg(ES8388_REG::DACPOWER, 0xC0); // disable DAC and disable Lout/Rout/1/2
  res &= write_reg(ES8388_REG::CONTROL1, 0x12); // Enfr=0

  /* DAC I2S setup: 16 bit word length, I2S format; MCLK / Fs = 256*/
  res &= write_reg(ES8388_REG::DACCONTROL1, 0x18);
  res &= write_reg(ES8388_REG::DACCONTROL2, 0x02);

  /* DAC to output route mixer configuration: */
  res &= write_reg(ES8388_REG::DACCONTROL17, 0x90); // only left DAC to left mixer enable 0db
  res &= write_reg(ES8388_REG::DACCONTROL20, 0x90); // only right DAC to right mixer enable 0db

  /* DAC and ADC use same LRCK, enable MCLK input; output resistance setup */
  res &= write_reg(ES8388_REG::DACCONTROL21, 0x80);
  res &= write_reg(ES8388_REG::DACCONTROL23, 0x00); // vroi=0

  /* DAC volume control: 0dB (maximum, unattenuated)  */
  res &= write_reg(ES8388_REG::DACCONTROL4, 0x00);  // LDACVOL
  res &= write_reg(ES8388_REG::DACCONTROL5, 0x00);  // RDACVOL

  /* power down ADC, we don't need it */
  res &= write_reg(ES8388_REG::ADCPOWER, 0xff);

  /* power up and enable DAC; */
  res &= write_reg(ES8388_REG::DACPOWER, 0x3c);//enable DAC (L&R) and enable Lout/Rout/1/2
  res &= write_reg(ES8388_REG::DACCONTROL3, 0x00);//DAC volume control, unmute both DAC channels

  /* set up MCLK) */
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0_CLK_OUT1);
  WRITE_PERI_REG(PIN_CTRL, 0xFFF0);

  return res;
}

// Line In to Output mixer routing
// OUT1 -> Select Line OUTL/R1
// OUT2 -> Select Line OUTL/R2
// OUTALL -> Enable ALL
bool ES8388::outputSelect(OutSel _sel)
{
  bool res = false;
  switch(_sel)
  {
    case OutSel::OUT1:
      res = write_reg(ES8388_REG::DACCONTROL16, 0x30);
      break;
    case OutSel::OUT2:
      res = write_reg(ES8388_REG::DACCONTROL16, 0x0C);
      break;
    case OutSel::OUTALL:
      res = write_reg(ES8388_REG::DACCONTROL16, 0x3C);
      break;
    default:
      break;
  }
  return res;
}

// Select input source
// IN1     -> Select Line IN L/R 1
// IN2     -> Select Line IN L/R 2
// IN1DIFF -> differential IN L/R 1
// IN2DIFF -> differential IN L/R 2
bool ES8388::inputSelect(insel_t sel)
{
  bool res = true;
  if (sel == IN1)
    res &= write_reg(ES8388_REG::ADCCONTROL2, 0x00);
  else if (sel == IN2)
    res &= write_reg(ES8388_REG::ADCCONTROL2, 0x50);
  else if (sel == IN1DIFF)
  {
    res &= write_reg(ES8388_REG::ADCCONTROL2, 0xF0);
    res &= write_reg(ES8388_REG::ADCCONTROL3, 0x00);
  }
  else if (sel == IN2DIFF)
  {
    res &= write_reg(ES8388_REG::ADCCONTROL2, 0xF0);
    res &= write_reg(ES8388_REG::ADCCONTROL3, 0x80);
  }
  _inSel = sel;
  return res;
}

// mute Output
bool ES8388::DACmute(bool mute)
{
  uint8_t _reg;
  read_reg(ES8388_REG::DACCONTROL3, _reg);
  if (mute)
  {
    bitSet(_reg, 2);
  }
  else
  {
    bitClear(_reg, 2);
  }
  return  write_reg(ES8388_REG::DACCONTROL3, _reg);;
}

// set output volume max is 33
bool ES8388::setOutputVolume(OutSel outSel, uint8_t vol)
{
  vol = min(vol, getMaxVolume());
  bool res = true;
  if (outSel == OutSel::OUTALL || outSel == OutSel::OUT1)
  {
    res &= write_reg(ES8388_REG::DACCONTROL24, vol); // LOUT1VOL
    res &= write_reg(ES8388_REG::DACCONTROL25, vol); // ROUT1VOL
  }
  else if (outSel == OutSel::OUTALL || outSel == OutSel::OUT2)
  {
    res &= write_reg(ES8388_REG::DACCONTROL26, vol); // LOUT2VOL
    res &= write_reg(ES8388_REG::DACCONTROL27, vol); // ROUT2VOL
  }
  return res;
}

uint8_t ES8388::getOutputVolume()
{
  static uint8_t _reg;
  read_reg(ES8388_REG::DACCONTROL24, _reg);
  return _reg;
}

uint8_t ES8388::getMaxVolume() const
{
  return 33;
}

// set input gain max is 8 +24db
bool ES8388::setInputGain(uint8_t gain)
{
  if (gain > 8)
    gain = 8;
  bool res = true;
  gain = (gain << 4) | gain;
  res &= write_reg(ES8388_REG::ADCCONTROL1, gain);
  return res;
}

uint8_t ES8388::getInputGain()
{
  static uint8_t _reg;
  read_reg(ES8388_REG::ADCCONTROL1, _reg);
  _reg = _reg & 0x0F;
  return _reg;
}

// Recommended ALC setting from User Guide
// DISABLE -> Disable ALC
// GENERIC -> Generic Mode
// VOICE   -> Voice Mode
// MUSIC   -> Music Mode
bool ES8388::setALCmode(alcmodesel_t alc)
{
  bool res = true;

  // generic ALC setting
  uint8_t ALCSEL = 0b11;      // stereo
  uint8_t ALCLVL = 0b0011;    //-12db
  uint8_t MAXGAIN = 0b111;    //+35.5db
  uint8_t MINGAIN = 0b000;    //-12db
  uint8_t ALCHLD = 0b0000;    // 0ms
  uint8_t ALCDCY = 0b0101;    // 13.1ms/step
  uint8_t ALCATK = 0b0111;    // 13.3ms/step
  uint8_t ALCMODE = 0b0;      // ALC
  uint8_t ALCZC = 0b0;        // ZC off
  uint8_t TIME_OUT = 0b0;     // disable
  uint8_t NGAT = 0b1;         // enable
  uint8_t NGTH = 0b10001;     //-51db
  uint8_t NGG = 0b00;         // hold gain
  uint8_t WIN_SIZE = 0b00110; // default

  if (alc == DISABLE)
    ALCSEL = 0b00;
  else if (alc == MUSIC)
  {
    ALCDCY = 0b1010; // 420ms/step
    ALCATK = 0b0110; // 6.66ms/step
    NGTH = 0b01011;  // -60db
  }
  else if (alc == VOICE)
  {
    ALCLVL = 0b1100; // -4.5db
    MAXGAIN = 0b101; // +23.5db
    MINGAIN = 0b010; // 0db
    ALCDCY = 0b0001; // 820us/step
    ALCATK = 0b0010; // 416us/step
    NGTH = 0b11000;  // -40.5db
    NGG = 0b01;      // mute ADC
    res &= write_reg(ES8388_REG::ADCCONTROL1, 0x77);
  }
  res &= write_reg(ES8388_REG::ADCCONTROL10, ALCSEL << 6 | MAXGAIN << 3 | MINGAIN);
  res &= write_reg(ES8388_REG::ADCCONTROL11, ALCLVL << 4 | ALCHLD);
  res &= write_reg(ES8388_REG::ADCCONTROL12, ALCDCY << 4 | ALCATK);
  res &= write_reg(ES8388_REG::ADCCONTROL13,
                   ALCMODE << 7 | ALCZC << 6 | TIME_OUT << 5 | WIN_SIZE);
  res &= write_reg(ES8388_REG::ADCCONTROL14, NGTH << 3 | NGG << 2 | NGAT);

  return res;
}

// MIXIN1 – direct IN1 (default)
// MIXIN2 – direct IN2
// MIXRES – reserved es8388
// MIXADC – ADC/ALC input (after mic amplifier)
bool ES8388::mixerSourceSelect(mixsel_t LMIXSEL, mixsel_t RMIXSEL)
{
  bool res = true;
  uint8_t _reg;
  _reg = (LMIXSEL << 3) | RMIXSEL;
  res &= write_reg(ES8388_REG::DACCONTROL16, _reg);
  return res;
}

// LD/RD = DAC(i2s), false disable, true enable
// LI2LO/RI2RO from mixerSourceSelect(), false disable, true enable
// LOVOL = gain, 0 -> 6db, 1 -> 3db, 2 -> 0db, higher will attenuate
bool ES8388::mixerSourceControl(bool LD2LO, bool LI2LO, uint8_t LI2LOVOL,
                                bool RD2RO, bool RI2RO, uint8_t RI2LOVOL)
{
  bool res = true;
  uint8_t _regL, _regR;
  if (LI2LOVOL > 7)
    LI2LOVOL = 7;
  if (RI2LOVOL > 7)
    RI2LOVOL = 7;
  _regL = (LD2LO << 7) | (LI2LO << 6) | (LI2LOVOL << 3);
  _regR = (RD2RO << 7) | (RI2RO << 6) | (RI2LOVOL << 3);
  res &= write_reg(ES8388_REG::DACCONTROL17, _regL);
  res &= write_reg(ES8388_REG::DACCONTROL20, _regR);
  return res;
}

// Mixer source control
// DACOUT -> Select Sink From DAC
// SRCSEL -> Select Sink From SourceSelect()
// MIXALL -> Sink DACOUT + SRCSEL
bool ES8388::mixerSourceControl(mixercontrol_t mix)
{
  bool res = true;
  if (mix == DACOUT)
    mixerSourceControl(true, false, 2, true, false, 2);
  else if (mix == SRCSELOUT)
    mixerSourceControl(false, true, 2, false, true, 2);
  else if (mix == MIXALL)
    mixerSourceControl(true, true, 2, true, true, 2);
  return res;
}

// true -> analog out = analog in
// false -> analog out = DAC(i2s)
bool ES8388::analogBypass(bool bypass)
{
  bool res = true;
  if (bypass)
  {
    if (_inSel == IN1)
      mixerSourceSelect(MIXIN1, MIXIN1);
    else if (_inSel == IN2)
      mixerSourceSelect(MIXIN2, MIXIN2);
    mixerSourceControl(false, true, 2, false, true, 2);
  }
  else
  {
    mixerSourceControl(true, false, 2, true, false, 2);
  }
  return res;
}