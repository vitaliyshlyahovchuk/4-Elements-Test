#include "stdafx.h"
#include "SelectingChip.h"
#include "Game.h"
#include "GameField.h"

Render::Texture *textureLink = 0;
Render::Texture *textureBack = 0;

Render::Texture *textureLink2 = 0;
Render::Texture *textureBack2 = 0;

void SelectingChips::Init()
{
	textureLink = Core::resourceManager.Get<Render::Texture>("SeqLink");
	textureLink -> setFilteringType(Render::Texture::BILINEAR);
	textureLink -> setAddressType(Render::Texture::REPEAT);
	textureBack = Core::resourceManager.Get<Render::Texture>("ChipInSeqBack");

	textureLink2 = Core::resourceManager.Get<Render::Texture>("SeqLink2");
	textureLink2 -> setFilteringType(Render::Texture::BILINEAR);
	textureLink2 -> setAddressType(Render::Texture::REPEAT);
	textureBack2 = Core::resourceManager.Get<Render::Texture>("ChipInSeqBack2");
}

void SelectingChips::InitConstruct()
{
	_timerNewLight = 0.1f;
	 _needNewLight = false;
}

void DrawLink2(const FPoint &pos_prev, const FPoint &pos_next, const float &time, const Color &color, const bool &diagonal)
{
	FPoint dir = (pos_next - pos_prev).Normalized()*GameSettings::SQUARE_SIDEF;
	std::swap(dir.x, dir.y);  dir.y = -dir.y; //Перпендикулярно к направлению
	float width = 0.45f*1.5f;
	FPoint p0_up = pos_prev + dir*width;
	FPoint p0_down = pos_prev - dir*width;

	FPoint p1_up = pos_next + dir*width;
	FPoint p1_down = pos_next - dir*width;

	float  v0 = (p0_down.y < p0_up.y) ? 0.f: 1.f;
	float  v1 = (p0_down.y < p0_up.y) ? 1.f: 0.f;

	Render::DrawQuad(p0_down, p1_down, p0_up, p1_up, Color::WHITE, Color::WHITE, Color::WHITE, Color::WHITE, 0.f+time+pos_prev.x/100.f, 1.f+time+pos_prev.x/100.f, v0, v1);
}

void SelectingChips::Draw(AddressVector &seq, float time)
{
//
//	if(seq.empty())
//	{
//		return;
//	}
//
//	size_t count = seq.size();
//	for(size_t i = 0; i < count; i++)
//	{
//		FPoint pos = FPoint(seq[i].ToPoint())*GameSettings::SQUARE_SIDEF;
//		Game::Square *sq =  GameSettings::gamefield[seq[i]];
//		if(sq->GetChip()._effInSelection == 0)
//		{
//			continue;
//		}
//		Render::device.PushMatrix();
//		if(sq->IsIce()) {			
//			Render::device.MatrixTranslate(pos + FPoint(-10.f, -3.f)*GameSettings::SQUARE_SCALE + FPoint(0.f, Game::ChipColor::YOffset_ChipBackIce));
//		} else {
//			Render::device.MatrixTranslate(pos + FPoint(-10.f, -3.f)*GameSettings::SQUARE_SCALE + FPoint(0.f, Game::ChipColor::YOffset_ChipBack));
//		}
//		Game::MatrixSquareScale();
//		textureBack->Draw(FPoint(0.f, 0.f));
//		Render::device.PopMatrix();
//	}
//
//	textureLink->Bind();
//	if(seq.size() > 1)
//	{
//		FPoint offset_3D =  FPoint(0.f, 0.f); //Фишка в клетку вписывается чуть выше в псевдо 3D.
//		Game::Square *sq_prev = GameSettings::gamefield[seq[0]];
//		FPoint pos_prev = sq_prev->GetChipPos() +  GameSettings::CELL_HALF + offset_3D;
//		for(size_t i = 1; i < count; i++)
//		{
//			Game::Square *sq =  GameSettings::gamefield[seq[i]];
//			if(sq->GetChip()._effInSelection == 0)
//			{
//				continue;
//			}
//			FPoint pos_next = sq->GetChipPos() +  GameSettings::CELL_HALF + offset_3D; 
//			FPoint dir = pos_next - pos_prev;
//
//			DrawLink2(pos_prev, pos_next, time, Color::WHITE, false);
//
//			if(_needNewLight && dir.x != 0)
//			{
//				ParticleEffectPtr eff = Game::AddEffect(GameField::gameField->_effUnderChipsCont, "ChipInSeqLine");
//				eff->SetPos(math::lerp(pos_prev, pos_next, math::random(1.f)));
//				eff->Reset();
//			}
//			pos_prev = pos_next;
//			sq_prev = sq;
//		}
//	}
//	Render::device.SetBlendMode(Render::ALPHA);
//
//	_needNewLight = false;
}

