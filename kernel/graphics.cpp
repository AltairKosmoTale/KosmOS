// #@@range_begin(pixel_writer_impl)
#include "graphics.hpp"

void RGBResv8BitPerColorPixelWriter::Write(Vector2D<int> pos, const PixelColor& c) {
	auto p = PixelAt(pos);
	p[0] = c.r;
	p[1] = c.g;
	p[2] = c.b;
}
// #@@range_end(pixel_writer_impl)

void BGRResv8BitPerColorPixelWriter::Write(Vector2D<int> pos, const PixelColor& c) {
	auto p = PixelAt(pos);
	p[0] = c.b;
	p[1] = c.g;
	p[2] = c.r;
}

// #@@range_begin(draw_rectangle)
void DrawRectangle(PixelWriter& writer, const Vector2D<int>& pos, const Vector2D<int>& size, const PixelColor& c) {
	for (int dx = 0; dx < size.x; ++dx) {
		writer.Write(pos + Vector2D<int>{dx, 0}, c);
		writer.Write(pos + Vector2D<int>{dx, size.y - 1}, c);
	}
	for (int dy = 1; dy < size.y - 1; ++dy) {
		writer.Write(pos + Vector2D<int>{0, dy}, c);
		writer.Write(pos + Vector2D<int>{size.x - 1, dy}, c);
	}
}
// #@@range_end(draw_rectangle)

// #@@range_begin(fill_rectangle)
void FillRectangle(PixelWriter& writer, const Vector2D<int>& pos, const Vector2D<int>& size, const PixelColor& c) {
	for (int dy = 0; dy < size.y; ++dy) {
		for (int dx = 0; dx < size.x; ++dx) {
			writer.Write(pos + Vector2D<int>{dx, dy}, c);
		}
	}
}
// #@@range_end(fill_rectangle)
void DrawDesktop(PixelWriter& writer) {
	const auto width = writer.Width();
	const auto height = writer.Height();
	FillRectangle(writer,
		{0, 0},
		{width, height - 30},
		kDesktopBGColor);
	FillRectangle(writer,
		{0, height - 30},
		{width, 30},
		{20, 20, 20});
}