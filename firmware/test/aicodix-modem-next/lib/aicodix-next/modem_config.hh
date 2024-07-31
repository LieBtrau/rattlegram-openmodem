#pragma once
#include <cstdint>

typedef struct {
    int oper_mode;  // Operating mode
    int band_width; // Occupied Bandwidth
    int mod_bits;   // Number of bits per symbol
    int cons_rows;  
    int comb_cols;  
    int code_order; // Code order for polar codes
    int code_cols;
    int reserved_tones;
} modem_config_t;

/// @brief Modem configurations
static const modem_config_t modem_configs[] = 
{
    { 0, 1600, 0, 0, 0, 0, 256, 0 },        // No payload
    { 20, 1600, 4, 1, 0, 10, 256, 0 },      // 64 bytes, QAM16
    { 21, 1600, 4, 2, 0, 11, 256, 0 },      // 128 bytes, QAM16
    { 22, 1600, 3, 3, 0, 12, 256, 0 },      // 256 bytes, 8PSK
    { 23, 1600, 2, 8, 0, 12, 256, 0 },      // 256 bytes, QPSK
    { 24, 1600, 3, 10, 0, 13, 256, 0 },     // 512 bytes, 8PSK
    { 25, 1600, 2, 32, 0, 14, 256, 0 },     // 1024 bytes, QPSK
    { 26, 1700, 4, 4, 8, 12, 256, 8 },      // 256 bytes, QAM16
    { 27, 1700, 4, 8, 8, 13, 256, 8 },      // 512 bytes, QAM16
    { 28, 1700, 4, 16, 8, 14, 256, 8 },     // 1024 bytes, QAM16
    { 29, 1900, 6, 5, 16, 13, 273, 15 },    // 512 bytes, QAM64
    { 30, 1900, 6, 10, 16, 14, 273, 15 }    // 1024 bytes, QAM64
};
