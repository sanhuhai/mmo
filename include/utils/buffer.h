#pragma once

#include <vector>
#include <cstdint>
#include <cstring>
#include <string>
#include <stdexcept>

namespace mmo {

class Buffer {
public:
    Buffer() : read_pos_(0), write_pos_(0) {
    }

    explicit Buffer(size_t size) 
        : data_(size)
        , read_pos_(0)
        , write_pos_(0) {
    }

    Buffer(const uint8_t* data, size_t size)
        : data_(data, data + size)
        , read_pos_(0)
        , write_pos_(size) {
    }

    void WriteUInt8(uint8_t value) {
        EnsureCapacity(1);
        data_[write_pos_++] = value;
    }

    void WriteUInt16(uint16_t value) {
        EnsureCapacity(2);
        data_[write_pos_++] = (value >> 8) & 0xFF;
        data_[write_pos_++] = value & 0xFF;
    }

    void WriteUInt32(uint32_t value) {
        EnsureCapacity(4);
        data_[write_pos_++] = (value >> 24) & 0xFF;
        data_[write_pos_++] = (value >> 16) & 0xFF;
        data_[write_pos_++] = (value >> 8) & 0xFF;
        data_[write_pos_++] = value & 0xFF;
    }

    void WriteUInt64(uint64_t value) {
        EnsureCapacity(8);
        data_[write_pos_++] = (value >> 56) & 0xFF;
        data_[write_pos_++] = (value >> 48) & 0xFF;
        data_[write_pos_++] = (value >> 40) & 0xFF;
        data_[write_pos_++] = (value >> 32) & 0xFF;
        data_[write_pos_++] = (value >> 24) & 0xFF;
        data_[write_pos_++] = (value >> 16) & 0xFF;
        data_[write_pos_++] = (value >> 8) & 0xFF;
        data_[write_pos_++] = value & 0xFF;
    }

    void WriteInt8(int8_t value) {
        WriteUInt8(static_cast<uint8_t>(value));
    }

    void WriteInt16(int16_t value) {
        WriteUInt16(static_cast<uint16_t>(value));
    }

    void WriteInt32(int32_t value) {
        WriteUInt32(static_cast<uint32_t>(value));
    }

    void WriteInt64(int64_t value) {
        WriteUInt64(static_cast<uint64_t>(value));
    }

    void WriteFloat(float value) {
        uint32_t int_value;
        std::memcpy(&int_value, &value, sizeof(float));
        WriteUInt32(int_value);
    }

    void WriteDouble(double value) {
        uint64_t int_value;
        std::memcpy(&int_value, &value, sizeof(double));
        WriteUInt64(int_value);
    }

    void WriteString(const std::string& value) {
        WriteUInt32(static_cast<uint32_t>(value.length()));
        WriteData(reinterpret_cast<const uint8_t*>(value.data()), value.length());
    }

    void WriteData(const uint8_t* data, size_t size) {
        EnsureCapacity(size);
        std::memcpy(data_.data() + write_pos_, data, size);
        write_pos_ += size;
    }

    uint8_t ReadUInt8() {
        CheckReadSize(1);
        return data_[read_pos_++];
    }

    uint16_t ReadUInt16() {
        CheckReadSize(2);
        uint16_t value = (static_cast<uint16_t>(data_[read_pos_]) << 8) |
                         static_cast<uint16_t>(data_[read_pos_ + 1]);
        read_pos_ += 2;
        return value;
    }

    uint32_t ReadUInt32() {
        CheckReadSize(4);
        uint32_t value = (static_cast<uint32_t>(data_[read_pos_]) << 24) |
                         (static_cast<uint32_t>(data_[read_pos_ + 1]) << 16) |
                         (static_cast<uint32_t>(data_[read_pos_ + 2]) << 8) |
                         static_cast<uint32_t>(data_[read_pos_ + 3]);
        read_pos_ += 4;
        return value;
    }

    uint64_t ReadUInt64() {
        CheckReadSize(8);
        uint64_t value = (static_cast<uint64_t>(data_[read_pos_]) << 56) |
                         (static_cast<uint64_t>(data_[read_pos_ + 1]) << 48) |
                         (static_cast<uint64_t>(data_[read_pos_ + 2]) << 40) |
                         (static_cast<uint64_t>(data_[read_pos_ + 3]) << 32) |
                         (static_cast<uint64_t>(data_[read_pos_ + 4]) << 24) |
                         (static_cast<uint64_t>(data_[read_pos_ + 5]) << 16) |
                         (static_cast<uint64_t>(data_[read_pos_ + 6]) << 8) |
                         static_cast<uint64_t>(data_[read_pos_ + 7]);
        read_pos_ += 8;
        return value;
    }

    int8_t ReadInt8() {
        return static_cast<int8_t>(ReadUInt8());
    }

    int16_t ReadInt16() {
        return static_cast<int16_t>(ReadUInt16());
    }

    int32_t ReadInt32() {
        return static_cast<int32_t>(ReadUInt32());
    }

    int64_t ReadInt64() {
        return static_cast<int64_t>(ReadUInt64());
    }

    float ReadFloat() {
        uint32_t int_value = ReadUInt32();
        float value;
        std::memcpy(&value, &int_value, sizeof(float));
        return value;
    }

    double ReadDouble() {
        uint64_t int_value = ReadUInt64();
        double value;
        std::memcpy(&value, &int_value, sizeof(double));
        return value;
    }

    std::string ReadString() {
        uint32_t length = ReadUInt32();
        CheckReadSize(length);
        std::string value(reinterpret_cast<const char*>(data_.data() + read_pos_), length);
        read_pos_ += length;
        return value;
    }

    std::vector<uint8_t> ReadData(size_t size) {
        CheckReadSize(size);
        std::vector<uint8_t> data(data_.begin() + read_pos_, data_.begin() + read_pos_ + size);
        read_pos_ += size;
        return data;
    }

    const uint8_t* Data() const { return data_.data(); }
    uint8_t* Data() { return data_.data(); }

    size_t Size() const { return write_pos_; }
    size_t Capacity() const { return data_.size(); }
    size_t ReadableBytes() const { return write_pos_ - read_pos_; }
    size_t WritableBytes() const { return data_.size() - write_pos_; }

    void Clear() {
        read_pos_ = 0;
        write_pos_ = 0;
        data_.clear();
    }

    void Reset() {
        read_pos_ = 0;
        write_pos_ = 0;
    }

    void ShrinkToFit() {
        data_.resize(write_pos_);
        data_.shrink_to_fit();
    }

    void Compact() {
        if (read_pos_ > 0) {
            std::memmove(data_.data(), data_.data() + read_pos_, ReadableBytes());
            write_pos_ -= read_pos_;
            read_pos_ = 0;
        }
    }

private:
    void EnsureCapacity(size_t size) {
        if (write_pos_ + size > data_.size()) {
            data_.resize(std::max(data_.size() * 2, write_pos_ + size));
        }
    }

    void CheckReadSize(size_t size) {
        if (read_pos_ + size > write_pos_) {
            throw std::out_of_range("Buffer read out of range");
        }
    }

    std::vector<uint8_t> data_;
    size_t read_pos_;
    size_t write_pos_;
};

}
