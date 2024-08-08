/**
 * @file encode.hh
 * @author Christoph Tack
 * @brief OFDM-modem encoder
 * @version 0.1
 * @date 2024-07-28
 * 
 * @copyright Copyright (c) 2024
 * @note Based on the [original code](https://github.com/aicodix/modem/tree/next) from Ahmet Inan <inan@aicodix.de>, Copyright 2021
 */

#include <iostream>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <cmath>
#include "xorshift.hh"
#include "complex.hh"
#include "permute.hh"
#include "utils.hh"
#include "bitman.hh"
#include "decibel.hh"
#include "fft.hh"
#include "wav.hh"
#include "pcm.hh"
#include "mls.hh"
#include "crc.hh"
#include "psk.hh"
#include "qam.hh"
#include "polar_tables.hh"
#include "polar_parity_aided.hh"
#include "bose_chaudhuri_hocquenghem_encoder.hh"
#include "modem_config.hh"

template <typename value, typename cmplx, int rate>
struct Encoder
{

private:
	typedef int8_t code_type;
	static const int symbol_len = (1280 * rate) / 8000;
	static const int guard_len = symbol_len / 8;
	static const int bits_max = 16384;
	static const int data_max = 1024;
	static const int cols_max = 273 + 16;
	static const int mls0_len = 127;
	static const int mls0_poly = 0b10001001;
	static const int mls1_len = 255;
	static const int mls1_poly = 0b100101011;
	static const int mls2_poly = 0b100101010001;
	DSP::FastFourierTransform<symbol_len, cmplx, -1> fwd;
	DSP::FastFourierTransform<symbol_len, cmplx, 1> bwd;
	CODE::CRC<uint16_t> crc0;
	CODE::CRC<uint32_t> crc1;
	CODE::BoseChaudhuriHocquenghemEncoder<mls1_len, 71> bchenc;
	CODE::PolarParityEncoder<code_type> polarenc;
	CODE::FisherYatesShuffle<1024> shuffle_1024;
	CODE::FisherYatesShuffle<2048> shuffle_2048;
	CODE::FisherYatesShuffle<4096> shuffle_4096;
	CODE::FisherYatesShuffle<8192> shuffle_8192;
	CODE::FisherYatesShuffle<16384> shuffle_16384;
	uint8_t input_data[data_max];
	code_type code[bits_max], mesg[bits_max];
	cmplx fdom[symbol_len];
	cmplx tdom[symbol_len];
	cmplx temp[symbol_len];
	cmplx kern[symbol_len];
	cmplx guard[guard_len];
	cmplx prev[cols_max];
	value papr_min, papr_max;
	int16_t samples[symbol_len + guard_len];
	int mod_bits;
	int oper_mode;
	int code_order; // Polar encoder order : 2**code_order = number of data bits
	int code_off;
	int cons_cols;
	int cons_rows;
	int mls0_off;
	int mls1_off;
	int comb_dist = 1;
	int comb_off = 1;
	int reserved_tones = 0;
	void (*sampleSink)(int16_t samples[], int count) { nullptr }; 

	static int bin(int carrier)
	{
		return (carrier + symbol_len) % symbol_len;
	}
	static int nrz(bool bit)
	{
		return 1 - 2 * bit;
	}
	void clipping_and_filtering(value scale, bool limit)
	{
		for (int i = 0; i < symbol_len; ++i) {
			value pwr = norm(tdom[i]);
			if (pwr > value(1))
				tdom[i] /= sqrt(pwr);
		}
		fwd(temp, tdom);
		for (int i = 0; i < symbol_len; ++i) {
			if (norm(fdom[i])) {
				temp[i] *= scale / std::sqrt(value(symbol_len));
				cmplx err = temp[i] - fdom[i];
				value mag = abs(err);
				value lim = 0.1 * mod_distance();
				if (limit && mag > lim)
					temp[i] -= ((mag - lim) / mag) * err;
			} else {
				temp[i] = 0;
			}
		}
		bwd(tdom, temp);
		for (int i = 0; i < symbol_len; ++i)
			tdom[i] /= scale * std::sqrt(value(symbol_len));
	}
	void tone_reservation()
	{
		for (int n = 0; n < 100; ++n) {
			int peak = 0;
			for (int i = 1; i < symbol_len; ++i)
				if (norm(tdom[peak]) < norm(tdom[i]))
					peak = i;
			cmplx orig = tdom[peak];
			if (norm(orig) <= value(1))
				break;
			for (int i = 0; i < symbol_len; ++i)
				tdom[i] -= orig * kern[(symbol_len-peak+i)%symbol_len];
		}
	}

