#pragma once

namespace h264encoder {
bool LoadEncoderLibrary();
void UnloadEncoderLibrary();

void EncodeFrame(void* pEncoder, mp4_h26x_writer_t* pWriter, uint8_t* data,
                 int32_t iPicWidth, int32_t iPicHeight, int64_t timestamp);

void Finalize(void* pOpenH264Encoder, MP4E_mux_t* pMP4Muxer,
              mp4_h26x_writer_t* pMP4H264Writer);

bool CreateOpenH264Encoder(void** ppEncoder, MP4E_mux_t** pMP4Muxer,
                           mp4_h26x_writer_t** pMP4H264Writer,
                           int32_t iPicWidth, int32_t iPicHeight, FILE* fp);

void DestroyOpenH264Encoder(void* pOpenH264Encoder);

}  // namespace h264encoder
