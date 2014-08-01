#include "stdafx.h"
#include "GameChipRemover.h"

#include "Game.h"
#include "GameField.h"
#include "GameOrder.h"


namespace Game {
    
    ChipRemover::ChipRemover(FPoint pos, int color, float pause)
	: GameFieldController("ChipRemover", 4.f, GameField::Get())
	, _pos(pos)
    {
        _uv = Game::GetChipRect(color, true, false, false);
        _state = CR_DELAY;
        _timeState = -pause;
    }
    
    void ChipRemover::Update(float dt)
    {
        if(_state == CR_DELAY)
        {
            _timeState += dt;
            if(_timeState >= 0)
            {
                _timeState = 0.f;
                _state = CR_STAY;
            }
        }else if(_state == CR_STAY)
        {
            _timeState += (dt/Game::ChipColor::CHIP_START_HIDE);
            if(_timeState >= 1)
            {
                _timeState = 0.f;
                _state = CR_HIDE;
            }
        }
        else if(_state == CR_HIDE)
        {
            _timeState += dt*6.f;
            if(_timeState >= 1)
            {
                _timeState = 1.f;
                _state = CR_FINISH;
            }
        }
        
    }
    
    bool ChipRemover::isFinish()
    {
        return _state == CR_FINISH;
    }
    
    void ChipRemover::Draw()
    {
        float scale = 1.f;
        float alpha = 1.f;
        float add_alpha = 0.f;
        
        if(_state == CR_DELAY)
        {
            scale = 1.f;
            alpha = 1.f;
            add_alpha = 0.f;
        }else if(_state == CR_STAY)
        {
            scale = math::lerp(1.f, 1.2f, _timeState);
            alpha = 1.f;
            add_alpha = 0.f;//math::clamp(0.f, 0.2f, sinf(_timeState*math::PI)*0.5f);
        }
        else if(_state == CR_HIDE)
        {
            scale = math::lerp(1.2f, 0.1f, _timeState);
            alpha = math::lerp(1.0f, 0.5f, _timeState);
            add_alpha = 0.f;
        }
        else
        {
            return;
        }
        
        Render::device.PushMatrix();
        Render::device.MatrixTranslate(_pos);
        
        
        Render::device.MatrixScale(scale);
        
        Render::BeginAlphaMul(alpha);
        ChipColor::chipsTex->Draw(ChipColor::DRAW_FRECT.MovedBy(-GameSettings::CELL_HALF), _uv);
        Render::EndAlphaMul();
        
        if(add_alpha > 0)
        {
            Render::device.SetBlendMode(Render::ADD);
            Render::BeginAlphaMul(add_alpha);
            ChipColor::chipsTex->Draw(ChipColor::DRAW_FRECT.MovedBy(-GameSettings::CELL_HALF), _uv);
            Render::EndAlphaMul();
            Render::device.SetBlendMode(Render::ALPHA);
        }
        
        Render::device.PopMatrix();
    }
    
    ChipRemoverByBonus::ChipRemoverByBonus(FPoint pos, int color, float pause)
    : GameFieldController("ChipRemoverByBonus", 4.f, GameField::Get())
    , _pos(pos)
    {
        _uv = Game::GetChipRect(color, true, false, false);
        _state = CR_DELAY;
        _timeState = -pause;
    }
    
    void ChipRemoverByBonus::Update(float dt)
    {
        if(_state == CR_DELAY)
        {
            _timeState += dt;
            if(_timeState >= 0)
            {
                _timeState = 0.f;
                _state = CR_STAY;
            }
        }else if(_state == CR_STAY)
        {
            _timeState += dt*4.f;
            if(_timeState >= 1)
            {
                _timeState = 0.f;
                _state = CR_HIDE;
            }
        }
        else if(_state == CR_HIDE)
        {
            _timeState += dt*8.f;
            if(_timeState >= 1)
            {
                _timeState = 1.f;
                _state = CR_FINISH;
            }
        }
        
    }
    
    bool ChipRemoverByBonus::isFinish()
    {
        return _state == CR_FINISH;
    }
    
    void ChipRemoverByBonus::Draw()
    {
        float scale = 1.f;
        float alpha = 1.f;
        float add_alpha = 0.f;
        
        if(_state == CR_DELAY)
        {
            scale = 1.f;
            alpha = 1.f;
            add_alpha = 0.f;
        }else if(_state == CR_STAY)
        {
            scale = math::lerp(1.f, 1.1f, _timeState);
            alpha = 1.f;
            add_alpha = math::clamp(0.f, 2.f, sinf(_timeState*math::PI)*0.5f);
        }
        else if(_state == CR_HIDE)
        {
            scale = math::lerp(1.1f, 0.1f, _timeState);
            alpha = math::lerp(1.0f, 0.5f, _timeState);
            add_alpha = 0.f;
        }
        else
        {
            return;
        }
        
        Render::device.PushMatrix();
        Render::device.MatrixTranslate(_pos);
        
        
        Render::device.MatrixScale(scale);
        
        Render::BeginAlphaMul(alpha);
        ChipColor::chipsTex->Draw(ChipColor::DRAW_FRECT.MovedBy(-GameSettings::CELL_HALF), _uv);
        Render::EndAlphaMul();
        
        if(add_alpha > 0)
        {
            Render::device.SetBlendMode(Render::ADD);
            Render::BeginAlphaMul(add_alpha);
            ChipColor::chipsTex->Draw(ChipColor::DRAW_FRECT.MovedBy(-GameSettings::CELL_HALF), _uv);
            Render::EndAlphaMul();
            Render::device.SetBlendMode(Render::ALPHA);
        }
        
        Render::device.PopMatrix();
    }



ChipOrderRemover::ChipOrderRemover(FPoint pos, FPoint to, int color, Game::FieldAddress square)
	: GameFieldController("ChipOrderRemover", 2.0f, GameField::Get())
	, _square(square)
	, _color(color)
{
	_uv = Game::GetChipRect(color, true, false, false);

	_path.addKey(pos);
	_path.addKey(to);
	_path.CalculateGradient();
	Game::Orders::KillCell(Game::Order::Objective(_color), _square);
}

void ChipOrderRemover::Update(float dt)
{
	if( local_time < 1.0f )
	{
		local_time += time_scale * dt;
	}
}

bool ChipOrderRemover::isFinish()
{
	return (local_time >= 1.0f);
}

void ChipOrderRemover::Draw()
{
	float t = math::ease(std::min(local_time, 1.0f), 0.2f, 0.2f);

	Render::device.PushMatrix();

	FPoint pos = _path.getGlobalFrame(t);
	Render::device.MatrixTranslate(pos);

	float scale = math::lerp(1.0f, 0.5f, t);
	Render::device.MatrixScale(scale);

	float alpha = math::lerp(1.0f, 0.5f, t);
	Render::BeginAlphaMul(alpha);
	ChipColor::chipsTex->Draw(ChipColor::DRAW_FRECT.MovedBy(-GameSettings::CELL_HALF), _uv);
	Render::EndAlphaMul();

	Render::device.PopMatrix();
}

} // namespace Game