#include "stdafx.h"
#include "Match3Background.h"
#include "Game.h"

namespace Match3Background
{

	BackgroundCloud::BackgroundCloud(Render::Texture *tex, FPoint start_pos, FPoint dir, float time_scale)
		: _texture(tex)
	{
		_centerTexture = IPoint(_texture->Width()/2, _texture->Height()/2);
		_pos = start_pos - dir*FPoint(_centerTexture).Length();
		_dir = dir.Normalized();
		_time_scale = time_scale;
		_max_distance = FPoint(GameSettings::VIEW_RECT.Width()/2.f,GameSettings::VIEW_RECT.Height()/2.f).Length() + FPoint(_centerTexture).Length();
		_visible = false;
	}

	void BackgroundCloud::Draw(FPoint delta)
	{
		_pos -= delta;
		_texture->Draw(_pos - _centerTexture);
	}

	bool BackgroundCloud::Update(float dt)
	{
		_pos += _dir*dt*_time_scale;
		bool next_visible = FPoint(_dir*_max_distance).Length() > FPoint(_pos - GameSettings::FIELD_SCREEN_CENTER).Length();
		if(!next_visible && _visible)
		{
			return true;
		}
		_visible = next_visible;
		return false;
	}


	void FonDrawer::Init(rapidxml::xml_node<>* xml_level)
	{
		MyAssert(xml_level);
		rapidxml::xml_node<>* elemGradient = xml_level->first_node("Gradient");

		if (elemGradient)
		{
			_gradientTop = Color(elemGradient->first_node("Color1"));
			_gradientBottom = Color(elemGradient->first_node("Color0"));
		}
		else
		{
			_gradientTop = Color(255, 255, 255, 255);
			_gradientBottom = Color(255, 255, 255, 255);
		}
		ryushkaDepthColor = Color(xml_level->first_node("RyushkaDepthColor"));

		rapidxml::xml_node<> *xml_tiles_info = xml_level->first_node("tiles");
		if(xml_tiles_info)
		{
			rapidxml::xml_node<> *xml_tile_info = xml_tiles_info->first_node("tile");
			while(xml_tile_info)
			{
				_textures.push_back(Core::resourceManager.Get<Render::Texture>(Xml::GetStringAttribute(xml_tile_info, "texture")));
				xml_tile_info = xml_tile_info->next_sibling("tile");
			}
		}
		rapidxml::xml_node<> *xml_cloud = xml_level->first_node("cloud");
		while(xml_cloud)
		{
			Render::Texture* tex = 	Core::resourceManager.Get<Render::Texture>(Xml::GetStringAttribute(xml_cloud, "texture"));
			_cloudsTex.push_back(tex);
			xml_cloud = xml_cloud->next_sibling("cloud");
		}
	}

	void FonDrawer::LoadLevel()
	{
		for(size_t i = 0; i < _textures.size(); i++)
		{
			_textures[i]->BeginUse();
		}
		for(auto i : _cloudsTex)
		{
			i->BeginUse();
		}
		if(!_cloudsTex.empty())
		{
			int count = math::random(5, 10);
			for(int i = 0; i < count; i++)
			{
				_cloudsForDraw.push_back(new BackgroundCloud(_cloudsTex[math::random(_cloudsTex.size()-1)], 
					FPoint(math::random(0.f, GameSettings::VIEW_RECT.Width() + 300.f), math::random(0.f, float(GameSettings::VIEW_RECT.Height()))), 
					FPoint(1.f, 0.f), 
					math::random(10.f, 40.f)));
			}
		}
		_prevFieldPos = FPoint(-1.f, -1.f);
	}

	void FonDrawer::ClearLevel()
	{
		for(size_t i = 0; i < _textures.size(); i++)
		{
			_textures[i]->EndUse();
		}
		for(auto i : _cloudsTex)
		{
			i->EndUse();
		}
		for(auto i : _cloudsForDraw)
		{
			delete i;
		}
		_cloudsForDraw.clear();
	}

	void FonDrawer::Draw(const float alpha)
	{
		float u0, u1, v0, v1;
		float w = _textures.front()->getBitmapRect().width + 0.f;
		float h = _textures.front()->getBitmapRect().height + 0.f;
		Game::Background_GetUVs(1.f, 1.f, w, u0, u1, v0, v1);
		FPoint start_point; //(0.f, Render::device.Width(), 0.f, Render::device.Height());
		float low_u = math::floor(u0);
		start_point.x = w*(low_u - u0);

		float left_v = math::floor(v0);
		start_point.y = h*(left_v - v0);

		size_t x_count = (u1 - low_u) + 1.f;
		size_t y_count = (v1 - left_v) + 1.f;
		FPoint center = FPoint(w*low_u, h*left_v);
		for(size_t x = 0; x < x_count; x++)
		{
			for(size_t y = 0; y < y_count; y++)
			{
				FPoint offset = FPoint(w*x, h*y);
				FPoint p = start_point + offset;
				int tile_num = int((offset + center).GetDistanceToOrigin() + (offset + center).GetDistanceTo(FPoint(100.f, 100.f))) % _textures.size();
				_textures[tile_num]->Draw(p);
			}
		}
	}

	void FonDrawer::DrawClouds(float alpha, FPoint field_pos)
	{
		FPoint delta = (field_pos - _prevFieldPos);
		if(_prevFieldPos.x < 0)
		{
			delta = FPoint(0.f, 0.f);
		}
		_prevFieldPos = field_pos;
		Render::BeginAlphaMul(alpha);
		for(auto i : _cloudsForDraw)
		{
			i->Draw(delta*math::lerp(0.f, 1.f, i->_time_scale/60.f));
		}
		Render::EndAlphaMul();
	}

	bool SortBySpeed(BackgroundCloud* c1, BackgroundCloud* c2)
	{
		return c1->_time_scale < c2->_time_scale;
	}

	void FonDrawer::Update(float dt)
	{
		int erased_cloud = 0;
		for(std::list<BackgroundCloud*>::iterator i = _cloudsForDraw.begin(); i != _cloudsForDraw.end();)
		{
			if((*i)->Update(dt))
			{
				delete *i;
				i = _cloudsForDraw.erase(i);
				erased_cloud++;
			}else{
				i++;
			}
		}
		if(erased_cloud > 0)
		{
			for(int i = 0; i < erased_cloud; i++)
			{
				_cloudsForDraw.push_back(new BackgroundCloud(_cloudsTex[math::random(_cloudsTex.size()-1)], 
					FPoint(-100.f, math::random(0.f, float(GameSettings::VIEW_RECT.Height()))), 
					FPoint(1.f, 0.f), 
					math::random(10.f, 40.f)));
			}
			_cloudsForDraw.sort(SortBySpeed);
		}
	}

}//namespace Match3Background