#include "stdafx.h"
#include "WallDrawer.h"
#include "EditorUtils.h"
#include "FieldStyles.h"
#include "SomeOperators.h"
#include "Energy.h"
#include "GameOrder.h"

namespace Gadgets
{
	WallDrawer wallDrawer;
} //namespace Gadgets

Render::Texture *growingWallTex = NULL;
Render::Texture *growingWallFabricTex = NULL;


Render::Texture *backgroundLow = NULL;
Render::Texture *backgroundMiddle = NULL;
Render::Texture *backgroundTop = NULL;
Render::Texture *backgroundEmpty = NULL;

void WallDrawer::Init(rapidxml::xml_node<> *xml_root)
{
	_textureStone = Core::resourceManager.Get<Render::Texture>("StoneRepeat");
	_textureStone->setFilteringType(Render::Texture::BILINEAR);
	_textureStone->setAddressType(Render::Texture::REPEAT);
	 
	_textureFieldChess = Core::resourceManager.Get<Render::Texture>("FieldChess");
	_textureFieldChess->setFilteringType(Render::Texture::BILINEAR);
	_textureFieldChess->setAddressType(Render::Texture::REPEAT);

	_textureIce = Core::resourceManager.Get<Render::Texture>("IceRepeat");
	_textureIce->setFilteringType(Render::Texture::NEAREST);
	_textureIce->setAddressType(Render::Texture::REPEAT);
	
	_textureWood = Core::resourceManager.Get<Render::Texture>("WoodRepeate");
	_textureWood->setFilteringType(Render::Texture::BILINEAR);
	_textureWood->setAddressType(Render::Texture::REPEAT);

	_textureJava = Core::resourceManager.Get<Render::Texture>("JavaRepeate");
	_textureJava->setFilteringType(Render::Texture::BILINEAR);
	_textureJava->setAddressType(Render::Texture::REPEAT);

	_textureCyclops = Core::resourceManager.Get<Render::Texture>("CyclopsRepeat");
	_textureCyclops->setFilteringType(Render::Texture::BILINEAR);
	_textureCyclops->setAddressType(Render::Texture::REPEAT);
	

	Place2D::Init(xml_root);	
	_exist_sand = _exist_grow_wall = _exist__wall_fabric = _exist_energy_wall = _exist_indestructible = _exist_fill_order = _exist_permanent = false;
	_exist_wall2 = _exist_wall3 = false;
	_exist_wood = _exist_ice = _exist_cyclops = false;
}

WallDrawer::~WallDrawer()
{
	
};

void WallDrawer::ProcessPlace(bool is_wall(Game::Square *sq), bool is_visible(Game::Square *sq), Render::Texture *texture, const Color &color, const std::string &name_border, IRect rectDraw, float zDepth, const bool need_draw_squares, const bool need_draw_invert_coners)
{
	//1. Вычисляем
	Place2D::Calculate(is_wall, is_visible);
	//2. Рисуем
	if(texture){
		texture->Bind();
	}else{
		Render::device.SetTexturing(false);
	}

	if(!name_border.empty())
	{
		Place2D::mask_map = &Place2D::borders[name_border];
	}else{
		Place2D::mask_map = NULL;
	}

	Place2D::DrawPlace(color, rectDraw, zDepth, need_draw_squares, need_draw_invert_coners);

	if(texture)
	{
	}else{
		Render::device.SetTexturing(true);
	}
}


//Вспомогательные функции для индикации клеток отрисовки
bool up_element(Game::Square *sq)
{
	return sq->GetWood() > 0 || sq->IsIce() || sq->IsStone();
}

bool is_wall_0(Game::Square *sq)
{
	return sq->GetWall() == 0 && sq->GetSandTime() == 0;
}

bool is_wall_1(Game::Square *sq)
{
	return sq->GetWall() >= 1 && sq->GetSandTime() == 0;
}

bool is_wall_2(Game::Square *sq)
{
	return sq->GetWall() >= 2 && !up_element(sq);
}

bool is_wall_3(Game::Square *sq)
{
	return sq->GetWall() >= 3;
}

