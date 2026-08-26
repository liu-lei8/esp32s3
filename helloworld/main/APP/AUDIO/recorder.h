#pragma once    /*recorder.h 录音文件*/

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "my_es8311.h"
#include "xl9555.h"
#include "wavplay.h"

#define REC_RX_BUF_SIZE     1024*4      /*定义RX数组的大小*/
#define REC_SAMPLE_RATE     16000

void wav_recorder(void);