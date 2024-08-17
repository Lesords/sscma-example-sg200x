#include "video.h"

static int app_ipcam_Exit(void)
{
    APP_CHK_RET(app_ipcam_Venc_Stop(APP_VENC_ALL), "Venc Stop");
    APP_CHK_RET(app_ipcam_Vpss_DeInit(), "Vpss DeInit");
    APP_CHK_RET(app_ipcam_Vi_DeInit(), "Vi DeInit");
    APP_CHK_RET(app_ipcam_Sys_DeInit(), "System DeInit");

    return CVI_SUCCESS;
}

static int app_ipcam_Init(void)
{
    APP_CHK_RET(app_ipcam_Sys_Init(), "init systerm");
    APP_CHK_RET(app_ipcam_Vi_Init(), "init vi module");
    APP_CHK_RET(app_ipcam_Vpss_Init(), "init vpss module");
    APP_CHK_RET(app_ipcam_Venc_Init(APP_VENC_ALL), "init video encode");

    return CVI_SUCCESS;
}

int initVideo(void)
{
    /* load each moudles parameter from param_config.ini */
    APP_CHK_RET(app_ipcam_Param_Load(), "load global parameter");

    return 0;
}

int deinitVideo(void)
{
    app_ipcam_Exit();
}

int startVideo(void)
{
    /* init modules include <Peripheral; Sys; VI; VB; OSD; Venc; AI; Audio; etc.> */
    APP_CHK_RET(app_ipcam_Init(), "app_ipcam_Init");

    /* start video encode */
    APP_CHK_RET(app_ipcam_Venc_Start(APP_VENC_ALL), "start video processing");
}

int setupVideo(video_ch_index_t ch, const video_ch_param_t* param)
{
    if (ch >= VIDEO_CH_MAX) {
        APP_PROF_LOG_PRINT(LEVEL_ERROR, "video ch(%d) index is out of range\n", ch);
        return -1;
    }
    if (param == NULL) {
        APP_PROF_LOG_PRINT(LEVEL_ERROR, "video ch(%d) param is null\n", ch);
        return -1;
    }
    if (param->format >= VIDEO_FORMAT_INVALID) {
        APP_PROF_LOG_PRINT(LEVEL_ERROR, "video ch(%d) format(%d) is not support\n", ch, param->format);
        return -1;
    }

    APP_PARAM_SYS_CFG_S* sys = app_ipcam_Sys_Param_Get();
    APP_PARAM_VPSS_CFG_T* vpss = app_ipcam_Vpss_Param_Get();
    APP_PARAM_VENC_CTX_S* venc = app_ipcam_Venc_Param_Get();

    APP_PARAM_VB_CFG_S* vb = &sys->vb_pool[ch];
    vb->bEnable = CVI_TRUE;
    vb->width = param->width;
    vb->height = param->height;
    vb->fmt = (param->format == VIDEO_FORMAT_RGB888) ? PIXEL_FORMAT_RGB_888 : PIXEL_FORMAT_NV21;

    VPSS_CHN_ATTR_S* vpss_chn = &vpss->astVpssGrpCfg[0].astVpssChnAttr[ch];
    vpss_chn->u32Width = param->width;
    vpss_chn->u32Height = param->height;
    vpss_chn->enPixelFormat = (param->format == VIDEO_FORMAT_RGB888) ? PIXEL_FORMAT_RGB_888 : PIXEL_FORMAT_NV21;

    APP_VENC_CHN_CFG_S* venc_chn = &venc->astVencChnCfg[ch];
    venc_chn->u32Width = param->width;
    venc_chn->u32Height = param->height;
    venc_chn->u32DstFrameRate = param->fps;
    if (param->format == VIDEO_FORMAT_RGB888) {
        venc_chn->enType = PT_JPEG; // for special use
    } else if (param->format == VIDEO_FORMAT_H264) {
        venc_chn->enType = PT_H264;
    } else if (param->format == VIDEO_FORMAT_H265) {
        venc_chn->enType = PT_H265;
    } /*else if (param->format == VIDEO_FORMAT_JPEG) {
        venc_chn->enType = PT_JPEG;
    }*/

    APP_PROF_LOG_PRINT(LEVEL_INFO, "venc_chn=%d, enType=%d, enBindMode=%d\n", ch, venc_chn->enType, venc_chn->enBindMode);

    return 0;
}

int registerVideoFrameHandler(video_ch_index_t ch, int index, video_data_handler handler)
{
    app_ipcam_Venc_Consumes(ch, index, handler);
    return 0;
}