bool is_visible_editor(Game::Square *sq)
{
	return Game::isVisible(sq);
}

bool is_visible_game(Game::Square *sq)
{
	return Game::isVisible(sq) && sq->IsStayFly();
}

bool ( * is_visible )(Game::Square *sq) = &is_visible_game;

bool is_wall_border(Game::Square *sq)
{
	return (sq->GetWall() >= 1 && sq->GetSandTime() == 0) || !is_visible(sq);
}

bool is_sand(Game::Square *sq)
{
	return sq->GetSandTime() > 0;//return sq->IsSand();
}

int current_wall_color_for_check = 0;

bool is_color_wall(Game::Square *sq)
{
	if(sq->GetWall() <= 0)
	{
		return false;
	}
	return sq->GetWallColor() == current_wall_color_for_check;
}

bool is_true(Game::Square *sq)//Заглушка - не надо обрабатывать края- считаем что они есть везде
{
	return true;
}

bool is_wood(Game::Square *sq)
{
	return sq->GetWood() > 0 && is_visible(sq);
}

bool is_stone(Game::Square *sq)
{
	return sq->IsStone() && is_visible(sq);
}

bool is_ice(Game::Square *sq)
{
	return sq->IsIce() && is_visible(sq) && !sq->IsCyclops();
}

bool is_cyclops(Game::Square *sq)
{
	return sq->IsCyclops() && is_visible(sq);
}

bool is_indestructible(Game::Square *sq)
{
	return sq->IsIndestructible() && is_visible(sq);
}

bool is_permanent(Game::Square *sq)
{
	return sq->IsPermanent();
}

bool is_ordered_to_fill(Game::Square *sq)
{
	return Game::Orders::CellIsOrdered(sq->address);
}

bool is_grow_wall(Game::Square *sq)
{
	return sq->IsGrowingWall() && !sq->IsGrowingWallFabric() && is_visible(sq);
}

bool is_grow_wall_fabric(Game::Square *sq)
{
	return sq->IsGrowingWall() && sq->IsGrowingWallFabric() && is_visible(sq);
}

bool is_energy_wall(Game::Square *sq)
{
	return sq->IsEnergyWall() && is_visible(sq);
}

bool is_base_visible(Game::Square *sq)
{
	return is_visible(sq) && (sq->GetWall() == 0);// && !Energy::field.FullOfEnergy(sq->address);
}

void WallDrawer::DrawFieldBase()
{
	Place2D::SetParams(0.4f, 0.4f, 0.4f);
	Place2D::BindBorders(NULL);
	Place2D::Clear();
	Color bgColor =  Color("#8a642d")*FieldStyles::current->bgEmptyColor;
	bgColor.alpha = 255;

	IRect rectDraw(0, 0, GameSettings::SQUARE_SIDE*2, GameSettings::SQUARE_SIDE*2);

	Place2D::mask_map = NULL;
	Place2D::Calculate(is_visible, is_base_visible);

	_textureFieldChess->Bind();
	Place2D::DrawPlace(bgColor, rectDraw, ZBuf::GROUND);
}

