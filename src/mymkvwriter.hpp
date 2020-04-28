#pragma once

#include <inttypes.h>

#include <string>
#include <stdio.h>

#include "mkvmuxer.hpp"

class MyMkvWriter : public mkvmuxer::IMkvWriter {
   public:
    explicit MyMkvWriter(const std::string& file);
    virtual ~MyMkvWriter();

    virtual mkvmuxer::int64 Position() const;
    virtual mkvmuxer::int32 Position(mkvmuxer::int64 position);
    virtual bool Seekable() const;
    virtual mkvmuxer::int32 Write(const void* buffer, mkvmuxer::uint32 length);
    virtual void ElementStartNotify(mkvmuxer::uint64 element_id, mkvmuxer::int64 position);

    void Notify();

   private:
    FILE* fp;
    mkvmuxer::uint64 pos;
    mkvmuxer::uint64 len;
};
