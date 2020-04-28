#include "buffers.hpp"

#include <algorithm>
#include <assert.h>

GLint Buffers::getWidth() { return bufferWidth; }
GLint Buffers::getHeight() { return bufferHeight; }

GLuint Buffers::getFrameBuffer(BufferType type) {
    switch (type) {
        case READ:
            return frameBuffers[readBufferIndex];
        case WRITE:
            return frameBuffers[writeBufferIndex];
    }

    assert(0);

    return 0;
}

GLuint Buffers::getDepthBuffer(BufferType type) {
    switch (type) {
        case READ:
            return depthBuffers[readBufferIndex];
        case WRITE:
            return depthBuffers[writeBufferIndex];
    }

    assert(0);

    return 0;
}

GLuint Buffers::getBackBuffer(BufferType type) {
    switch (type) {
        case READ:
            return backBuffers[readBufferIndex];
        case WRITE:
            return backBuffers[writeBufferIndex];
    }

    assert(0);

    return 0;
}

GLuint Buffers::getPixelBuffer(BufferType type) {
    switch (type) {
        case READ:
            return pixelBuffers[readBufferIndex];
        case WRITE:
            return pixelBuffers[writeBufferIndex];
    }

    assert(0);

    return 0;
}

void Buffers::initialize(GLint width, GLint height) {
    // Framebuffers
    glGenFramebuffers(2, frameBuffers);
    glGenRenderbuffers(2, depthBuffers);
    glGenTextures(2, backBuffers);
    glGenBuffers(2, pixelBuffers);

    updateFrameBuffersSize(width, height);

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Buffers::updateFrameBuffersSize(GLint width, GLint height) {
    bufferWidth = width;
    bufferHeight = height;

    for (int32_t i = 0; i < 2; i++) {
        glBindFramebuffer(GL_FRAMEBUFFER, frameBuffers[i]);
        glBindRenderbuffer(GL_RENDERBUFFER, depthBuffers[i]);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16,
                              bufferWidth, bufferHeight);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                  GL_RENDERBUFFER, depthBuffers[i]);

        glBindTexture(GL_TEXTURE_2D, backBuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bufferWidth, bufferHeight, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, 0);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, backBuffers[i], 0);

        glBindTexture(GL_TEXTURE_2D, 0);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glBindBuffer(GL_PIXEL_PACK_BUFFER, pixelBuffers[i]);
        glBufferData(GL_PIXEL_PACK_BUFFER, bufferWidth * bufferHeight * 4L, 0,
                     GL_STREAM_DRAW);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    }
}

void Buffers::swap() { std::swap(writeBufferIndex, readBufferIndex); }
