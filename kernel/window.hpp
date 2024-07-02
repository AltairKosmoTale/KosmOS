#pragma once

#include <vector>
#include <optional>
#include "graphics.hpp"
#include "frame_buffer.hpp"

// #@@range_begin(window)
class Window {
 public:
	// #@@range_begin(windowwriter)
	// WindowWriter: Window와 관련된 PixelWriter 제공
	class WindowWriter : public PixelWriter {
	 public:
		WindowWriter(Window& window) : window_{window} {} // 생성자에서 초기화
		// 지정된 위치에 지정된 색을 그림
		virtual void Write(Vector2D<int> pos, const PixelColor& c) override {
			window_.Write(pos, c);
		}
		virtual int Width() const override { return window_.Width(); }
		virtual int Height() const override { return window_.Height(); }
	 private:
		Window& window_;
	};
	// #@@range_end(windowwriter)
	// 지정된 픽셀 수의 평면 렌더링 영역 작성
	Window(int width, int height, PixelFormat shadow_format);
	~Window() = default;
	Window(const Window& rhs) = delete;
	Window& operator=(const Window& rhs) = delete;
	// 주어진 FrameBuffer에 이 윈도우의 표시 영역을 렌더링
	// @param dst: 렌더링 target, @param position: writer의 왼쪽 상단 기준 렌더링 위치
	void DrawTo(FrameBuffer& dst, Vector2D<int> position);
	// 표시 영역 투명색 설정
	void SetTransparentColor(std::optional<PixelColor> c);
	// 인스턴스와 연결된 WindowWriter 취득
	WindowWriter* Writer();
	// 지정한 위치의 픽셀 반환
	const PixelColor& At(Vector2D<int> pos) const;
	void Write(Vector2D<int> pos, PixelColor c);
	// 평명 렌더링 영역의 "가로" 픽셀 단위로 반환
	int Width() const;
	// 평명 렌더링 영역의 "세로" 픽셀 단위로 반환
	int Height() const;
	void Move(Vector2D<int> dst_pos, const Rectangle<int>& src);
	
 private:
	// #@@range_begin(fields)
	int width_, height_; // 가로, 세로
	std::vector<std::vector<PixelColor>> data_{}; // 픽셀 배열
	WindowWriter writer_{*this}; // 쓰기 기능 제공 // 멤버변수 초기화
	std::optional<PixelColor> transparent_color_{std::nullopt}; // 투명색
	// optional: "값을 갖지 않는" 상태를 명시적으로 표현하는 "Wrapper Class"
	FrameBuffer shadow_buffer_{};
	// #@@range_end(fields)
};
// #@@range_end(window)
