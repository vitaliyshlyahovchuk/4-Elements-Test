#include "stdafx.h"
#include "SplineBezier.h"

SplineBezier::Item::Item(math::Vector3 pp1, math::Vector3 pp2, math::Vector3 pp3, float *l0, float *lenghtXY)
{
	p1 = pp1;
	p2 = pp2;
	p3 = pp3;
	A = p1;
	B = 2.f * (p2 - p1);
	C = (p1 - 2.f * p2 + p3);

	lenght_begin = (*l0);

	float lenght_max = 0.f;
	//(*lenghtXY) = 0.f;
	math::Vector3 pb = p1, pe;
	math::Vector3 peXY = pe, pbXY = pb;
	for(int i=1; i<= 10; i++)
	{
		pe = getFrame(i/10.f);
		lenght_max += !(pe - pb);

		peXY = pe; pbXY = pb;
		peXY.z =0; pbXY.z =0;
		(*lenghtXY) += !(peXY - pbXY);
		pb = pe;
	}
	lenght_end =   lenght_begin + lenght_max;
	(*l0) = lenght_end;
	//pb = p1;
	//items [0] = 0;
	//step = 1.f/10.f;
	//float current_lenght = 0;
	//for(int i=1; i< 11; i++)
	//{
	//	pe = getFrame(i/10.f);
	//	current_lenght +=!(pe - pb);
	//	items [i] = current_lenght/lenght_max;
	//	pb = pe;
	//}
	//currentItem = 0;
}

math::Vector3 SplineBezier::Item::getFrame(float time_bool)
{
	return A + B*time_bool + C*time_bool*time_bool;
	//math::Vector3 v1 = math::lerp(p1, p2, sqrt(time_bool));
	//math::Vector3 v2 = math::lerp(p2, p3, time_bool*time_bool);
	//`return math::lerp(v1, v2, time_bool);
}

//float SplineBezier::Item::getRealTime(float k)
//{
//	float itemBegin, itemEnd;
//	itemBegin = items[currentItem];
//	while(k < itemBegin){
//		itemBegin = items[--currentItem];
//	}
//	itemEnd = items[currentItem + 1];
//	while( itemEnd < k){
//		itemEnd = items[++currentItem + 1];
//	}
//	itemBegin = items[currentItem];
//	return ((k - itemBegin)/(itemEnd - itemBegin) + currentItem)/10.f;
//}
//
//math::Vector3 SplineBezier::Item::getFrameTimed(float k)
//{
//	float time_bool = getRealTime(k);
//	return A + B*time_bool + C*time_bool*time_bool;
//}

SplineBezier::SplineBezier()
{
	Clear();
}

void SplineBezier::Clear()
{
	keys.clear();
	items.clear();
	size = 0;
	error = true;
	currentItem = 0;
	lenght_max = 0;
	lenght_max_xy = 0;
}

void SplineBezier::addKey(math::Vector3 newKey)
{
	keys.push_back(newKey);
	size ++;
}
bool SplineBezier::CalculateGradient(bool cycled)
{
	error = false;
	while(size < 3){ //Заглушка
		keys.push_back(math::Vector3(0.f, size*(-1.f),0));
		size++;
	}
	lenght_max_xy = 0.f;
	for(size_t i=1; i< size-1; i++)
	{
		math::Vector3 key0, key1, key2;
		if(i==1)  {
			key0 = keys[0];
		}else{
			key0 = (keys[i-1] + keys[i])/2.f;
		}
		key1 = keys[i];
		if(i== (size - 2)){
			key2 = keys[size-1];
		}else{
			key2 = (keys[i] + keys[i+1])/2.f;
		}
		items.push_back(Item(key0, key1, key2, &lenght_max, &lenght_max_xy));
	}
	/*
	for(size_t i=1; i< size-1; i++)
	{
	items.push_back(Item((keys[i-1] + keys[i])/2.f, keys[i], (keys[i] + keys[i+1])/2.f, &lenght_max));
	}                
	*/
	return true;
}
math::Vector3 SplineBezier::getGlobalFrame(float time_bool)
{
	if(error){
		Assert(false);
		return math::Vector3(0.f, 0.f, 0.f);
	}
	time_bool = math::clamp(0.f, 1.f, time_bool);

	float lenght_norm = lenght_max*time_bool;
	float lenght_b = items[currentItem].lenght_begin;
	while(lenght_norm < lenght_b)
	{
		currentItem--;
		lenght_b = items[currentItem].lenght_begin;
	}
	float lenght_e = items[currentItem].lenght_end;
	while( lenght_norm > lenght_e)
	{
		currentItem++;
		lenght_e = items[currentItem].lenght_end;
	}
	lenght_b = items[currentItem].lenght_begin;
	return items[currentItem].getFrame((lenght_norm - lenght_b)/(lenght_e - lenght_b));
}

//math::Vector3 SplineBezier::getFrameTimed(float time_bool)
//{
//	if(error){
//		return math::Vector3(0.f, 0.f, 0.f);
//	}
//	float lenght_norm = lenght_max*time_bool;
//	float lenght_b = items[currentItem].lenght_begin;
//	while(lenght_norm < lenght_b)
//	{
//		currentItem--;
//		lenght_b = items[currentItem].lenght_begin;
//	}
//	float lenght_e = items[currentItem].lenght_end;
//	while( lenght_norm > lenght_e)
//	{
//		currentItem++;
//		lenght_e = items[currentItem].lenght_end;
//	}
//	lenght_b = items[currentItem].lenght_begin;
//	return items[currentItem].getFrameTimed((lenght_norm - lenght_b)/(lenght_e - lenght_b));
//}

float SplineBezier::GetLenght()
{
	return lenght_max;
}

float SplineBezier::GetLenghtXY()
{
	return lenght_max_xy;
}


void DistributeByLong(SplinePath<FPoint> &path)
{
	//1. Сохраняем предыдущий сплайн
	SplinePath<FPoint> prev_path = path;
	path.Clear();

	//2. Считаем общую длинну (используем мелкий шаг для большей точности)
	float fullLenght = 0.f;
	FPoint prev = prev_path.getGlobalFrame(0.f);
	int count = prev_path.keys.size()*10.f;
	for(int i = 1; i <= count; ++i)
	{
		float t = float(i)/count;
		FPoint p = prev_path.getGlobalFrame(t);
		fullLenght += prev.GetDistanceTo(p);
		prev = p;
	}
	//3. Проходя мелким шагом заносим ключевую точку после каждого преодоления крупного(нужного нам) шага
	prev = FPoint(prev_path.getGlobalFrame(0.f).x, prev_path.getGlobalFrame(0.f).y);
	path.addKey(prev);
	float current_L = 0.f;
	float dist = 0.f;
	float dist_limmit = 0.f;
	const float step = fullLenght/prev_path.keys.size();
	const float d_L = step/10.f; //Мелкий шаг на порядок меньше большого
	while(dist < fullLenght - step)
	{
		current_L += d_L;
		FPoint p = prev_path.getGlobalFrame(current_L/fullLenght);
		dist += p.GetDistanceTo(prev);
		dist_limmit += p.GetDistanceTo(prev);
		if(dist_limmit >= step)
		{
			path.addKey(p);
			dist_limmit = 0.f;
		}
		prev = p;
	}
	path.addKey(prev_path.getGlobalFrame(1.f));
	path.CalculateGradient();
}