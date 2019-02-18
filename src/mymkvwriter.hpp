#pragma once

#include <inttypes.h>

#include <string>
#include <stdio.h>

#include "mkvmuxer.hpp"

class MyMkvWriter : public mkvmuxer::IMkvWriter {
   public:
    explicit MyMkvWriter(const std::string& file);
    virtual ~MyMkvWriter();

    virtual int64_t Position() const;
    virtual int32_t Position(int64_t position);
    virtual bool Seekable() const;
    virtual int32_t Write(const void* buffer, uint32_t length);
    virtual void ElementStartNotify(uint64_t element_id, int64_t position);

    void Notify();

   private:
    FILE* fp;
    uint64_t pos;
    uint64_t len;
};