void SelectingChips::Clear()
{
}

void SelectingChips::Update(float dt)
{
	_timerNewLight -= dt;
	if(_timerNewLight < 0)
	{
		_needNewLight = true;
		_timerNewLight = 0.1f;
	}
}

/*
*
*	ChipInSequence
*
*/

ChipInSequence::ChipInSequence(IPoint index, bool draw_link , IPoint index_from)
	: GameFieldController("ChipInSequence", 1.f, GameField::Get())
	, _posFrom(FPoint(index_from)*GameSettings::SQUARE_SIDEF + GameSettings::CELL_HALF)
	, _posTo(FPoint(index)*GameSettings::SQUARE_SIDEF + GameSettings::CELL_HALF)
	, _draw_link(draw_link)
	, _timerHide(1.f)
	, _localTime(0.f)
	, _backTex(textureBack)
	, _linkTex(textureLink)
	, _timerNewLight(0.1f)

{
	_sq = GameSettings::gamefield[index];

	_effInSelection = _effCont.AddEffect("ChipInSeq");
	_effInSelection->SetPos(FPoint(index)*GameSettings::SQUARE_SIDEF);
	_effInSelection->Reset();
}

ChipInSequence::~ChipInSequence()
{
	if(_effInSelection.get()){
		_effInSelection->Kill();
	}
	_effInSelection.reset();
}

void ChipInSequence::Update(float dt)
{
	if(0.f < _timerHide &&  _timerHide < 1.f)
	{
		_timerHide -= dt*4.f;
		if(_timerHide <= 0.f)
		{
			_timerHide = 0.f;
		}
	}
	_localTime += dt;

	_timerNewLight -= dt;
	if(_timerNewLight < 0)
	{
		_timerNewLight = 0.3f;
		FPoint dir = _posTo - _posFrom;

		if(dir.x != 0)
		{
			ParticleEffectPtr eff = _effCont.AddEffect("ChipInSeqLine");
			eff->SetPos(math::lerp(_posFrom, _posTo, math::random(1.f)));
			eff->Reset();
		}
	}
	_effCont.Update(dt);
}

void ChipInSequence::DrawUnderChips()
{
	Render::BeginAlphaMul(math::clamp(0.f, 1.f, _timerHide));
	Render::device.PushMatrix();
	if(_sq->IsIce()) {			
		Render::device.MatrixTranslate(_posTo + FPoint(-50.f, -43.f)*GameSettings::SQUARE_SCALE + FPoint(0.f, Game::ChipColor::YOffset_ChipBackIce));
	} else {
		Render::device.MatrixTranslate(_posTo + FPoint(-50.f, -43.f)*GameSettings::SQUARE_SCALE + FPoint(0.f, Game::ChipColor::YOffset_ChipBack));
	}
	Game::MatrixSquareScale();
	_backTex->Draw(FPoint(0.f, 0.f));
	Render::device.PopMatrix();

	if(_draw_link)
	{
		_linkTex->Bind();
		FPoint offset_3D =  FPoint(0.f, 0.f); //Фишка в клетку вписывается чуть выше в псевдо 3D.
		
		FPoint dir = _posTo - _posFrom;

		DrawLink2(_posFrom, _posTo, _localTime, Color::WHITE, false);
	}
	_effCont.Draw();
	Render::EndAlphaMul();
}

void ChipInSequence::Hide()
{
	_timerHide = 0.998f;
	_backTex = textureBack2;
	_linkTex = textureLink2;
}

bool ChipInSequence::isFinish()
{
	return _timerHide <= 0.f;
}