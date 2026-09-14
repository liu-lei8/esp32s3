#include "avi.h"

const char* avi = "avi";

AVI_INFO g_avix;        /*avi文件相关信息*/
const char* AVI_VIDS_FLAG_TBL[2] = {"00dc", "01dc"};
const char* AVI_AUDS_FLAG_TBL[2] = {"00wb", "01wb"};

uint32_t avi_search_id(uint8_t* buf, uint32_t size, char* id)
{
    uint32_t idsize = 0;

    for (uint32_t i = 0; i < size - 4; i++)
    {
        if (buf[i] == id[0] && buf[i + 1] == id[1] && buf[i + 2] == id[2] && buf[i + 3] == id[3])
        {
            idsize = MAKEDWORD(buf + i + 4);        /*得到帧的大小，必须大于16字节才返回，否则不是有效数据*/
            if (idsize > 0x10) return i;
        }
    }
    
    return 0;
}

AVISTATUS avi_get_streaminfo(uint8_t* buf)
{
    g_avix.StreamID = MAKEWORD(buf + 2);        /*获取流类型*/
    g_avix.StreamSize = MAKEDWORD(buf + 4);     /*获取流大小*/

    if (g_avix.StreamSize > AVI_MAX_FRAME_SIZE) /*帧大小太大了，直接返回错误*/
    {
        ESP_LOGE(avi, "Frame Size Over:%ld", g_avix.StreamSize);
        return AVI_STREAM_ERR;
    }

    if (g_avix.StreamSize % 2)      /*流的大小为奇数则加1，流的大小必须为偶数*/
    {
        g_avix.StreamSize++;
    }

    if (g_avix.StreamID == AVI_VIDS_FLAG || g_avix.StreamID == AVI_AUDS_FLAG)
    {
        return AVI_OK;
    }

    return AVI_STREAM_ERR;
}

