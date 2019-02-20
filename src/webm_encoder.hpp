#pragma once

#include <string>
#include <cstdlib>

// libwebm
#include <mkvmuxer.hpp>
// libvpx
#include <vpxenc.h>
#include <vpx/vp8cx.h>
// libyuv
#include <libyuv.h>

#include "mymkvwriter.hpp"

class WebmEncoder {
   public:
    WebmEncoder(const std::string &file, int timebase_num, int timebase_den,
                unsigned int width, unsigned int height, unsigned int bitrate);
    ~WebmEncoder();
    bool addRGBAFrame(const uint8_t *rgba, unsigned long deadline);
    bool finalize(unsigned long deadline);
    std::string lastError();

   private:
    bool InitCodec(int timebase_num, int timebase_den, unsigned int width,
                   unsigned int height, unsigned int bitrate);
    bool InitMkvWriter(const std::string &file);
    bool InitImageBuffer();

    bool RGBAtoVPXImage(const uint8_t *data);
    bool EncodeFrame(vpx_image_t *img, unsigned long deadline);

    vpx_codec_ctx_t ctx;
    unsigned int frame_cnt = 0;
    vpx_codec_enc_cfg_t cfg;
    vpx_codec_iface_t *iface = vpx_codec_vp8_cx();
    vpx_image_t *img;
    std::string last_error;
    MyMkvWriter *mkv_writer;
    mkvmuxer::Segment *main_segment;
};
