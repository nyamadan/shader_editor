#include "recording.hpp"
#include "file_utils.hpp"
#include <math.h>

bool Recording::getIsRecording() { return isRecording; }

void Recording::start(const int32_t bufferWidth, const int32_t bufferHeight,
                      const std::string& fileName, const int32_t videoType,
                      const int32_t webmBitrate,
                      const unsigned long webmEncodeDeadline) {
    this->isRecording = true;

    this->videoType = videoType;
    this->bufferWidth = bufferWidth;
    this->bufferHeight = bufferHeight;

    const int32_t ySize = bufferWidth * bufferHeight;
    const int32_t uSize = ySize / 4;
    const int32_t vSize = uSize;
    yuvBuffer = std::make_unique<uint8_t[]>(ySize + uSize + vSize);

    switch (videoType) {
        case 0: {
            fp = fopen(fileName.c_str(), "wb");
            fprintf(fp,
                    "YUV4MPEG2 W%d H%d F30000:1001 Ip A0:0 C420 XYSCSS=420\n",
                    bufferWidth, bufferHeight);
        } break;
        case 1: {
            pWebmEncoder = std::make_unique<WebmEncoder>(
                fileName, 1001, 30000, bufferWidth, bufferHeight, webmBitrate);
        } break;
        case 2: {
            fp = fopen(fileName.c_str(), "wb");
            void* pEncoder = nullptr;
            h264encoder::CreateOpenH264Encoder(&pEncoder, &pMP4Muxer,
                                               &pMP4H264Writer, bufferWidth,
                                               bufferHeight, fp);
            delete pOpenH264Encoder;
            pOpenH264Encoder = pEncoder;
        } break;
    }
}

void Recording::update(bool isLastFrame, int64_t currentFrame,
                       GLuint writeBuffer, GLuint readBuffer) {
    glBindBuffer(GL_PIXEL_PACK_BUFFER, writeBuffer);
    glReadPixels(0, 0, bufferWidth, bufferHeight, GL_RGBA, GL_UNSIGNED_BYTE, 0);

    if (currentFrame > 0) {
        glBindBuffer(GL_PIXEL_PACK_BUFFER, readBuffer);

#ifdef __EMSCRIPTEN__
        {
            auto size = bufferWidth * bufferHeight * 4;
            auto ptr = std::make_unique<uint8_t[]>(size);
            EM_ASM({ Module.ctx.getBufferSubData(Module.ctx.PIXEL_PACK_BUFFER, 0, HEAPU8.subarray($0, $0 + $1)); }, ptr.get(), size);
            writeOneFrame(ptr.get(), currentFrame);
        }
#else
        {
            auto* ptr = static_cast<const uint8_t*>(glMapBufferRange(
                GL_PIXEL_PACK_BUFFER, 0, bufferWidth * bufferHeight * 4,
                GL_MAP_READ_BIT));

            if (ptr != nullptr) {
                writeOneFrame(ptr, currentFrame);
                glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
                ptr = nullptr;
            }
        }
#endif
    }

    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

    if (isLastFrame) {
        glBindBuffer(GL_PIXEL_PACK_BUFFER, writeBuffer);

#ifdef __EMSCRIPTEN__
        {
            auto size = bufferWidth * bufferHeight * 4;
            auto ptr = std::make_unique<uint8_t[]>(size);
            EM_ASM({ Module.ctx.getBufferSubData(Module.ctx.PIXEL_PACK_BUFFER, 0, HEAPU8.subarray($0, $0 + $1)); }, ptr.get(), size);
            writeOneFrame(ptr.get(), currentFrame);
        }
#else
        {
            auto* ptr = static_cast<const uint8_t*>(glMapBufferRange(
                GL_PIXEL_PACK_BUFFER, 0, bufferWidth * bufferHeight * 4,
                GL_MAP_READ_BIT));

            if (ptr != nullptr) {
                writeOneFrame(ptr, currentFrame);
                glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
                ptr = nullptr;
            }
        }
#endif

        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

        switch (videoType) {
            case 0: {
                fclose(fp);
                fp = nullptr;
#ifdef __EMSCRIPTEN__
                {
                    std::string buff;
                    readText("video.y4m", buff);
                    EM_ASM({
                        const a = document.createElement("a");
                        document.body.appendChild(a);
                        a.style = "display: none";
                        const data =HEAPU8.subarray($0, $0 + $1); 
                        const blob = new Blob([data], { type: "octet/stream" });
                        const url = window.URL.createObjectURL(blob);
                        a.href = url;
                        a.download = "video.y4m";
                        a.click();
                        document.body.removeChild(a);
                        window.URL.revokeObjectURL(url);
                    }, buff.c_str(), buff.size());
                }
#endif
            } break;
            case 1: {
                pWebmEncoder->finalize(webmEncodeDeadline);
#ifdef __EMSCRIPTEN__
                {
                    std::string buff;
                    readText("video.webm", buff);
                    EM_ASM({
                        const a = document.createElement("a");
                        document.body.appendChild(a);
                        a.style = "display: none";
                        const data =HEAPU8.subarray($0, $0 + $1); 
                        const blob = new Blob([data], { type: "octet/stream" });
                        const url = window.URL.createObjectURL(blob);
                        a.href = url;
                        a.download = "video.webm";
                        a.click();
                        document.body.removeChild(a);
                        window.URL.revokeObjectURL(url);
                    }, buff.c_str(), buff.size());
                }
#endif
            } break;
            case 2: {
                if (pOpenH264Encoder != nullptr) {
                    h264encoder::Finalize(pOpenH264Encoder, pMP4Muxer,
                                          pMP4H264Writer);
                    fclose(fp);

                    pOpenH264Encoder = nullptr;
                    pMP4Muxer = nullptr;
                    pMP4H264Writer = nullptr;
                    fp = nullptr;
#ifdef __EMSCRIPTEN__
                    {
                        std::string buff;
                        readText("video.mp4", buff);
                        EM_ASM({
                            const a = document.createElement("a");
                            document.body.appendChild(a);
                            a.style = "display: none";
                            const data =HEAPU8.subarray($0, $0 + $1); 
                            const blob = new Blob([data], { type: "octet/stream" });
                            const url = window.URL.createObjectURL(blob);
                            a.href = url;
                            a.download = "video.mp4";
                            a.click();
                            document.body.removeChild(a);
                            window.URL.revokeObjectURL(url);
                        }, buff.c_str(), buff.size());
                    }
#endif
                }
            } break;
        }

        isRecording = false;
    }
}

