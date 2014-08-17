#ifndef _WALL_DRAWER_H_
#define _WALL_DRAWER_H_
#include "Game.h"
#include "Place2D.h"

extern bool is_true(Game::Square *sq);
//extern bool is_visible(Game::Square *sq);

class WallDrawer
{
private:
	Render::Texture *_textureStone, *_textureIce, *_textureWood, *_textureJava, *_textureCyclops;
	Render::Texture *_textureFieldChess;
	bool _exist_ice, _exist_wood, _exist_cyclops;
	bool _exist_sand, _exist_grow_wall,  _exist__wall_fabric, _exist_energy_wall, _exist_indestructible, _exist_fill_order, _exist_permanent;
	bool _exist_wall2, _exist_wall3;
	void ProcessPlace(bool is_wall(Game::Square *sq), bool is_visible(Game::Square *sq), Render::Texture *texture, const Color &color, const std::string &name_border, IRect rectDraw, float zDepth, const bool need_draw_squares = true, const bool need_draw_invert_coners = true);
public:
	WallDrawer(){};
	~WallDrawer();
	void Init(rapidxml::xml_node<> *xml_root);
	void LoadLevel();
	void DrawFieldBase();

	void DrawGround();
	void DrawGroundTransparent();
	void DrawGroundBorders();

	void DrawSand();

	void DrawStone();
	void DrawStoneBorders();

	void DrawIce();
	void DrawIceBorders();

	void DrawWood();
	void DrawWoodBorders();

	void DrawCyclops();
	void DrawCyclopsBorders();
	void DrawCyclopsTransparent();

	void Init();
	//На каждом уровне может быть свой набор элементов. После загрузки инициализируем переменные _exist...
	//В каждом апдейте проверять эти переменные - вариант. Но есть сомнения. Для цветной земли проверяется в апдейте
	void InitAfterLoadLevel();
	void InitEditor();
};

namespace Gadgets
{
	void WallDrawerInit();

	extern WallDrawer wallDrawer;

} //Gadgets

#endif //_WALL_DRAWER_H_