void WallDrawer::DrawGround()
{
	Place2D::SetParams(0.41f, 0.35f, 0.33f); //Углы земли приводятся в соответствие с графикой границ земли. Делаем их чуть более острыми

	Render::device.SetBlend(false);

	IRect drawRect(0, 0, GameSettings::SQUARE_SIDE * 2, GameSettings::SQUARE_SIDE * 2);

	const Color wall_color("#9ac62d");
	Render::Texture *textureWall = _textureFieldChess;
	Place2D::SetChessOnConers(false);

	//Render::Texture *textureWall = NULL;
	ProcessPlace(is_wall_1, is_visible, textureWall, wall_color*FieldStyles::current->bgBottomColor, "", drawRect, ZBuf::GROUND_1);
	if(_exist_wall2)
	{
		ProcessPlace(is_wall_2, is_visible, textureWall, wall_color*FieldStyles::current->bgMiddleColor, "", drawRect, ZBuf::GROUND_2);
	}
	if(_exist_wall3)
	{
		ProcessPlace(is_wall_3, is_visible, textureWall, wall_color*FieldStyles::current->bgTopColor, "", drawRect, ZBuf::GROUND_3);
	}

	Place2D::SetChessOnConers(true);

	Place2D::wall_borders.clear();
	Place2D::BindBorders(&Place2D::wall_borders);

	//Все границы обычной собираем тут: (Учитывается граница поля)
	ProcessPlace(is_wall_border, is_true, NULL, FieldStyles::current->bgBottomColor, "wall", GameSettings::CELL_RECT, ZBuf::GROUND_3, false);

	if(_exist_grow_wall)
	{
		ProcessPlace(is_grow_wall, is_true, growingWallTex, Color::WHITE, "", growingWallTex->getBitmapRect(), ZBuf::GROUND_3);
	}

	if(_exist__wall_fabric)
	{
		ProcessPlace(is_grow_wall_fabric, is_true, growingWallFabricTex, Color::WHITE, "", growingWallFabricTex->getBitmapRect(), ZBuf::GROUND_3);
	}

	//Сепециальная земля - убираетя матчем с энергией
	Place2D::SetParams(0.33f, 0.35f, 0.35f);
	Place2D::wall_energy_borders.clear();
	if(_exist_energy_wall)
	{
		Place2D::BindBorders(&Place2D::wall_energy_borders);
		ProcessPlace(is_energy_wall, is_true, _textureJava, Color(200,200,255, 120), "java", IRect(0, 0, 128, 128), ZBuf::GROUND_3);
	}

	Render::device.SetBlend(true);
}

void WallDrawer::DrawGroundTransparent()
{
	//_agressiveChoolateExist = false;
	//Подсвеченная земля(цветная). Рисуется поверх обычной земли
	std::map<int, Color> _walls_colors;

	for (int x = Game::visibleRect.LeftBottom().x; x <= Game::visibleRect.RightTop().x; x++)
	{
		for (int y = Game::visibleRect.LeftBottom().y; y <= Game::visibleRect.RightTop().y; y++)
		{
			Game::Square *s = GameSettings::gamefield[x + 1][y+1];
			int color = s->GetWallColor();
			if(color > 0)
			{
				Color col = GameSettings::chip_settings[color].color_wall;
				col.alpha = 120;
				_walls_colors.insert( std::make_pair(color, col) );
			}
			//if(!_agressiveChoolateExist && is_agressive_chockolate(s))
			//{
			//	_agressiveChoolateExist = true;
			//}
		}
	}

	Place2D::SetParams(0.5f, 0.5f, 0.5f);
	for(std::map<int, Color>::iterator it = _walls_colors.begin(); it != _walls_colors.end();it++)
	{
		current_wall_color_for_check = it->first;
		ProcessPlace(is_color_wall, is_true, NULL, it->second, "", GameSettings::CELL_RECT, ZBuf::GROUND_3 - ZBuf::Z_EPS);
	}

	if(_exist_indestructible)
	{
		ProcessPlace(is_indestructible, is_true, NULL, Color(255, 230, 100, 120), "", GameSettings::CELL_RECT, ZBuf::GROUND_3 - ZBuf::Z_EPS);
	}

	if(_exist_permanent && EditorUtils::editor)
	{
		ProcessPlace(is_permanent, is_true, NULL, Color(200, 150, 50, 120), "", GameSettings::CELL_RECT, ZBuf::GROUND_3 - ZBuf::Z_EPS);
	}

	if(_exist_fill_order)
	{
		ProcessPlace(is_ordered_to_fill, is_true, NULL, Color(50, 255, 100, 120), "", GameSettings::CELL_RECT, ZBuf::GROUND_3 - ZBuf::Z_EPS);
	}
}

