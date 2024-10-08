/**
 * The treats the 16bit data as 16bit signed.
 * 
 * 0x7FFF : 32767 : Vout minimum
 * 0x0000 : flat line 25mV
 * 0x8000 : -32768 : Vout maximum
 */

#include "ES8388Output.h"
#include "SampleSource.h"

ES8388Output::ES8388Output(i2s_port_t i2sPort, i2s_pin_config_t* pin_config) : I2SOutput(i2sPort, pin_config)
{
}

void ES8388Output::start(SampleSource *sample_generator)
{
	i2s_config_t m_i2sConfig = {
		.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX), // Only TX
		.sample_rate = sample_generator->sampleRate(),
		.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT, // 16-bit per channel
		.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT, // 2-channels
		.communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_STAND_I2S),
		.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1, //Interrupt level 1
		.dma_buf_count = 2,
		.dma_buf_len = 256,
		.use_apll = true,
		.tx_desc_auto_clear = true,
		.fixed_mclk = (int)sample_generator->sampleRate() * 256};
    I2SOutput::start(&m_i2sConfig, sample_generator);
}

void ES8388Output::configureI2S()
{
    // Enable MCLK output
    WRITE_PERI_REG(PIN_CTRL, READ_PERI_REG(PIN_CTRL) & 0xFFFFFFF0);
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0_CLK_OUT1);
    delay(5);
}
