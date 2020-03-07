#define MINIMP4_IMPLEMENTATION
#include "mp4muxer.h"

void MP4MuxOpen(FILE* fp, write_callback callback, int32_t width,
                int32_t height, MP4E_mux_t** mux, mp4_h26x_writer_t** mp4wr) {
    *mux = MP4E_open(0, 0, fp, callback);
    *mp4wr = malloc(sizeof(mp4_h26x_writer_t));
    mp4_h26x_write_init(*mp4wr, *mux, width, height, 0);
}

void MP4MuxClose(MP4E_mux_t* mux, mp4_h26x_writer_t* mp4wr) {
    MP4E_close(mux);
    free(mp4wr);
}

void MP4MuxWrite(mp4_h26x_writer_t* mp4wr, const uint8_t* nal, int32_t length) {
    mp4_h26x_write_nal(mp4wr, nal, length, 90000 * 1001 / 30000);
}