void WallDrawer::DrawGroundBorders()
{
	Render::device.SetStencilTest(true);
	Render::device.SetStencilOp(Render::StencilOp::KEEP, Render::StencilOp::KEEP, Render::StencilOp::KEEP);

	// границы земли и энергии
	Render::device.SetStencilFunc(Render::StencilFunc::NOTEQUAL, 0xC0, 0xF0);
	Place2D::DrawBorders(Place2D::wall_borders, "wall", Color::WHITE, false);

	// границы земли и прокопанной земли
	Render::device.SetStencilFunc(Render::StencilFunc::EQUAL, 0xC0, 0xF0);
	Place2D::DrawBorders(Place2D::wall_borders, "wall_over");

	Render::device.SetStencilTest(false);


	Place2D::DrawBorders(Place2D::wall_energy_borders, "java");
}

void WallDrawer::DrawIce()
{
	if(!_exist_ice)
	{
		return;
	}
	Place2D::SetParams(0.3f, 0.4f, 0.5f);
	Place2D::ice_borders.clear();
	Place2D::Clear();
	Place2D::BindBorders(&Place2D::ice_borders);

	Render::device.PushMatrix();
	Render::device.MatrixTranslate(math::Vector3(0.f, 0.f, 0.0f));
	ProcessPlace(is_ice, is_true, _textureIce, Color::WHITE, "ice", GameSettings::CELL_RECT, ZBuf::ICE, true);
	Render::device.PopMatrix();
}

void WallDrawer::DrawIceBorders()
{
	if(!_exist_ice)
	{
		return;
	}
	Place2D::DrawBorders(Place2D::ice_borders, "ice");
}

void WallDrawer::DrawWood()
{
	if(!_exist_wood)
	{
		return;
	}
	Place2D::SetParams(0.32f, 0.4f, 0.5f);
	Place2D::wood_borders.clear();
	Place2D::Clear();
	Place2D::BindBorders(&Place2D::wood_borders);

	ProcessPlace(is_wood, is_true, _textureWood, Color::WHITE, "wood", (GameSettings::CELL_RECT*2).MovedBy(IPoint(GameSettings::SQUARE_SIDE/4,GameSettings::SQUARE_SIDE/4)) , ZBuf::WOOD_BASE, true);		
}

void WallDrawer::DrawWoodBorders()
{
	if(!_exist_wood)
	{
		return;
	}
	Place2D::DrawBorders(Place2D::wood_borders, "wood");
}

void WallDrawer::DrawCyclops()
{
	if (!_exist_cyclops)
	{
		return;
	}
	Place2D::SetParams(0.3f, 0.4f, 0.5f);
	Place2D::cyclops_borders.clear();
	Place2D::Clear();
	Place2D::BindBorders(&Place2D::cyclops_borders);

	Render::device.PushMatrix();
	Render::device.MatrixTranslate(math::Vector3(0.f, 0.f, 0.0f));
	ProcessPlace(is_cyclops, is_true, _textureCyclops, Color::WHITE, "cyclops", GameSettings::CELL_RECT, ZBuf::CYCLOPS, true);
	Render::device.PopMatrix();
}

void WallDrawer::DrawCyclopsBorders()
{
	if (!_exist_cyclops)
	{
		return;
	}
	Place2D::DrawBorders(Place2D::cyclops_borders, "cyclops");
}

bool is_visible_for_sand(Game::Square *sq)
{
	return is_visible(sq) && (sq->GetWall() == 0 || (sq->GetSandTime() > 0));
}

void WallDrawer::DrawSand()
{
	Place2D::SetParams(0.5f, 0.5f, 0.5f);
	Place2D::Clear();
	//5. Земля - песок
	if(_exist_sand)
	{
		ProcessPlace(is_sand, is_visible_for_sand, Game::sandSquareTexture, Color::WHITE, "", GameSettings::CELL_RECT, 0.0f); 
	}
}

void WallDrawer::DrawStone()
{
	//Рисуем плиты
	Place2D::SetParams(0.25f, 0.5f, 0.35f);
	Place2D::stone_borders.clear();
	Place2D::Clear();
	Place2D::BindBorders(&Place2D::stone_borders);

	Render::device.PushMatrix();
	Render::device.MatrixTranslate(math::Vector3(0.f, 0.f, 0.0f));
	ProcessPlace(is_stone, is_true, _textureStone, Color::WHITE, "stone", GameSettings::CELL_RECT*2, ZBuf::ICE, true);
	Render::device.PopMatrix();

}

