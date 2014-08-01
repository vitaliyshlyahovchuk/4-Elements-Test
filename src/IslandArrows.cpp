#include "stdafx.h"
#include "IslandArrows.h"
#include "Game.h"
#include "FreeFront.h"

namespace Island
{
	Render::Texture *arrow_texture = 0;
	float arrow_time = 0.f;

	void InitGame()
	{
		arrow_texture = Core::resourceManager.Get<Render::Texture>("IslandArrow");
		arrow_time = 0.f;
	}

	void Update(float dt)
	{
		arrow_time += dt;
	}

	void DrawArrows()
	{
		//—трелы показывающие продолжение пол€
		IPoint lines[4][3] = {	
			{Game::activeRect.RightBottom() + IPoint(1,-1), Game::activeRect.RightTop() + IPoint(1,1), IPoint(0,1)},
			{Game::activeRect.RightTop() + IPoint(1,1), Game::activeRect.LeftTop() + IPoint(-1,1), IPoint(-1,0)},
			{Game::activeRect.LeftTop() + IPoint(-1,1), Game::activeRect.LeftBottom() + IPoint(-1,-1), IPoint(0,-1)},
			{Game::activeRect.LeftBottom() + IPoint(-1,-1), Game::activeRect.RightBottom() + IPoint(1,-1), IPoint(1,0)}
		};
		float time = arrow_time*6.f;
		for(size_t i = 0; i < 4; i++)
		{
			for(IPoint p = lines[i][0], dir = lines[i][2]; p != lines[i][1]; p += dir)
			{
				Game::Square *sq = GameSettings::gamefield[p];
				//time += math::PI/2.f;
				if(!Game::isVisible(sq) || !(sq->IsFlyType(sq->FLY_NO_DRAW) || sq->IsFlyType(sq->FLY_HIDING)))
				{
					continue;
				}
				IPoint near_index = p + IPoint(-dir.y, dir.x);
				Game::Square *sq_near = GameSettings::gamefield[near_index];
				if(Game::isVisible(sq_near) && !sq_near->IsFlyType(Game::Square::FLY_NO_DRAW))
				{
					if(Gadgets::squareDist[p] > Gadgets::squareDist[near_index])
					{
						continue;
					}
					Render::device.PushMatrix();
					Render::device.MatrixTranslate(sq_near->GetCellPos() - FPoint(-dir.y, dir.x)*(GameSettings::SQUARE_SIDEF*0.47f + 2.f*sinf(time)) + GameSettings::CELL_HALF);
					Game::MatrixSquareScale();
					Render::device.MatrixRotate(math::Vector3(0.f, 0.f, 1.f), i*90.f);
					Render::device.MatrixScale(0.95f + 0.05f*sinf(time));
					Render::BeginAlphaMul(sq_near->_flyArrowAlpha*math::clamp(0.f, 1.f, 0.7f + 0.3f*sinf(time)));
					float offset_x = -27.f;
					arrow_texture->Draw(offset_x, -40.f);
					Render::EndAlphaMul();
					Render::device.PopMatrix();	

				}
			}
		}
	}
}