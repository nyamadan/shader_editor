#include "common.hpp"
#include "mp4muxer.h"

#include <windows.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>

namespace {
static HMODULE _openh264 = nullptr;
static int32_t (*_WelsCreateSVCEncoder)(void**) = nullptr;
static void (*_WelsDestroySVCEncoder)(void*) = nullptr;

int WriteCallback(int64_t offset, const void* buffer, size_t size,
                  void* token) {
    FILE* f = (FILE*)token;
    fseek(f, static_cast<long>(offset), SEEK_SET);
    return fwrite(buffer, 1, size, f) != size;
}

}  // namespace

namespace h264encoder {
#if defined(_MSC_VER) || defined(__MINGW32__)

bool LoadEncoderLibrary() {
    const auto LibraryName = "openh264-2.0.0-win64.dll";

    _openh264 = LoadLibraryA(LibraryName);

    if (_openh264 == nullptr) {
        return false;
    }

    _WelsCreateSVCEncoder =
        (int32_t(*)(void**))GetProcAddress(_openh264, "WelsCreateSVCEncoder");

    _WelsDestroySVCEncoder =
        (void (*)(void*))GetProcAddress(_openh264, "WelsDestroySVCEncoder");

    return true;
}

void Finalize(void* pOpenH264Encoder, MP4E_mux_t* pMP4Muxer,
              mp4_h26x_writer_t* pMP4H264Writer) {
    int32_t rv = ((int32_t(*)(void* pEncoder))(*(void***)pOpenH264Encoder)[3])(
        pOpenH264Encoder);
    assert(rv == 0);

    _WelsDestroySVCEncoder(pOpenH264Encoder);

    MP4MuxClose(pMP4Muxer, pMP4H264Writer);
}

void EncodeFrame(void* pEncoder, mp4_h26x_writer_t* pWriter, uint8_t* data,
                 int32_t iPicWidth, int32_t iPicHeight, int64_t timestamp) {
    int32_t rv = 0;

    const int32_t ySize = iPicWidth * iPicHeight;
    const int32_t uSize = ySize / 4;
    const int32_t vSize = uSize;

    uint8_t sFbi[5144];
    memset(sFbi, 0, 5144);

    uint8_t pic[72];
    memset(pic, 0, 72);
    *(int32_t*)(pic + 0) = 23;               // iColorFormat
    *((int32_t*)(pic + 4) + 0) = iPicWidth;  // iStride[0]
    *((int32_t*)(pic + 4) + 1) = *((int32_t*)(pic + 4) + 2) =
        iPicWidth >> 1;                  // iStride[1],iStride[1]
    *(int32_t*)(pic + 56) = iPicWidth;   // Width
    *(int32_t*)(pic + 60) = iPicHeight;  // Height
    *(int64_t*)(pic + 64) = timestamp;   // uiTimeStamp

    *((void**)(pic + 24) + 0) = data;                  // pData[0]
    *((void**)(pic + 24) + 1) = data + ySize;          // pData[1]
    *((void**)(pic + 24) + 2) = data + ySize + uSize;  // pData[2]
    *((void**)(pic + 24) + 3) = nullptr;               // pData[3]

    rv = ((int32_t(*)(void* pEncoder, void* pic, void* sFbi))(
        *(void***)pEncoder)[4])(pEncoder, &pic, sFbi);
    assert(rv == 0);

    int32_t iLayer = 0;
    int32_t iFrameSize = 0;
    int32_t iLayerNum = *(int32_t*)&sFbi[0];
    while (iLayer < iLayerNum) {
        uint8_t* pLayerBsInfo = &sFbi[40 * iLayer + 8];
        if (pLayerBsInfo != NULL) {
            int32_t iLayerSize = 0;
            int32_t iNalCount = *(int32_t*)(pLayerBsInfo + 16);
            int32_t* pNalLengthInByte = *(int32_t**)(pLayerBsInfo + 24);
            uint8_t* pBsBuf = *(uint8_t**)(pLayerBsInfo + 32);
            int iNalIdx = iNalCount - 1;
            do {
                iLayerSize += pNalLengthInByte[iNalIdx];
                --iNalIdx;
            } while (iNalIdx >= 0);
            iFrameSize += iLayerSize;

            MP4MuxWrite(pWriter, pBsBuf, iLayerSize);
        }
        ++iLayer;
    }
}

bool CreateOpenH264Encoder(void** ppEncoder, MP4E_mux_t** ppMP4Muxer,
                           mp4_h26x_writer_t** ppMP4H264Writer,
                           int32_t iPicWidth, int32_t iPicHeight, FILE* fp) {
    int rv = 0;
    rv = _WelsCreateSVCEncoder(ppEncoder);
    assert(rv == 0);

    void* pEncoder = *ppEncoder;

#ifndef NDEBUG
    int32_t logLevel = 16;
    ((int32_t(*)(void*, int32_t, int32_t*))(*(void***)pEncoder)[7])(
        pEncoder, 25, &logLevel);
    assert(rv == 0);
#endif

    uint8_t param[916];
    memset(param, 0, 916);
    rv = ((int32_t(*)(void* pEncoder, void* pParam))(*(void***)pEncoder)[2])(
        pEncoder, param);
    assert(rv == 0);

    *(int32_t*)(param + 0) = 1;           // iUsageType
    *(int32_t*)(param + 4) = iPicWidth;   // Width
    *(int32_t*)(param + 8) = iPicHeight;  // Height
    *(int32_t*)(param + 12) = 0;          // iTargetBitrate
    *(int32_t*)(param + 16) = -1;         // iRCMode
    // *(int32_t*)(param + 20) = 60.0f;      // fMaxFrameRate
    *(uint8_t*)(param + 860) = 0;  // bEnableFrameSkip
    // *(uint8_t*)(param + 868) = 0;         // iMaxQp
    // *(uint8_t*)(param + 872) = 0;         // iMinQp
    for (uint64_t i = 0; i < 4; i++) {
        *(uint8_t*)(param + 60 + i * 200) = 23;  // iDlayerQp
    }

    rv = ((int32_t(*)(void* pEncoder, void* pParam))(*(void***)pEncoder)[1])(
        pEncoder, param);
    assert(rv == 0);

    int32_t videoFormat = 23;
    ((int32_t(*)(void*, int32_t, int32_t*))(*(void***)pEncoder)[7])(
        pEncoder, 0, &videoFormat);
    assert(rv == 0);

    MP4MuxOpen(fp, WriteCallback, iPicWidth, iPicHeight, ppMP4Muxer,
               ppMP4H264Writer);

    return true;
}

void DestroyOpenH264Encoder(void* pOpenH264Encoder) {
    int32_t rv = ((int32_t(*)(void* pEncoder))(*(void***)pOpenH264Encoder)[3])(
        pOpenH264Encoder);
    assert(rv == 0);

    _WelsDestroySVCEncoder(pOpenH264Encoder);
}

#else
#endif
}  // namespace h264encoder
