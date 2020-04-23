#pragma once

#include "common.hpp"

typedef enum {
    READ,
    WRITE,
} BufferType;

class Buffers {
   private:
    int32_t writeBufferIndex = 0;
    int32_t readBufferIndex = 1;
    GLuint frameBuffers[2] = {0};
    GLuint depthBuffers[2] = {0};
    GLuint backBuffers[2] = {0};
    GLuint pixelBuffers[2] = {0};

    GLint bufferWidth;
    GLint bufferHeight;

   public:
    GLint getWidth();
    GLint getHeight();

    GLuint getFrameBuffer(BufferType type);

    GLuint getDepthBuffer(BufferType type);

    GLuint getBackBuffer(BufferType type);

    GLuint getPixelBuffer(BufferType type);

    void initialize(GLint width, GLint height);

    void updateFrameBuffersSize(GLint width, GLint height);

    void swap();
};
