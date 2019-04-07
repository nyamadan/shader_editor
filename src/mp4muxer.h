#pragma once

#include <sys/types.h>
#include <stdint.h>

#ifdef MINIMP4_IMPLEMENTATION
#include <minimp4.h>
#else
struct MP4E_mux_t;
struct mp4_h26x_writer_t;
#endif

typedef void (*write_callback)(int64_t, const void* buffer, size_t size,
                               void* token);

#ifdef __cplusplus
extern "C" {
#endif
void MP4MuxOpen(FILE* fp, write_callback callback, int32_t width,
                int32_t height, MP4E_mux_t** mux, mp4_h26x_writer_t** mp4wr);

void MP4MuxClose(MP4E_mux_t* mux, mp4_h26x_writer_t* mp4wr);

void MP4MuxWrite(mp4_h26x_writer_t* mp4wr, const uint8_t* nal, int32_t length);

#ifdef __cplusplus
}
#endif
