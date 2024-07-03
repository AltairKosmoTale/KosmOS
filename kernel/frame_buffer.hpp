#pragma once

#include <vector>
#include <memory>

#include "frame_buffer_config.hpp"
#include "graphics.hpp"
#include "error.hpp"

class FrameBuffer {
 public:
	Error Initialize(const FrameBufferConfig& config);
	Error Copy(Vector2D<int> dst_pos, const FrameBuffer& src, const Rectangle<int>& src_area);
	void Move(Vector2D<int> dst_pos, const Rectangle<int>& src);

	FrameBufferWriter& Writer() { return *writer_; }
	const FrameBufferConfig& Config() const { return config_; }

 private:
	FrameBufferConfig config_{}; // 가로, 세로, 픽셀 데이터 형식
	std::vector<uint8_t> buffer_{}; // 렌더링 영역
	std::unique_ptr<FrameBufferWriter> writer_{}; // unique: 소유권 on FrameBuffer
};

