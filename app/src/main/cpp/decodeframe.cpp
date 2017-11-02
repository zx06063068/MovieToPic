
#include <jni.h>
#include <string>

extern "C"
{
#include <libavutil/log.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>


#include <android/log.h>
#define LOGE(format, ...)  __android_log_print(ANDROID_LOG_ERROR, "(>_<)", format, ##__VA_ARGS__)
#define LOGI(format, ...)  __android_log_print(ANDROID_LOG_INFO,  "(^_^)", format, ##__VA_ARGS__)
#define BI_RGB 0x0
typedef struct tagBITMAPFILEHEADER {
    unsigned short bfType;      //2 位图文件的类型，必须为“BM”
    unsigned long bfSize;       //4 位图文件的大小，以字节为单位
    unsigned short bfReserved1; //2 位图文件保留字，必须为0
    unsigned short bfReserved2; //2 位图文件保留字，必须为0
    unsigned long bfOffBits;    //4 位图数据的起始位置，以相对于位图文件头的偏移量表示，以字节为单位
} BITMAPFILEHEADER;           //该结构占据14个字节。
typedef struct tagBITMAPINFOHEADER {
    unsigned long biSize;       //4 本结构所占用字节数
    long biWidth;               //4 位图的宽度，以像素为单位
    long biHeight;              //4 位图的高度，以像素为单位
    unsigned short biPlanes;    //2 目标设备的平面数不清，必须为1
    unsigned short biBitCount;  //2 每个像素所需的位数，必须是1(双色), 4(16色)，8(256色)或24(真彩色)之一
    unsigned long biCompression;
    //4 位图压缩类型，必须是 0(不压缩),1(BI_RLE8压缩类型)或2(BI_RLE4压缩类型)之一
    unsigned long biSizeImage;  //4 位图的大小，以字节为单位
    long biXPelsPerMeter;       //4 位图水平分辨率，每米像素数
    long biYPelsPerMeter;       //4 位图垂直分辨率，每米像素数
    unsigned long biClrUsed;    //4 位图实际使用的颜色表中的颜色数
    unsigned long biClrImportant;//4 位图显示过程中重要的颜色数
} BITMAPINFOHEADER;

//Output FFmpeg's av_log()
void custom_log(void *ptr, int level, const char* fmt, va_list vl){
    FILE *fp=fopen("/storage/emulated/0/av_log.txt","a+");
    if(fp){
        vfprintf(fp,fmt,vl);
        fflush(fp);
        fclose(fp);
    }
}
int SaveFrameToBMP(char *pPicFile, uint8_t *pRGBBuffer, int nWidth, int nHeight, int nBitCount) {
    BITMAPFILEHEADER bmpheader;
    BITMAPINFOHEADER bmpinfo;
    memset(&bmpheader, 0, sizeof(BITMAPFILEHEADER));
    memset(&bmpinfo, 0, sizeof(BITMAPINFOHEADER));

    FILE *fp = NULL;
    fp = fopen(pPicFile, "wb");
    if (NULL == fp) {
        LOGE("file open error");
        return -1;
    }
    // set BITMAPFILEHEADER value
    bmpheader.bfType = ('M' << 8) | 'B';
    bmpheader.bfReserved1 = 0;
    bmpheader.bfReserved2 = 0;
    bmpheader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER)-2;
    bmpheader.bfSize = bmpheader.bfOffBits + nWidth * nHeight * nBitCount / 8;
    // set BITMAPINFOHEADER value
    bmpinfo.biSize = sizeof(BITMAPINFOHEADER);
    bmpinfo.biWidth = nWidth;
    bmpinfo.biHeight = 0 - nHeight;
    bmpinfo.biPlanes = 1;
    bmpinfo.biBitCount = nBitCount;
    bmpinfo.biCompression = BI_RGB;
    bmpinfo.biSizeImage = 0;
    bmpinfo.biXPelsPerMeter = 100;
    bmpinfo.biYPelsPerMeter = 100;
    bmpinfo.biClrUsed = 0;
    bmpinfo.biClrImportant = 0;
    // write pic file
    fwrite(&bmpheader.bfType, sizeof(bmpheader.bfType), 1, fp);
    fwrite(&bmpheader.bfSize, sizeof(bmpheader.bfSize), 1, fp);
    fwrite(&bmpheader.bfReserved1, sizeof(bmpheader.bfReserved1), 1, fp);
    fwrite(&bmpheader.bfReserved2, sizeof(bmpheader.bfReserved2), 1, fp);
    fwrite(&bmpheader.bfOffBits, sizeof(bmpheader.bfOffBits), 1, fp);

    fwrite(&bmpinfo.biSize, sizeof(bmpinfo.biSize), 1, fp);
    fwrite(&bmpinfo.biWidth, sizeof(bmpinfo.biWidth), 1, fp);
    fwrite(&bmpinfo.biHeight, sizeof(bmpinfo.biHeight), 1, fp);
    fwrite(&bmpinfo.biPlanes, sizeof(bmpinfo.biPlanes), 1, fp);
    fwrite(&bmpinfo.biBitCount, sizeof(bmpinfo.biBitCount), 1, fp);
    fwrite(&bmpinfo.biCompression, sizeof(bmpinfo.biCompression), 1, fp);
    fwrite(&bmpinfo.biSizeImage, sizeof(bmpinfo.biSizeImage), 1, fp);
    fwrite(&bmpinfo.biXPelsPerMeter, sizeof(bmpinfo.biXPelsPerMeter), 1, fp);
    fwrite(&bmpinfo.biYPelsPerMeter, sizeof(bmpinfo.biYPelsPerMeter), 1, fp);
    fwrite(&bmpinfo.biClrUsed, sizeof(bmpinfo.biClrUsed), 1, fp);
    fwrite(&bmpinfo.biClrImportant, sizeof(bmpinfo.biClrImportant), 1, fp);
    fwrite(pRGBBuffer, nWidth * nHeight * nBitCount / 8, 1, fp);
    fclose(fp);
    LOGI("write over");
    return 0;
}

