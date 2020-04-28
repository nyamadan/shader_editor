#include "mymkvwriter.hpp"

MyMkvWriter::MyMkvWriter(const std::string &file) : pos(0), len(0) {
    fp = fopen(file.c_str(), "wb");
}

MyMkvWriter::~MyMkvWriter() {}

mkvmuxer::int32 MyMkvWriter::Write(const void* buffer, mkvmuxer::uint32 length) {
    fwrite(buffer, sizeof(uint8_t), length, fp);
    len += length;
    pos += length;
    return 0;
}

mkvmuxer::int64 MyMkvWriter::Position() const { return pos; }

mkvmuxer::int32 MyMkvWriter::Position(mkvmuxer::int64 position) {
    pos = position;
    fseek(fp, pos, SEEK_SET);
    return 0;
}

bool MyMkvWriter::Seekable() const { return true; }

void MyMkvWriter::ElementStartNotify(mkvmuxer::uint64, mkvmuxer::int64) {}

void MyMkvWriter::Notify() {
    fclose(fp);
}