void Recording::cleanup() {
    if (pOpenH264Encoder != nullptr) {
        h264encoder::DestroyOpenH264Encoder(pOpenH264Encoder);
        pOpenH264Encoder = nullptr;
    }
}

void Recording::writeOneFrame(const uint8_t* rgbaBuffer, int64_t currentFrame) {
    const int32_t ySize = bufferWidth * bufferHeight;
    const int32_t uSize = ySize / 4;
    const int32_t vSize = uSize;

    const int32_t yStride = bufferWidth;
    const int32_t uStride = bufferWidth / 2;
    const int32_t vStride = uStride;

    uint8_t* yBuffer = yuvBuffer.get();
    uint8_t* uBuffer = yBuffer + ySize;
    uint8_t* vBuffer = uBuffer + uSize;

    switch (videoType) {
        case 0: {
            libyuv::ABGRToI420(rgbaBuffer, bufferWidth * 4, yBuffer, yStride,
                               uBuffer, uStride, vBuffer, vStride, bufferWidth,
                               -bufferHeight);

            fputs("FRAME\n", fp);
            fwrite(yuvBuffer.get(), sizeof(uint8_t),
                   (int64_t)ySize + uSize + vSize, fp);
        } break;
        case 1: {
            pWebmEncoder->addRGBAFrame(rgbaBuffer, webmEncodeDeadline);
        } break;
        case 2: {
            if (pOpenH264Encoder != nullptr) {
                libyuv::ABGRToI420(rgbaBuffer, bufferWidth * 4, yBuffer,
                                   yStride, uBuffer, uStride, vBuffer, vStride,
                                   bufferWidth, -bufferHeight);
                int64_t timestamp = roundl((currentFrame + 1) * 1001 / 30000);
                h264encoder::EncodeFrame(pOpenH264Encoder, pMP4H264Writer,
                                         yuvBuffer.get(), bufferWidth,
                                         bufferHeight, timestamp);
            }
        } break;
    }
}

Recording::~Recording() { cleanup(); }