	/**
	 * @brief Generate a symbol
	 * @param papr_reduction Enable PAPR reduction
	 */
	void symbol(bool papr_reduction = true)
	{
		// IFFT operation
		bwd(tdom, fdom);

		// Normalize the symbol
		value scale = 2;
		for (int i = 0; i < symbol_len; ++i)
			tdom[i] /= scale * std::sqrt(value(symbol_len));
		// TODO
		clipping_and_filtering(scale, /*oper_mode > 25*/reserved_tones && papr_reduction);

		// Tone reservation for QAM
		// TODO
		if (/*oper_mode > 25*/reserved_tones && papr_reduction)
			tone_reservation();
		// Remove zeros from the symbol
		for (int i = 0; i < symbol_len; ++i)
			tdom[i] = cmplx(std::min(value(1), tdom[i].real()), std::min(value(1), tdom[i].imag()));

		// Calculate the PAPR for the symbol, for reference purposes
		value peak = 0, mean = 0;
		for (int i = 0; i < symbol_len; ++i) {
			value power(norm(tdom[i]));
			peak = std::max(peak, power);
			mean += power;
		}
		mean /= symbol_len;
		if (mean > 0) {
			value papr(peak / mean);
			papr_min = std::min(papr_min, papr);
			papr_max = std::max(papr_max, papr);
		}

		// Calculate the guard interval
		for (int i = 0; i < guard_len; ++i) {
			value x = value(i) / value(guard_len - 1);
			value ratio(0.5);
			x = std::min(x, ratio) / ratio;
			x = value(0.5) * (value(1) - std::cos(DSP::Const<value>::Pi() * x));
			guard[i] = DSP::lerp(guard[i], tdom[i+symbol_len-guard_len], x);
		}

		// Write the real part of the complex signals (guard & tdom) to a buffer.  Ignore the imaginary part.
		for (int i = 0; i < guard_len; ++i)
		{	
			// Write guard interval to the buffer
			samples[i] = std::clamp<value>(std::nearbyint(32767 * guard[i].real()), -32768, 32767);
			// Cyclic prefix [https://en.wikipedia.org/wiki/Cyclic_prefix]
			guard[i] = tdom[i];
		}
		for (int i = 0; i < symbol_len; ++i)
		{
			// Write the OFDM-symbol to the buffer
			samples[i + guard_len] = std::clamp<value>(std::nearbyint(32767 * tdom[i].real()), -32768, 32767);
		}
		if(sampleSink)
		{
			// Send the buffer to the sampleSink
			sampleSink(samples, symbol_len + guard_len);
		}
	}

	cmplx mod_map(code_type *b)
	{
		switch (mod_bits) {
		case 2:
			return PhaseShiftKeying<4, cmplx, code_type>::map(b);
		case 3:
			return PhaseShiftKeying<8, cmplx, code_type>::map(b);
		case 4:
			return QuadratureAmplitudeModulation<16, cmplx, code_type>::map(b);
		case 6:
			return QuadratureAmplitudeModulation<64, cmplx, code_type>::map(b);
		}
		return 0;
	}
	value mod_distance()
	{
		switch (mod_bits) {
		case 2:
			return PhaseShiftKeying<4, cmplx, code_type>::DIST;
		case 3:
			return PhaseShiftKeying<8, cmplx, code_type>::DIST;
		case 4:
			return QuadratureAmplitudeModulation<16, cmplx, code_type>::DIST;
		case 6:
			return QuadratureAmplitudeModulation<64, cmplx, code_type>::DIST;
		}
		return 2;
	}
			
public:
	Encoder() : crc0(0xA8F4), crc1(0x8F6E37A0), bchenc({
			0b100011101, 0b101110111, 0b111110011, 0b101101001,
			0b110111101, 0b111100111, 0b100101011, 0b111010111,
			0b000010011, 0b101100101, 0b110001011, 0b101100011,
			0b100011011, 0b100111111, 0b110001101, 0b100101101,
			0b101011111, 0b111111001, 0b111000011, 0b100111001,
			0b110101001, 0b000011111, 0b110000111, 0b110110001}){}

