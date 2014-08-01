#pragma once
#include "Spline.h"

class SplineBezier
{
	size_t  size, currentItem;
	struct Item
	{
		math::Vector3 p1, p2, p3;
		float lenght_begin;
		float lenght_end;
		math::Vector3 A,B,C;
		//float lenght_max;
		//float items[11];
		//float step;
		//int currentItem;
		Item(math::Vector3 pp1, math::Vector3 pp2, math::Vector3 pp3, float *l0, float *lenghtXY);
		math::Vector3 getFrame(float time_bool);
		//float getRealTime(float k);
		//math::Vector3 getFrameTimed(float k);
	};
	std::vector<Item> items;
	float lenght_max, lenght_max_xy;
public:
	std::vector<math::Vector3> keys;
	bool error;
public:
	SplineBezier();
	void Clear();
	void addKey(math::Vector3 newKey);
	bool CalculateGradient(bool cycled = false);
	math::Vector3 getGlobalFrame(float time_bool);        
	//math::Vector3 getFrameTimed(float time_bool);

	float GetLenght();
	float GetLenghtXY();
};

/*
	Пересчитать сплайн
	Распределить ключевые точки равномерно по длинне
*/
void DistributeByLong(SplinePath<FPoint> &path);
