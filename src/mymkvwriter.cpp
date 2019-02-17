#include "mymkvwriter.hpp"

MyMkvWriter::MyMkvWriter(const std::string &file) : pos(0), len(0) {
    fp = fopen(file.c_str(), "wb");
}

MyMkvWriter::~MyMkvWriter() {}

int32_t MyMkvWriter::Write(const void* buffer, uint32_t length) {
    fwrite(buffer, sizeof(uint8_t), length, fp);
    len += length;
    pos += length;
    return 0;
}

int64_t MyMkvWriter::Position() const { return pos; }

int32_t MyMkvWriter::Position(int64_t position) {
    pos = position;
    fseek(fp, pos, SEEK_SET);
    return 0;
}

bool MyMkvWriter::Seekable() const { return true; }

void MyMkvWriter::ElementStartNotify(uint64_t, int64_t) {}

void MyMkvWriter::Notify() {
    fclose(fp);
}