	/**
	 * @brief Configure the OFDM-encoder
	 * 
	 * @param freq_off audio center frequency of the OFDM signal
	 * @param metadata_symbol metadata to be sent, only lowest 55 bits will be used
	 * @param modem_config a pointer to the modem configuration
	 * @return true valid configuration
	 * @return false invalid configuration
	 */
	bool configure(int freq_off, uint64_t metadata_symbol, const modem_config_t* modem_config)
	{
		if (freq_off % 50)
		{
			std::cerr << "Frequency offset must be divisible by 50." << std::endl;
			return false;
		}
		oper_mode = modem_config->oper_mode;
		int band_width = modem_config->band_width;
		mod_bits = modem_config->mod_bits;
		cons_rows = modem_config->cons_rows;
		int comb_cols = modem_config->comb_cols;
		code_order = modem_config->code_order;
		int code_cols = modem_config->code_cols;
		reserved_tones = modem_config->reserved_tones;


		if (freq_off < band_width / 2 - rate / 2 || freq_off > rate / 2 - band_width / 2)
		{
			std::cerr << "Unsupported frequency offset." << std::endl;
			return false;
		}
		int offset = (freq_off * symbol_len) / rate;
		mls0_off = offset - mls0_len + 1;
		mls1_off = offset - mls1_len / 2;
		cons_cols = code_cols + comb_cols;
		code_off = offset - cons_cols / 2;
		if (oper_mode > 0) {
			comb_dist = comb_cols ? cons_cols / comb_cols : 1;
			comb_off = comb_cols ? comb_dist / 2 : 1;
			if (reserved_tones) {
				value kern_fac = 1 / value(10 * reserved_tones);
				for (int i = 0, j = code_off - reserved_tones / 2; i < reserved_tones; ++i, ++j) {
					if (j == code_off)
						j += cons_cols;
					fdom[bin(j)] = kern_fac;
				}
				bwd(kern, fdom);
			}
		}
		papr_min = 1000, papr_max = -1000;
		return true;
	}

	/**
	 * @brief Generate a noise block
	 * @note This can be used to open squelch when using VOX on the transceiver.
	 */
	void noise_block()
	{
		CODE::MLS seq2(mls2_poly);
		// Calculate the scaling factor for the pilot block
		value code_fac = std::sqrt(value(symbol_len) / value(cons_cols));
		std::memset(fdom, 0, sizeof(fdom));
		for (int i = code_off; i < code_off + cons_cols; ++i)
		{
			fdom[bin(i)] = code_fac * nrz(seq2());
		}
		symbol();
	}

	/**
	 * @brief Synchronization symbol
	 * [Robust Frequency and Timing Synchronization for OFDM](https://home.mit.bme.hu/~kollar/papers/Schmidl2.pdf)
	 * 
	 * @note It must be sent at least once at the beginning of the transmission.  This allows the receiver to synchronize to the transmitter.
	 * @note Schmidl-Cox synchronization symbol
	 */
	void synchronization_symbol()
	{
		CODE::MLS seq0(mls0_poly);
		value mls0_fac = std::sqrt(value(2 * symbol_len) / value(mls0_len));
		std::memset(fdom, 0, sizeof(fdom));
		fdom[bin(mls0_off-2)] = mls0_fac;
		for (int i = 0; i < mls0_len; ++i)
			fdom[bin(2*i+mls0_off)] = nrz(seq0());
		for (int i = 0; i < mls0_len; ++i)
			fdom[bin(2*i+mls0_off)] *= fdom[bin(2*(i-1)+mls0_off)];
		symbol(false);
	}

	/**
	 * @brief Generate the metadata symbol
	 * @param md Metadata to be sent, only lowest 55-8 bits will be used
	 * @note This function will add the operating mode to the metadata.
	 */
	void metadata_symbol(uint64_t md)
	{
		md = (md << 8) | oper_mode;
		uint8_t data[9] = { 0 }, // 71 bits : 55 bits of metadata + 16 bits of CRC
			parity[23] = { 0 };	// 23*8 = 184 bits
		// Total number of bits = 71 + 184 = 255
		for (int i = 0; i < 55; ++i)
			CODE::set_be_bit(data, i, (md>>i)&1);
		crc0.reset();
		uint16_t cs = crc0(md << 9);
		for (int i = 0; i < 16; ++i)
			CODE::set_be_bit(data, i+55, (cs>>i)&1);
		bchenc(data, parity);
		CODE::MLS seq1(mls1_poly);
		value cons_fac = std::sqrt(value(symbol_len) / value(cons_cols));
		for (int i = 0; i < symbol_len; ++i)
			fdom[i] = 0;
		fdom[bin(mls1_off-1)] = cons_fac;
		for (int i = 0; i < 71; ++i)
			fdom[bin(i+mls1_off)] = nrz(CODE::get_be_bit(data, i));
		for (int i = 71; i < mls1_len; ++i)
			fdom[bin(i+mls1_off)] = nrz(CODE::get_be_bit(parity, i-71));
		for (int i = 0; i < mls1_len; ++i)
			fdom[bin(i+mls1_off)] *= fdom[bin(i-1+mls1_off)];
		for (int i = 0; i < mls1_len; ++i)
			fdom[bin(i+mls1_off)] *= nrz(seq1());
		if (reserved_tones/*oper_mode > 25*/) {
			for (int i = code_off; i < code_off + cons_cols; ++i) {
				if (i == mls1_off-1)
					i += mls1_len + 1;
				fdom[bin(i)] = cons_fac * nrz(seq1());
			}
		}
		symbol();
	}

