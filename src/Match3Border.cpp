#include "stdafx.h"
#include "Match3Border.h"
#include "FieldStyles.h"
#include "EditorUtils.h"

namespace Gadgets
{
	void LevelBorder::Init(const std::string &name_border_map)
	{
		MyAssert(Place2D::borders.find(name_border_map) != Place2D::borders.end());
		_map = &Place2D::borders[name_border_map];
	}
	
	void LevelBorder::LoadLevel()
	{
		//_texture = Core::resourceManager.Get<Render::Texture>("LevelBorder");
		if(FieldStyles::current) {
			_texture = Core::resourceManager.Get<Render::Texture>(FieldStyles::current->border);
		} else {
			Assert(false);
		}
	}

	void LevelBorder::AddBorder(const IPoint &pos, const BYTE &mask)
	{
		//На основании позиции эмулируем рандом для выбора границы
		size_t variant = 0;
		Place2D::ScopePiece &orig = _map->map.find(mask)->second;
		if(orig.or_rects_v0.size() > 1)
		{
			variant = (int(sqrt(abs(pos.x*pos.y))) >> 3) % orig.or_rects_v0.size();
		}
		_levelScope.push_back(Place2D::ScopePieceForDraw(orig.or_rects_v0[variant].frect, orig.or_rects_v0[variant].rect.MovedBy(pos), 0.0f));
	}

	void LevelBorder::Clear()
	{
		Render::device.Release(&_vb);
		_levelScope.clear();
	}


	bool is_visible_for_border_game(Game::Square *sq)
	{
		return Game::isVisible(sq) && sq->IsStayFly();
	}

	bool is_visible_for_border_editor(Game::Square *sq)
	{
		return Game::isVisible(sq);
	}

	bool ( * is_visible_for_border )(Game::Square *sq) = &is_visible_for_border_game;

	void LevelBorder::Calculate()
	{	
		if(EditorUtils::editor)
		{
			is_visible_for_border = is_visible_for_border_editor;
		}else{
			is_visible_for_border = is_visible_for_border_game;
		}
		Place2D::SetParams(0.26f, 0.5f, 0.5f);
		Clear();
		//for (int x = 1; x < GameSettings::FIELD_MAX_WIDTH - 1; x++)
		//{
		//	for (int y = 1; y < GameSettings::FIELD_MAX_HEIGHT - 1; y++)
		//	{
		IPoint p1 = Game::activeRect.LeftBottom() + IPoint(-1, -1);
		IPoint p2 = p1 + IPoint(Game::activeRect.width+2, Game::activeRect.height + 2);
		for (int x = p1.x; x <= p2.x; x++)
		{
			for (int y = p1.y; y <= p2.y; y++)
			{
				IPoint pos = IPoint(x*GameSettings::SQUARE_SIDE, y*GameSettings::SQUARE_SIDE); //Центр крестика
				Game::Square *s0 = GameSettings::gamefield[x + 1][y + 1];
				Game::Square *s1 = GameSettings::gamefield[x    ][y + 1];
				Game::Square *s2 = GameSettings::gamefield[x    ][y    ];
				Game::Square *s3 = GameSettings::gamefield[x + 1][y    ];
			
				bool is_visible[4] = {	is_visible_for_border(s0) , is_visible_for_border(s1),  is_visible_for_border(s2), is_visible_for_border(s3)};

				BYTE mask = 0;
				for(size_t i = 0; i < 4; i++)
				{
					if(is_visible[i])
					{
						mask = mask | (1 << i);
					}
				}

				if(mask == 0)
				{
					continue;
				}

				if(_map->map.find(mask) != _map->map.end()) {
					AddBorder(pos, mask);
				} else if(mask == 5) {
					AddBorder(pos, 13);
					AddBorder(pos, 7);
				} else if(mask == 10) {
					AddBorder(pos, 11);
					AddBorder(pos, 14);
				}
			}
		}

		if( !_levelScope.empty() )
		{
			_vb.InitQuadBuffer(_levelScope.size());
			for(size_t i = 0; i < _levelScope.size(); ++i)
			{
				_vb.SetQuad(static_cast<int>(i), _levelScope[i].rect, _levelScope[i].frect);
			}
			_vb.Upload();
		}
	}

	// риcуем внешние границы поля
	void LevelBorder::Draw() 
	{
		if(!_levelScope.empty())
		{
			Render::device.PushMatrix();
			Render::device.MatrixTranslate( math::Vector3(0.0f, 0.0f, ZBuf::LEVEL_BORDER) );
			_texture->Bind();
			_vb.Draw();
			Render::device.PopMatrix();
		}
	}
} //Gadgets