JNIEXPORT jint JNICALL
Java_com_phicomm_decodeutil_DecodeFrameUtil_decode(
        JNIEnv *env,
        jobject , jstring input_jstr, jstring output_jstr) {

    AVFormatContext *pFormatCtx;
    int             i, videoindex;
    AVCodecContext  *pCodecCtx;
    AVCodec         *pCodec;
    AVFrame *pFrame,*pFrameYUV;
    uint8_t *out_buffer;
    AVPacket *packet;
    //int y_size;
    int ret, got_picture;
    int picoutid=0;
    struct SwsContext *img_convert_ctx;
    //FILE *fp_yuv;
    int frame_cnt;
    clock_t time_start, time_finish;
    double  time_duration = 0.0;

    char input_str[500]={0};
    char output_str[500]={0};
    char info[1000]={0};
    sprintf(input_str,"%s",env->GetStringUTFChars(input_jstr, NULL));
    sprintf(output_str,"%s",env->GetStringUTFChars(output_jstr, NULL));

    //FFmpeg av_log() callback
    av_log_set_callback(custom_log);

    av_register_all();
    avformat_network_init();
    pFormatCtx = avformat_alloc_context();

    if(avformat_open_input(&pFormatCtx,input_str,NULL,NULL)!=0){
        LOGE("Couldn't open input stream.\n");
        return -1;
    }
    if(avformat_find_stream_info(pFormatCtx,NULL)<0){
        LOGE("Couldn't find stream information.\n");
        return -1;
    }
    videoindex=-1;
    for(i=0; i<pFormatCtx->nb_streams; i++)
        if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO){
            videoindex=i;
            break;
        }
    if(videoindex==-1){
        LOGE("Couldn't find a video stream.\n");
        return -1;
    }
    pCodecCtx=pFormatCtx->streams[videoindex]->codec;
    pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
    if(pCodec==NULL){
        LOGE("Couldn't find Codec.\n");
        return -1;
    }
    if(avcodec_open2(pCodecCtx, pCodec,NULL)<0){
        LOGE("Couldn't open codec.\n");
        return -1;
    }

    pFrame=av_frame_alloc();
    pFrameYUV=av_frame_alloc();
    out_buffer=(unsigned char *)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_YUV420P,  pCodecCtx->width, pCodecCtx->height,1));
    av_image_fill_arrays(pFrameYUV->data, pFrameYUV->linesize,out_buffer,
                         AV_PIX_FMT_YUV420P,pCodecCtx->width, pCodecCtx->height,1);


    packet=(AVPacket *)av_malloc(sizeof(AVPacket));

    img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
                                     pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);


    sprintf(info,   "[Input     ]%s\n", input_str);
    sprintf(info, "%s[Output    ]%s\n",info,output_str);
    sprintf(info, "%s[Format    ]%s\n",info, pFormatCtx->iformat->name);
    sprintf(info, "%s[Codec     ]%s\n",info, pCodecCtx->codec->name);
    sprintf(info, "%s[Resolution]%dx%d\n",info, pCodecCtx->width,pCodecCtx->height);

    /*fp_yuv=fopen(output_str,"wb+");
    if(fp_yuv==NULL){
        printf("Cannot open output file.\n");
        return -1;
    }*/
    frame_cnt=0;
    time_start = clock();

    while (av_read_frame(pFormatCtx, packet) >= 0 && picoutid < 8) {
        if(packet->stream_index==videoindex){
            ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
            if(ret < 0){
                LOGE("Decode Error.\n");
                return -1;
            }
            if(got_picture){
                /*sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height,
                          pFrameYUV->data, pFrameYUV->linesize);

                y_size=pCodecCtx->width*pCodecCtx->height;
                fwrite(pFrameYUV->data[0],1,y_size,fp_yuv);    //Y
                fwrite(pFrameYUV->data[1],1,y_size/4,fp_yuv);  //U
                fwrite(pFrameYUV->data[2],1,y_size/4,fp_yuv);  //V*/
                //Output info
                char pictype_str[10]={0};
                switch(pFrame->pict_type){
                    case AV_PICTURE_TYPE_I: {
                        sprintf(pictype_str, "I");
                        char szPicFile[256];
                        picoutid++;
                        memset(szPicFile, 0, sizeof(szPicFile));
                        strncpy(szPicFile, output_str, sizeof(szPicFile));
                        char sframecnt[3];
                        sprintf(sframecnt, "%d", picoutid);
                        strncat(szPicFile, sframecnt, sizeof(szPicFile));
                        strncat(szPicFile, ".bmp", sizeof(szPicFile));
                        // Convert the image from its native format to RGB
                        AVFrame *pFrameRGB = NULL;
                        // Allocate an AVFrame structure
                        pFrameRGB = av_frame_alloc();
                        if (pFrameRGB == NULL) {
                            LOGI("Allocate an AVFrame structure error.");
                            return -1;
                        }
                        uint8_t *pBuffer;
                        int numBytes;
                        int destw = pCodecCtx->width;
                        int desth = pCodecCtx->height;
                        if (destw > 252 || desth > 252) {
                            int sca = destw / 252 > desth / 252 ? destw / 252 : desth / 252;
                            destw = destw / sca;
                            desth = desth / sca;
                        }
                        numBytes = av_image_get_buffer_size(AV_PIX_FMT_BGR24, destw,
                                                            desth, 1);
                        pBuffer = (uint8_t *) av_malloc(numBytes * sizeof(uint8_t));
                        av_image_fill_arrays(pFrameRGB->data, pFrameRGB->linesize, pBuffer,
                                             AV_PIX_FMT_BGR24, destw,
                                             desth, 1);
                        struct SwsContext *convert_ctx;
                        convert_ctx = sws_getContext(
                                pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
                                destw, desth,
                                AV_PIX_FMT_BGR24, SWS_BICUBIC, NULL, NULL, NULL);
                        sws_scale(convert_ctx, (uint8_t const *const *) pFrame->data,
                                  pFrame->linesize, 0,
                                  pCodecCtx->height, pFrameRGB->data, pFrameRGB->linesize);
                        SaveFrameToBMP(szPicFile, pFrameRGB->data[0],
                                       destw, desth, 24);
                        sws_freeContext(convert_ctx);
                        LOGI("Frame Index: %5d. Type:%s saved", frame_cnt, pictype_str);
                    }
                        break;
                    case AV_PICTURE_TYPE_P:sprintf(pictype_str,"P");break;
                    case AV_PICTURE_TYPE_B:sprintf(pictype_str,"B");break;
                    default:sprintf(pictype_str,"Other");break;
                }
                LOGI("Frame Index: %5d. Type:%s",frame_cnt,pictype_str);
                frame_cnt++;
            }
        }
        av_free_packet(packet);
    }
    time_finish = clock();
    time_duration=(double)(time_finish - time_start);
    sprintf(info, "%s[Time      ]%fms\n",info,time_duration);
    sprintf(info, "%s[Count     ]%d\n",info,frame_cnt);
    sws_freeContext(img_convert_ctx);
    //fclose(fp_yuv);
    av_frame_free(&pFrameYUV);
    av_frame_free(&pFrame);
    avcodec_close(pCodecCtx);
    avformat_close_input(&pFormatCtx);
    return picoutid;
}
}





