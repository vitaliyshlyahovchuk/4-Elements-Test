#ifndef PLACE_2D_H
#define PLACE_2D_H

#include "Game.h"

namespace Place2D
{
	struct ScopeRect
	{
		FRect frect;
		FRect rect;

	};


	struct ScopePiece
	{
		/*Можно задать два варианта отрисовки границы. Например для прокопов льда, плит, шоколада и частокола в
		углах  0101 и 1010 необходимо рисовать с большим радиусом! Для этого номер варимнта такого угла должен вычисляться явно.*/
		std::vector<ScopeRect> or_rects_v0, or_rects_v1; //Выбирается любой рандомом. 
		std::vector<ScopeRect> and_rects; //Обязательны(ый) для отрисовки если имеется

		//Опционально: Z-смещение для неадекватного уголка.
		float zDepthOffset;

		void Load(rapidxml::xml_node<> *xml_element, Render::Texture *texture, float fscale);
	};

	struct ScopePieceForDraw
	{
		//ScopePieceForDraw(ScopePiece &orig, const IPoint &pos);
		ScopePieceForDraw(FRect frect_, FRect rect_, float z);
		FRect frect;
		FRect rect;
		float z;
	};

	typedef std::map<BYTE, ScopePiece> ScopePieceMap;
	struct BorderInfo
	{
		ScopePieceMap map;
		Render::Texture *texture;

		void Load(rapidxml::xml_node<> *xml_info);
	};
	extern std::map<std::string, BorderInfo> borders;

	struct CellInfo
	{
		BYTE exist_count, visible_count;
		bool coners[4][4];
		bool vis[4][4];
		bool is_exist;
		bool visible;
		bool wall_exist; //Информацию о реальной земле нужно хранить для любых элементов для учета прокопов. Там внешние радиусы больше.
		bool wall_coner[4][4];
		FPoint pos;

		bool is_up[4][4];
		bool with_up;

		CellInfo();
	};
	const int MAP_HEIGHT = 30, MAP_WIDTH = 30;
	extern std::vector<ScopePieceForDraw> wall_borders, ice_borders, stone_borders, wood_borders, wall_energy_borders, misc_borders;

	extern BorderInfo *mask_map;
	extern VertexBufferIndexed *buffer;

	void SetParams(float r_in, float r_out, float r_out2, float d_inflate = 0);

	 //Рисовать инвертированные углы в клетках светлыми (первый квадрат)
	void SetChessOnConers(const bool value);
	void Init(rapidxml::xml_node<> *xml_root);
	void Release();

	void DrawPlace(const Color &color, const IRect &rectDraw, float zDepth, bool need_draw_squares = true, bool need_draw_invert_corners = true);
	void Calculate(bool is_wall(Game::Square *sq), bool is_visible(Game::Square *sq));
	void AddAddress(const Game::FieldAddress &address);
	void DrawBorders(std::vector<Place2D::ScopePieceForDraw> &vec, const std::string &border_id, Color color = Color::WHITE, bool clear = true);

	void Clear();
	void CalculateWithoutVisability();
	void DrawPlaceWithoutVisability(const Color &color, const IRect &rectDraw, float zDepth, const std::string &name_border);
	void BindBorders(std::vector<ScopePieceForDraw> *vec);
	void ClearDrawBuffer();
	void SetViewRect(IRect rect);

}
#endif//PLACE_2D_H