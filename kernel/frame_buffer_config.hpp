#pragma once

#include <stdint.h>

enum PixelFormat {
  kPixelRGBResv8BitPerColor,
  kPixelBGRResv8BitPerColor,
  // PixelBitMask: make 렌더링 complex
  // PixelBltOnly: make 렌더링 complex
};

struct FrameBufferConfig {
  uint8_t* frame_buffer; // 프레임 버퍼 영역
  uint32_t pixels_per_scan_line; // 프레임 버퍼의 여백을 포함한 가로 방향 픽셀 수
  uint32_t horizontal_resolution; // 수평 해상도
  uint32_t vertical_resolution; // 수직 해상도
  enum PixelFormat pixel_format; // 4가지
};