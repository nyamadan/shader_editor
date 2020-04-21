#pragma once

#include <memory>

#include "common.hpp"
#include "mp4muxer.h"
#include "webm_encoder.hpp"
#include "h264_encoder.hpp"
#include "recording.hpp"

class Recording {
   private:
    int32_t bufferWidth = 0;
    int32_t bufferHeight = 0;
    std::unique_ptr<uint8_t[]> yuvBuffer = nullptr;

    std::unique_ptr<WebmEncoder> pWebmEncoder = nullptr;
    unsigned long webmEncodeDeadline = 0;

    void* pOpenH264Encoder = nullptr;
    MP4E_mux_t* pMP4Muxer = nullptr;
    mp4_h26x_writer_t* pMP4H264Writer = nullptr;

    int32_t videoType = 0;
    bool isRecording = false;

    FILE* fp = nullptr;

   public:
    bool getIsRecording();

    void start(const int32_t bufferWidth, const int32_t bufferHeight,
               const std::string& fileName, const int32_t videoType,
               const int32_t webmBitrate,
               const unsigned long webmEncodeDeadline);

    void update(bool isLastFrame, int64_t currentFrame, GLuint writeBuffer, GLuint readBuffer);

    void cleanup();

    ~Recording();

   private:
    void writeOneFrame(const uint8_t* rgbaBuffer, int64_t currentFrame);
};
