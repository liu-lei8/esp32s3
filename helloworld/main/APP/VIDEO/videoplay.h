#pragma once /* videoplay.h */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "avi.h"
#include "mjpeg.h"
#include "esptim_video.h"
#include "ff.h"
#include "esp_vfs_fat.h"
#include "exfuns.h"
#include "text.h"
#include "my_es8311.h"

#define AVI_VIDEO_BUF_SIZE      (30 * 1024 + 8)         /* 一帧图片大小 + 下一帧ID信息大小 */

void video_play(void);          /* VIDEO文件播放程序 */
void video_task(void *arg);
void audio_task(void *arg);