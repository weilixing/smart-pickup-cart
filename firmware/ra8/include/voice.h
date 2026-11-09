#pragma once

#include <stdint.h>
#include <stdbool.h>

void voice_init(void);
void voice_play_index(uint8_t index);  /* 与上层 PHRASES 编号一致 */
void voice_update(void);
bool voice_busy(void);
uint8_t voice_last_done_index(void);   /* 完成后返回索引，调用方读走后清 0xFF */
