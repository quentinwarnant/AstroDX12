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

struct ivec3
{
	int32_t x;
	int32_t y;
	int32_t z;
	constexpr ivec3() : x(0), y(0), z(0) {}
	constexpr ivec3(int32_t _x, int32_t _y, int32_t _z) : x(_x), y(_y), z(_z) {}
	constexpr ivec3 operator+(const ivec3& other) const
	{
		return ivec3(x + other.x, y + other.y, z + other.z);
	}
	constexpr ivec3 operator-(const ivec3& other) const
	{
		return ivec3(x - other.x, y - other.y, z - other.z);
	}
	constexpr ivec3 operator*(int32_t scalar) const
	{
		return ivec3(x * scalar, y * scalar, z * scalar);
	}
	constexpr ivec3& operator+=(const ivec3& other)
	{
		x += other.x;
		y += other.y;
		z += other.z;
		return *this;
	}
	constexpr ivec3& operator-=(const ivec3& other)
	{
		x -= other.x;
		y -= other.y;
		z -= other.z;
		return *this;
	}
	constexpr ivec3& operator*=(int32_t scalar)
	{
		x *= scalar;
		y *= scalar;
		z *= scalar;
		return *this;
	}
};