	/**
	 * @brief Generate a data packet
	 * 
	 * @param data data bytes to be sent
	 * @param len number of bytes to be sent
	 * @return false when packet size is too large for the chosen operating mode
	 */
	bool data_packet(uint8_t *data, int len)
	{
		if (len > (1 << (code_order - 4)))
		{
			std::cerr << "Packet too large." << std::endl;
			return false;
		}
		std::memset(input_data, 0, sizeof(input_data));
		std::memcpy(input_data, data, len);
		int data_bits = 1 << (code_order -1);
		int data_bytes = data_bits / 8;
		// Scramble the data
		CODE::Xorshift32 scrambler;
		for (int i = 0; i < data_bytes; ++i)
			input_data[i] ^= scrambler();
		// Convert the data to NRZ
		for (int i = 0; i < data_bits; ++i)
			mesg[i] = nrz(CODE::get_le_bit(input_data, i));
		// Add CRC parity bits
		crc1.reset();
		for (int i = 0; i < data_bytes; ++i)
			crc1(input_data[i]);
		for (int i = 0; i < 32; ++i)
			mesg[i+data_bits] = nrz((crc1()>>i)&1);

		/**
		 * Polar encoding adds redundancy to the data to make it more robust against errors
		 * Shuffling (or interleaving) the data is a way to make the data more robust against burst errors
		 */
		switch(code_order) {
		case 10:
			polarenc(code, mesg, frozen_1024_562, code_order, 31, 3);
			shuffle_1024(code);
			break;
		case 11:
			polarenc(code, mesg, frozen_2048_1090, code_order, 31, 3);
			shuffle_2048(code);
			break;
		case 12:
			polarenc(code, mesg, frozen_4096_2147, code_order, 31, 3);
			shuffle_4096(code);
			break;
		case 13:
			polarenc(code, mesg, frozen_8192_4261, code_order, 31, 5);
			shuffle_8192(code);
			break;
		case 14:
			polarenc(code, mesg, frozen_16384_8489, code_order, 31, 9);
			shuffle_16384(code);
			break;
		}

		// Modulating the data
		for (int i = 0; i < cons_cols; ++i)
			prev[i] = fdom[bin(i+code_off)];
		CODE::MLS seq0(mls0_poly);
		for (int j = 0, k = 0; j < cons_rows; ++j) {
			for (int i = 0; i < cons_cols; ++i) {
				if (/*oper_mode < 26*/!reserved_tones) {
					prev[i] *= mod_map(code+k);
					fdom[bin(i+code_off)] = prev[i];
					k += mod_bits;
				} else if (i % comb_dist == comb_off) {
					prev[i] *= nrz(seq0());
					fdom[bin(i+code_off)] = prev[i];
				} else {
					fdom[bin(i+code_off)] = prev[i] * mod_map(code+k);
					k += mod_bits;
				}
			}
			symbol();
		}
		std::cerr << "PAPR: " << DSP::decibel(papr_min) << " .. " << DSP::decibel(papr_max) << " dB" << std::endl;
		return true;
	}

	/**
	 * @brief Empty packet
	 * 
	 */
	void silence_packet()
	{
		std::memset(fdom, 0, sizeof(fdom));
		symbol();
	}

	void setSampleSink(void (*sink)(int16_t samples[], int count))
	{
		sampleSink = sink;
	}

	int getSymbolLen()
	{
		return symbol_len;
	}
	int getGuardLen()
	{
		return guard_len;
	}

};


