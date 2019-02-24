#include "webm_encoder.hpp"

using namespace mkvmuxer;

namespace {
int vpx_img_plane_width(const vpx_image_t *img, int plane) {
    if ((plane == 1 || plane == 2) && img->x_chroma_shift > 0)
        return (img->d_w + 1) >> img->x_chroma_shift;
    else
        return img->d_w;
}

int vpx_img_plane_height(const vpx_image_t *img, int plane) {
    if ((plane == 1 || plane == 2) && img->y_chroma_shift > 0)
        return (img->d_h + 1) >> img->y_chroma_shift;
    else
        return img->d_h;
}

void clear_image(vpx_image_t *img) {
    for (int plane = 0; plane < 4; plane++) {
        auto *row = img->planes[plane];
        if (!row) {
            continue;
        }
        auto plane_width = vpx_img_plane_width(img, plane);
        auto plane_height = vpx_img_plane_height(img, plane);
        uint8_t value = plane == 3 ? 1 : 0;
        for (int y = 0; y < plane_height; y++) {
            memset(row, value, plane_width);
            row += img->stride[plane];
        }
    }
}
}  // namespace

WebmEncoder::WebmEncoder(const std::string &file, int timebase_num,
                         int timebase_den, unsigned int width,
                         unsigned int height, unsigned int bitrate_kbps) {
    if (!InitCodec(timebase_num, timebase_den, width, height, bitrate_kbps)) {
        throw last_error;
    }
    if (!InitMkvWriter(file)) {
        throw last_error;
    }
    if (!InitImageBuffer()) {
        throw last_error;
    }
}

WebmEncoder::~WebmEncoder() {
    vpx_img_free(img);
    delete main_segment;
    delete mkv_writer;
}

bool WebmEncoder::addRGBAFrame(const uint8_t *rgba, unsigned long deadline) {
    RGBAtoVPXImage(rgba);
    if (!EncodeFrame(img, deadline)) {
        return false;
    }
    return true;
}

bool WebmEncoder::finalize(unsigned long deadline) {
    if (!EncodeFrame(NULL, deadline)) {
        last_error = "Could not encode flush frame";
        return false;
    }

    if (!main_segment->Finalize()) {
        last_error = "Could not finalize mkv";
        return false;
    }
    mkv_writer->Notify();
    return true;
}

std::string WebmEncoder::lastError() { return std::string(last_error); }

bool WebmEncoder::EncodeFrame(vpx_image_t *img, unsigned long deadline) {
    vpx_codec_iter_t iter = NULL;
    const vpx_codec_cx_pkt_t *pkt;
    vpx_codec_err_t err;

    err = vpx_codec_encode(&ctx, img, frame_cnt, 1, 0, deadline);
    frame_cnt++;
    if (err != VPX_CODEC_OK) {
        last_error = std::string(vpx_codec_err_to_string(err));
        return false;
    }
    while ((pkt = vpx_codec_get_cx_data(&ctx, &iter)) != NULL) {
        if (pkt->kind != VPX_CODEC_CX_FRAME_PKT) {
            continue;
        }
        auto frame_size = pkt->data.frame.sz;
        auto is_keyframe = (pkt->data.frame.flags & VPX_FRAME_IS_KEY) != 0;
        auto timebase = 1000000000ULL * cfg.g_timebase.num / cfg.g_timebase.den;
        auto timestamp = pkt->data.frame.pts * timebase;

        Frame frame;
        if (!frame.Init((uint8_t *)pkt->data.frame.buf, pkt->data.frame.sz)) {
            last_error = "Could not initialize frame container";
            return false;
        }
        frame.set_track_number(1);
        frame.set_timestamp(timestamp);
        frame.set_is_key(is_keyframe);
        frame.set_duration(timebase);
        if (!main_segment->AddGenericFrame(&frame)) {
            last_error = "Could not add frame";
            return false;
        }
    }
    return true;
}

bool WebmEncoder::InitCodec(int timebase_num, int timebase_den,
                            unsigned int width, unsigned int height,
                            unsigned int bitrate) {
    vpx_codec_err_t err;
    err = vpx_codec_enc_config_default(iface, &cfg, 0);
    if (err != VPX_CODEC_OK) {
        last_error = std::string(vpx_codec_err_to_string(err));
        return false;
    }
    cfg.g_timebase.num = timebase_num;
    cfg.g_timebase.den = timebase_den;
    cfg.g_w = width;
    cfg.g_h = height;
    cfg.rc_target_bitrate = bitrate;

    err = vpx_codec_enc_init(&ctx, iface, &cfg, 0);
    if (err != VPX_CODEC_OK) {
        last_error = std::string(vpx_codec_err_to_string(err));
        return false;
    }
    return true;
}

bool WebmEncoder::InitMkvWriter(const std::string &file) {
    mkv_writer = new MyMkvWriter(file);

    main_segment = new Segment();
    if (!main_segment->Init(mkv_writer)) {
        last_error = "Could not initialize main segment";
        return false;
    }
    if (main_segment->AddVideoTrack(cfg.g_w, cfg.g_h, 1) == 0) {
        last_error = "Could not add video track";
        return false;
    }
    main_segment->set_mode(Segment::Mode::kFile);
    return true;
}

bool WebmEncoder::InitImageBuffer() {
    img = vpx_img_alloc(NULL, VPX_IMG_FMT_I420, cfg.g_w, cfg.g_h, 0);
    if (img == NULL) {
        last_error = "Could not allocate vpx image buffer";
        return false;
    }
    return true;
}

bool WebmEncoder::RGBAtoVPXImage(const uint8_t *rgba) {
    clear_image(img);
    if (libyuv::ABGRToI420(rgba, cfg.g_w * 4, img->planes[VPX_PLANE_Y],
                           img->stride[VPX_PLANE_Y], img->planes[VPX_PLANE_U],
                           img->stride[VPX_PLANE_U], img->planes[VPX_PLANE_V],
                           img->stride[VPX_PLANE_V], cfg.g_w, -cfg.g_h) != 0) {
        last_error = "Could not convert to I420";
        return false;
    }
    return true;
}
