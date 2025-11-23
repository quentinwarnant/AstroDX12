#pragma once

#include <cstdint>

struct ivec2
{
	int32_t x;
	int32_t y;
	constexpr ivec2() : x(0), y(0) {}
	constexpr ivec2(int32_t _x, int32_t _y) : x(_x), y(_y) {}

	constexpr ivec2 operator+(const ivec2& other) const
	{
		return ivec2(x + other.x, y + other.y);
	}
	constexpr ivec2 operator-(const ivec2& other) const
	{
		return ivec2(x - other.x, y - other.y);
	}
	constexpr ivec2 operator*(int32_t scalar) const
	{
		return ivec2(x * scalar, y * scalar);
	}
	constexpr ivec2& operator+=(const ivec2& other)
	{
		x += other.x;
		y += other.y;
		return *this;
	}
	constexpr ivec2& operator-=(const ivec2& other)
	{
		x -= other.x;
		y -= other.y;
		return *this;
	}
	constexpr ivec2& operator*=(int32_t scalar)
	{
		x *= scalar;
		y *= scalar;
		return *this;
	}
};