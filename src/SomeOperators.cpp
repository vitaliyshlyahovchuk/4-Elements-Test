#include "stdafx.h"
#include "SomeOperators.h"

IRect operator *(int N, IRect r) {
	return IRect(N * r.x,
		N * r.y,
		N * r.width,
		N * r.height);
}

IRect operator *(IRect r, int N) {
	return operator * (N, r);
}

IPoint operator *(int N, IPoint p) {
	return IPoint(N * p.x, N * p.y);
}

FRect operator / (IRect smallRect, IRect bigRect) {
	float u1 = float(smallRect.x - bigRect.x) / bigRect.width;
	float u2 = float(smallRect.x + smallRect.width - bigRect.x) / bigRect.width;
	float v1 = float(smallRect.y - bigRect.y) / bigRect.height;
	float v2 = float(smallRect.y + smallRect.height - bigRect.y) / bigRect.height;
	return FRect(u1, u2, v1, v2);
}


FRect operator / (FRect smallRect, IRect bigRect) {
	float u1 = smallRect.xStart/ bigRect.width;
	float u2 = smallRect.xEnd / bigRect.width;
	float v1 = smallRect.yStart / bigRect.height;
	float v2 = smallRect.yEnd / bigRect.height;
	return FRect(u1, u2, v1, v2);
}

FPoint operator / (FPoint fpoint, IRect bigRect)
{
	return FPoint((fpoint.x - bigRect.x)/ bigRect.width, (fpoint.y - bigRect.y) / bigRect.height);
}

float RoundTo(const float value, int precision)
{
	static const float TEN (10.0f);
	float factor = ::powf(TEN, (float) precision);
	int int_value = math::round((float) value / factor);
	return ((float) int_value * factor);
}

FPoint& Normalize(FPoint& p)
{
	float length = math::sqrt(p.x * p.x + p.y * p.y);
	
	if (length > 0.0f) 
	{
		p.x /= length;
		p.y /= length;
	}

	return (p);
}

math::Vector3& Normalize(math::Vector3& v)
{
	float length = math::sqrt(v.x * v.x + v.y * v.y + v.z + v.z);

	if (length > 0.0f)
	{
		v.x /= length;
		v.y /= length;
		v.z /= length;
	}

	return (v);
}