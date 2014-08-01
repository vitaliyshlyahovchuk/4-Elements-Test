#include "stdafx.h"
#include "DetectBorder2D.h"
#include "GameField.h"
#include "MyApplication.h"

namespace DetectBorder2D
{
	struct compare_IPoint {
		bool operator() (const IPoint& p0, const IPoint& p1) const
		{		
			if(p0.x == p1.x)
			{
				return p0.y < p1.y;
			}
			return p0.x < p1.x;}
	};

	IPoint dirs[4] = { IPoint(1,1), IPoint(0, 1), IPoint(0,0), IPoint(1, 0)}; 
	std::map<Byte, std::vector<IPoint>> points_by_mask;

	void InitGame(rapidxml::xml_node<> *description_xml)
	{
		points_by_mask.clear();
		rapidxml::xml_node<>* borders_xml = description_xml->first_node("Curves")->first_node();
		Assert(borders_xml);
		rapidxml::xml_node<>* item_xml = borders_xml->first_node("item");
		while(item_xml)
		{
			IPoint offset(0,0);
			if(item_xml->first_node("offset"))
			{
				offset = IPoint(item_xml->first_node("offset"));
			}
			Byte mask = Xml::GetIntAttribute(item_xml, "mask");
			std::vector<IPoint> &points = points_by_mask[mask];
			rapidxml::xml_node<>* point_xml = item_xml->first_node("point");
			while(point_xml)
			{
				points.push_back(IPoint(point_xml) + offset);
				point_xml = point_xml->next_sibling("point");
			}
			item_xml = item_xml->next_sibling("item");
		}
	}
	typedef std::map<IPoint, int, compare_IPoint> map_IPoint;

	bool FindNear(map_IPoint &points, IPoint next_point, int &next_index, IPoint &result)
	{
		float dist_min = 100000.f;

		//Сначала берем те которые лежат в одной маске!
		map_IPoint::iterator i = points.begin(), finded_i = points.end();			
		for( ; i != points.end(); i++)
		{
			if(i->second == next_index)
			{
				float dist = FPoint(next_point).GetDistanceTo(i->first);
				if(dist < dist_min)
				{
					dist_min = dist;
					finded_i = i;
				}
			}
		}
		if(finded_i == points.end())
		{
			//С той же маской не осталось - можно искать в соседних
			i = points.begin();			
			for( ; i != points.end(); i++)
			{
				float dist = FPoint(next_point).GetDistanceTo(i->first);
				if(dist < dist_min && dist < 15.f)
				{
					dist_min = dist;
					finded_i = i;
				}
			}
		}
		if(finded_i != points.end())
		{
			result = finded_i->first;
			next_index = finded_i->second;
			return true;
		}	
		return false;
	}

	void FindStrip(map_IPoint &points, std::list<IPoint> &strip)
	{
		if(points.empty())
		{
			return;
		}
		strip.push_back(points.begin()->first);
		int current_index_start = points.begin()->second; //Индекс для поиска соседей явно подходящих друг другу (они лежат в одной маске!)
		int current_index0 = current_index_start;
		points.erase(points.begin());
		//Ищем ближайшие справа
		while(!points.empty())
		{
			IPoint result;
			if(FindNear(points, strip.back(), current_index0, result))
			{
				points.erase(points.find(result));	
				strip.push_back(result);
			}else{
				break;
			}
		}

		//И слева
		int current_index1 = current_index_start;
		while(!points.empty())
		{
			IPoint result;
			if(FindNear(points, strip.front(), current_index1, result))
			{
				points.erase(points.find(result));	
				strip.push_front(result);
			}else{
				break;
			}
		}
	}

	map_IPoint start_borders, finish_borders;

	int COUNTER_UINDEX = 0;

	void AddBorder(IPoint pos, Byte mask, bool is_first)
	{
		if(points_by_mask.find(mask) != points_by_mask.end())
		{
			std::vector<IPoint> &points = points_by_mask[mask];
			size_t count = points.size();
			COUNTER_UINDEX++;
			for(size_t i = 0; i < count; i++)
			{
				IPoint p = points[i] + pos; // + IPoint(GameSettings::SQUARE_SIDE/2, GameSettings::SQUARE_SIDE/2);
				if(is_first)
				{
					Assert(start_borders.find(p) == start_borders.end());
					start_borders[p] = COUNTER_UINDEX;
				}else{
					if(start_borders.find(p) != start_borders.end())
					{
						start_borders.erase(start_borders.find(p));
					}else{
						Assert(finish_borders.find(p) == finish_borders.end());
						finish_borders[p] = COUNTER_UINDEX;
					}
				}
			}
		}
	}

	void Draw()
	{
		Render::device.SetTexturing(false);

		Render::BeginColor(Color::BLACK);
		for(map_IPoint::iterator i = start_borders.begin(); i != start_borders.end(); i++)
		{
			IPoint p = i->first;
			Render::DrawFrame(IRect(p.x, p.y, 0,0).Inflated(2));	
		}
		Render::EndColor();

		Render::device.SetBlendMode(Render::ADD);
		Render::BeginColor(Color::WHITE);
		for(map_IPoint::iterator i = finish_borders.begin(); i != finish_borders.end(); i++)
		{
			IPoint p = i->first;
			Render::DrawFrame(IRect(p.x, p.y, 0,0).Inflated(2));	
		}
		Render::EndColor();
		Render::device.SetTexturing(true);
		Render::device.SetBlendMode(Render::ALPHA);
	}

}//namespace DetectBorder2D