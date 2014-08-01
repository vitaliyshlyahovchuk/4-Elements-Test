#pragma once
#include <vector>

namespace Render
{
	class Texture;
}

namespace Match3Background
{
	class BackgroundCloud
	{
	public:
		FPoint _pos;
		FPoint _dir;
		float _time_scale;
		Render::Texture *_texture;
		float _max_distance;
		IPoint _centerTexture;
		bool _visible;
	public:
		BackgroundCloud(Render::Texture *tex, FPoint start_pos, FPoint dir, float time_scale);
		void Draw(FPoint delta);
		bool Update(float dt);
	};

	class FonDrawer
	{
		Color _gradientTop;		// ÷вет градиента (верхний)
		Color _gradientBottom;	// ÷вет градиента (нижний)

		std::vector<Render::Texture*> _textures;
		std::vector<Render::Texture*> _cloudsTex;
		std::list<BackgroundCloud*> _cloudsForDraw;
		FPoint _prevFieldPos;
	public:
		Color ryushkaDepthColor;		// цвет глубины, в которую можно увеcти рюшку
	public:
		void Init(rapidxml::xml_node<>* xml_level);
		void LoadLevel();
		void ClearLevel();
		void Draw(const float alpha);
		void DrawClouds(float alpha, FPoint field_pos);
		void Update(float dt);
	};
}//namespace Match3Background