void WallDrawer::DrawStoneBorders()
{
	Render::device.PushMatrix();
	Render::device.MatrixTranslate(math::Vector3(0.f, 0.f, 0.f));
	Place2D::DrawBorders(Place2D::stone_borders, "stone");
	Render::device.PopMatrix();
}

void WallDrawer::LoadLevel()
{
	backgroundLow = Core::resourceManager.Get<Render::Texture>(FieldStyles::current->bg_bottom);
	backgroundMiddle = Core::resourceManager.Get<Render::Texture>(FieldStyles::current->bg_middle);
	backgroundTop = Core::resourceManager.Get<Render::Texture>(FieldStyles::current->bg_top);
	backgroundEmpty = Core::resourceManager.Get<Render::Texture>(FieldStyles::current->bg_empty);

	backgroundEmpty -> setFilteringType(Render::Texture::BILINEAR);
	backgroundEmpty -> setAddressType(Render::Texture::REPEAT);

	backgroundLow -> setFilteringType(Render::Texture::BILINEAR);
	backgroundLow -> setAddressType(Render::Texture::REPEAT);
	
	backgroundMiddle -> setFilteringType(Render::Texture::BILINEAR);
	backgroundMiddle -> setAddressType(Render::Texture::REPEAT);
	
	backgroundTop -> setFilteringType(Render::Texture::BILINEAR);
	backgroundTop -> setAddressType(Render::Texture::REPEAT);

	growingWallTex = Core::resourceManager.Get<Render::Texture>("GrowingWall");
	growingWallTex->setFilteringType(Render::Texture::BILINEAR);
	growingWallTex->setAddressType(Render::Texture::REPEAT);

	growingWallFabricTex = Core::resourceManager.Get<Render::Texture>("GrowingWallFabric");
	growingWallFabricTex->setFilteringType(Render::Texture::BILINEAR);
	growingWallFabricTex->setAddressType(Render::Texture::REPEAT);

	if(EditorUtils::editor)
	{
		is_visible = is_visible_editor;
	}else{
		is_visible = is_visible_game;
	}

}

void WallDrawer::InitAfterLoadLevel()
{
	_exist_sand = _exist_grow_wall = _exist__wall_fabric = _exist_energy_wall = _exist_indestructible = _exist_fill_order = EditorUtils::editor;
	_exist_wall2 = _exist_wall3 = EditorUtils::editor;
	_exist_wood = _exist_ice = _exist_cyclops = EditorUtils::editor;
	size_t count = GameSettings::squares.size();
	for(size_t i = 0; i < count; i++)
	{
		Game::Square *sq = GameSettings::squares[i];
		_exist_sand = _exist_sand || sq->IsSand();
		_exist_grow_wall = _exist_grow_wall || is_grow_wall(sq);
		_exist__wall_fabric = _exist__wall_fabric || is_grow_wall_fabric(sq);
		_exist_energy_wall = _exist_energy_wall || sq->IsEnergyWall();
		_exist_indestructible = _exist_indestructible || sq->IsIndestructible();
		_exist_permanent = _exist_permanent || sq->IsPermanent();
		_exist_fill_order = _exist_fill_order || Game::Orders::CellIsOrdered(sq->address);
		_exist_wall3 = _exist_wall3 || sq->GetWall() == 3;
		_exist_wall2 = _exist_wall2 || _exist_wall3 || sq->GetWall() == 2;
		_exist_wood = _exist_wood || sq->GetWood() > 0;
		_exist_ice = _exist_ice || sq->IsIce();
		_exist_cyclops = _exist_cyclops || sq->IsCyclops();
	}
}

void WallDrawer::InitEditor()
{
	_exist_sand = _exist_grow_wall = _exist__wall_fabric = _exist_energy_wall = _exist_indestructible = _exist_fill_order = _exist_permanent = true;
	_exist_wall2 = _exist_wall3 = true;
}