AVISTATUS avi_init(uint8_t* buf, uint32_t size)
{
    uint16_t offset;
    uint8_t* tbuf;
    AVISTATUS res = AVI_OK;
    AVI_HEADER* aviheader;
    LIST_HEADER* listheader;
    AVIH_HEADER* avihheader;
    STRH_HEADER* strhheader;

    STRF_BMPHEADER* bmpheader;
    STRF_WAVHEADER* wavheader;

    tbuf = buf;

    aviheader = (AVI_HEADER*)buf;
    if (aviheader->RiffID != AVI_RIFF_ID)
    {
        return AVI_RIFF_ERR;        /* RIFF ID错误 */
    }

    if (aviheader->AviID != AVI_AVI_ID)
    {
        return AVI_AVI_ERR;         /* AVI ID错误 */
    }

    buf += sizeof(AVI_HEADER);      /* 偏移 */
    listheader = (LIST_HEADER*)buf;
    if (listheader->ListID != AVI_LIST_ID)
    {
        return AVI_LIST_ERR;        /* LIST ID错误 */
    }
    
    if (listheader->ListType != AVI_HDRL_ID)
    {
        return AVI_HDRL_ERR;        /* HDRL ID 错误 */
    }

    buf += sizeof(LIST_HEADER);     /* 偏移 */
    avihheader = (AVIH_HEADER*)buf;
    if (avihheader->BlockID != AVI_AVIH_ID)
    {
        return AVI_AVIH_ERR;        /* AVIH ID错误 */
    }

    g_avix.SecPerFrame = avihheader->SecPerFrame;   /* 获取帧的间隔时间 */
    g_avix.TotalFrame = avihheader->TotalFrame;     /* 获取总帧 */

    buf += avihheader->BlockSize + 8;   /* 偏移 */
    listheader = (LIST_HEADER*)buf;
    if (listheader->ListID != AVI_LIST_ID)
    {
        return AVI_LIST_ERR;        /* LIST ID错误 */
    }

    if (listheader->ListType != AVI_STRL_ID)
    {
        return AVI_STRL_ERR;        /* STRL ID错误 */
    }

    strhheader = (STRH_HEADER*)(buf + 12);
    if (strhheader->BlockID != AVI_STRH_ID)
    {
        return AVI_STRH_ERR;        /* STRH ID 错误 */
    }

    if (strhheader->StreamType == AVI_VIDS_STREAM)  /* 视频帧在前*/
    {
        if (strhheader->Handler != AVI_FORMAT_MJPG)
        {
            return AVI_FORMAT_ERR;  /* 非MJPEG视频格式，不支持 */
        }

        g_avix.VideoFLAG = AVI_VIDS_FLAG_TBL[0];    /* "00dc"视频标记 */
        g_avix.AudioFLAG = AVI_AUDS_FLAG_TBL[1];    /* "01wb"音频标记 */

        bmpheader = (STRF_BMPHEADER*)(buf + 12 + strhheader->BlockSize + 8);
        if (bmpheader->BlockID != AVI_STRF_ID)
        {
            return AVI_STRF_ERR;    /* STRF ID错误 */
        }
        
        g_avix.Width = bmpheader->bmHeader.Width;
        g_avix.Height = bmpheader->bmHeader.Height;

        buf += listheader->BlockSize + 8;   /* 偏移 */
        listheader = (LIST_HEADER*)buf;
        if (listheader->ListID != AVI_LIST_ID)  /* 不含有音频帧的视频文件*/
        {
            g_avix.SampleRate = 0;      /* 音频采样率 */
            g_avix.AudioType = 0;       /* 音频格式 */
            g_avix.Channels = 0;        /* 音频通道数 */
        }
        else                                    /* 有音频帧 */
        {
            if (listheader->ListType != AVI_STRL_ID)
            {
                return AVI_STRL_ERR;    /* STRL ID错误 */
            }

            strhheader = (STRH_HEADER*)(buf + 12);
            if (strhheader->BlockID != AVI_STRH_ID)
            {
                return AVI_STRH_ERR;    /* STRH ID错误 */
            }
            
            if (strhheader->StreamType != AVI_AUDS_STREAM)
            {
                return AVI_FORMAT_ERR;  /* 格式错误 */
            }

            wavheader = (STRF_WAVHEADER*)(buf + 12 + strhheader->BlockSize + 8);
            if (wavheader->BlockID != AVI_STRF_ID)
            {
                return AVI_STRF_ERR;    /* STRF ID错误 */
            }

            g_avix.SampleRate = wavheader->SampleRate;  /* 音频采样率 */
            g_avix.Channels = wavheader->Channels;      /* 音频通道数 */
            g_avix.AudioType = wavheader->FormatTag;    /* 音频格式 */
        }
    }
    else if (strhheader->StreamType == AVI_AUDS_STREAM) /* 音频帧在前 */
    {
        g_avix.VideoFLAG = AVI_VIDS_FLAG_TBL[1];        /* "01dc" 视频帧标记 */
        g_avix.AudioFLAG = AVI_AUDS_FLAG_TBL[0];        /* "00wb" 音频帧标记 */

        wavheader = (STRF_WAVHEADER*)(buf + 12 + strhheader->BlockSize + 8);
        if (wavheader->BlockID != AVI_STRF_ID)
        {
            return AVI_STRF_ERR;
        }

        g_avix.SampleRate = wavheader->SampleRate;
        g_avix.Channels = wavheader->Channels;
        g_avix.AudioType = wavheader->FormatTag;

        buf += listheader->BlockSize + 8;           /* 偏移 */
        listheader = (LIST_HEADER*)buf;
        if (listheader->ListID != AVI_LIST_ID)
        {
            return AVI_LIST_ERR;
        }

        if (listheader->ListType != AVI_STRL_ID)
        {
            return AVI_STRL_ERR;
        }

        strhheader = (STRH_HEADER*)(buf + 12);
        if (strhheader->BlockID != AVI_STRH_ID)
        {
            return AVI_STRH_ERR;
        }

        if (strhheader->StreamType != AVI_VIDS_STREAM)
        {
            return AVI_FORMAT_ERR;
        }

        bmpheader = (STRF_BMPHEADER*)(buf + 12 + strhheader->BlockSize + 8);
        if (bmpheader->BlockID != AVI_STRF_ID)
        {
            return AVI_STRF_ERR;
        }

        if (bmpheader->bmHeader.Compression != AVI_FORMAT_MJPG)
        {
            return AVI_FORMAT_ERR;
        }

        g_avix.Width = bmpheader->bmHeader.Width;
        g_avix.Height = bmpheader->bmHeader.Height;
    }

    offset = avi_search_id(tbuf, size, "movi");   /* 查找movi ID */
    if (offset == 0)
    {
        return AVI_MOVI_ERR;
    }

    if (g_avix.SampleRate)          /* 有音频流，才查找 */
    {
        tbuf += offset;
        offset = avi_search_id(tbuf, size, g_avix.AudioFLAG);       /* 查找音频流标记 */
        if (offset == 0)
        {
            return AVI_STREAM_ERR;
        }

        tbuf += offset + 4;
        g_avix.AudioBufSize = *((uint16_t*)tbuf);   /* 得到音频流buf的大小 */
    }

    ESP_LOGI(avi, "avi init ok");
    ESP_LOGI(avi, "g_avix.SecPerFrame:%ld", g_avix.SecPerFrame);
    ESP_LOGI(avi, "g_avix.TotalFrame:%ld", g_avix.TotalFrame);
    ESP_LOGI(avi, "g_avix.Width:%ld", g_avix.Width);
    ESP_LOGI(avi, "g_avix.Height:%ld", g_avix.Height);
    ESP_LOGI(avi, "g_avix.AudioType:%d", g_avix.AudioType);
    ESP_LOGI(avi, "g_avix.SampleRate:%ld", g_avix.SampleRate);
    ESP_LOGI(avi, "g_avix.Channels:%d", g_avix.Channels);
    ESP_LOGI(avi, "g_avix.AudioBufSize:%d", g_avix.AudioBufSize);
    ESP_LOGI(avi, "g_avix.VideoFLAG:%s", g_avix.VideoFLAG);
    ESP_LOGI(avi, "g_avix.AudioFLAG:%s", g_avix.AudioFLAG);

    return res;
}


