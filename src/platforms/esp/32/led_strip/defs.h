#pragma once

#define LED_STRIP_RMT_DEFAULT_RESOLUTION 10000000 // 10MHz resolution
#define INTERRUPT_PRIORITY 3  // maximum priority level for RMT interrupt

#ifndef LED_STRIP_RMT_DEFAULT_TRANS_QUEUE_SIZE
#define LED_STRIP_RMT_DEFAULT_TRANS_QUEUE_SIZE 4
#endif  // LED_STRIP_RMT_DEFAULT_TRANS_QUEUE_SIZE

// the memory size of each RMT channel, in words (4 bytes)
#ifndef LED_STRIP_RMT_DEFAULT_MEM_BLOCK_SYMBOLS
#if CONFIG_IDF_TARGET_ESP32 || CONFIG_IDF_TARGET_ESP32S2
#define LED_STRIP_RMT_DEFAULT_MEM_BLOCK_SYMBOLS 64
#else
#define LED_STRIP_RMT_DEFAULT_MEM_BLOCK_SYMBOLS 48
#endif
#endif  // LED_STRIP_RMT_DEFAULT_MEM_BLOCK_SYMBOLS

#ifndef FASTLED_RMT_WITH_DMA
#define FASTLED_RMT_WITH_DMA 1
#else
#define FASTLED_RMT_WITH_DMA 0
#endif  // FASTLED_RMT_WITH_DMA

#ifndef FASTLED_RMT_MEMBLOCK_SYMBOLS
#if FASTLED_RMT_WITH_DMA
#define FASTLED_RMT_MEMBLOCK_SYMBOLS 1024
#else
#define FASTLED_RMT_MEMBLOCK_SYMBOLS 0  // Let the library decide
#endif  // FASTLED_RMT_WITH_DMA
#endif  // FASTLED_RMT_MEMBLOCK_SYMBOLS