#include "stdafx.h"
#include "GameColor.h"
#include "ActCounter.h"
#include "FlyTextBySpline.h"
#include "GameField.h"
#include "Match3Gadgets.h"
#include "GameChipExplodeRemover.h"
#include "EditorUtils.h"
#include "GameChipRemover.h"
#include "GameFillBonus.h"
#include "GameInfo.h"
#include "Match3.h"
#include "CombinedBonus.h"
#include "GameBonuses.h"
#include "GameFieldControllers.h"
#include "StarArrowFlyEffect.h"
#include "Energy.h"
#include "MovingMonster.h"
#include "Match3Spirit.h"

namespace Game
{
	float MIN_LEVEL_ALPHA_SEQ = 0.7f;
	bool ChipColor::chipSeqIsEmpty = true;
	size_t ChipColor::tutorialChainLength = 0;
	Render::Texture* ChipColor::chipsTex = NULL;
	Render::Texture* lockTex1 = NULL;
	Render::Texture* lockTex2 = NULL;
	IRect ChipColor::DRAW_RECT;
	FRect ChipColor::DRAW_FRECT;
	float ChipColor::timeAfterLastFallSound = 0.0f;
	int ChipColor::COLOR_FOR_ADAPT = 0;
	Render::Texture* BACK_ACT = NULL;
	Render::Texture* timeBombTex = NULL;
	FPoint BACK_ACT_CENTER;
	float ChipColor::YOffset_Chip = 0.f;
	float ChipColor::YOffset_ChipBack = 0.f;
	float ChipColor::YOffset_ChipBackIce = 0.f;
	float ChipColor::YOffset_Choc = 0.f;
	float ChipColor::YOffset_ChocFabric = 0.f;

	float ChipColor::YOffset_WOOD[5] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f};

	float ChipColor::YOffset_STONE = 0.f;	
	float ChipColor::CHIP_REMOVE_DELAY = 0.f;	

	IPoint ChipColor::CHIP_TAP_OFFSET = IPoint(0,0);

	int ChipColor::CURRENT_IN_SEQUENCE_ITER = 0;
	float ChipColor::CHIP_START_HIDE = 0.25f;

	struct StarArrowInfo
	{
		FPoint pos;
		Render::Texture *texture;
		float angle;
	};
	std::map<std::string, StarArrowInfo> _starArrowsInfo;

	/*
	* ChipColor
	*/
	void ChipColor::InitGame(rapidxml::xml_node<> *xml_node)
	{
		ChipColor::chipsTex = Core::resourceManager.Get<Render::Texture>("Chips");
		lockTex1 = Core::resourceManager.Get<Render::Texture>("Lock1");		
		lockTex2 = Core::resourceManager.Get<Render::Texture>("Lock2");
		BACK_ACT = Core::resourceManager.Get<Render::Texture>("ChipActBackground");		
		BACK_ACT_CENTER = FPoint(BACK_ACT->getBitmapRect().RightTop())*0.5f;
		ChipColor::timeAfterLastFallSound = 1.f;
		timeBombTex = Core::resourceManager.Get<Render::Texture>("TimeBombBackground");
		MIN_LEVEL_ALPHA_SEQ = gameInfo.getGlobalFloat("MIN_LEVEL_ALPHA_SEQ", 0.7f);

		rapidxml::xml_node<> *xml_arrow = xml_node->first_node("StarArrow")->first_node("items");
		while(xml_arrow)
		{
			std::string name = Xml::GetStringAttribute(xml_arrow, "name");
			_starArrowsInfo[name].pos = FPoint(xml_arrow) + FPoint(0.f, gameInfo.getConstFloat("OffsetY_Lightning", 0) - gameInfo.getConstFloat("YOffset_Chip", 0.f))*GameSettings::SQUARE_SCALE;
			_starArrowsInfo[name].texture = Core::resourceManager.Get<Render::Texture>("StarArrow_" + name);	
			_starArrowsInfo[name].angle = Xml::GetFloatAttribute(xml_arrow, "angle");
			xml_arrow = xml_arrow->next_sibling("items");
		}
	}

	ChipColor::ChipColor(int value)
		: _seqBonusEff(0)
		, _seqBonusEffDown(0)
		, _chipInSequence(0)
		, _isOnActiveZone(false)
	{
		Reset(true);
		_value = value;
	}

	void ChipColor::DrawDiagonalBlic(int value, FPoint pos, int countX, int countY, float fi, float time, int alpha, Color c, Render::SpriteBatch* batch, bool eye_open, bool in_ice)
	{
		//Рисуется только для обычных фишек
		//ChipColor::chipsTex->Bind(0, Render::STAGE_C_MODULATE + Render::STAGE_A_MODULATE);
		//Render::device.SetBlendMode(Render::ADD);
		if(!batch)
		{
			return; //При отрисовке из контроллера блик можно не выводить пока
		}
		FRect rect = GetDrawRect(value, in_ice, false);

		FRect uv(Game::GetChipRect(value, eye_open, in_ice, _selected > 0));
		ChipColor::chipsTex->TranslateUV(rect, uv);

		int in = countX;
		int jn = countY;
		float k = fi/2.f;

		bool hor = false;

		for (int i = 0; i<in; i++)
		{
			for (int j = 0; j<jn; j++)
			{
				float x1 = math::lerp(rect.xStart, rect.xEnd, (i+0.f)/in) + pos.x;
				float x2 = math::lerp(rect.xStart, rect.xEnd, (i+1.f)/in) + pos.x;
				float y1 = math::lerp(rect.yStart, rect.yEnd, (j+0.f)/jn) + pos.y;
				float y2 = math::lerp(rect.yStart, rect.yEnd, (j+1.f)/jn) + pos.y;

				float u1 = math::lerp(uv.xStart, uv.xEnd, (i+0.f)/in);
				float u2 = math::lerp(uv.xStart, uv.xEnd, (i+1.f)/in);
				float v1 = math::lerp(uv.yStart, uv.yEnd, (j+0.f)/jn);
				float v2 = math::lerp(uv.yStart, uv.yEnd, (j+1.f)/jn);

				float t1, t2, t3, t4;

				if (hor) {
					t1 = math::clamp(0.0f, 1.0f, -(i + j + 0.f) / (in + jn) * k + time * fi);
					t2 = math::clamp(0.0f, 1.0f, -(i + j + 1.f) / (in + jn) * k + time * fi);
					t3 = math::clamp(0.0f, 1.0f, -(i + j + 1.f) / (in + jn) * k + time * fi);
					t4 = math::clamp(0.0f, 1.0f, -(i + j + 2.f) / (in + jn) * k + time * fi);
				} else {
					t1 = math::clamp(0.0f, 1.0f, -(i + (jn - j - 1) + 1.f) / (in + jn) * k + time * fi);
					t2 = math::clamp(0.0f, 1.0f, -(i + (jn - j - 1) + 2.f) / (in + jn) * k + time * fi);
					t3 = math::clamp(0.0f, 1.0f, -(i + (jn - j - 1) + 0.f) / (in + jn) * k + time * fi);
					t4 = math::clamp(0.0f, 1.0f, -(i + (jn - j - 1) + 1.f) / (in + jn) * k + time * fi);
				}

				Color cv[4] = { Color(c.red, c.green, c.blue, math::lerp(0, alpha, math::sin(t1*math::PI))),
								Color(c.red, c.green, c.blue, math::lerp(0, alpha, math::sin(t3*math::PI))),
								Color(c.red, c.green, c.blue, math::lerp(0, alpha, math::sin(t4*math::PI))),
								Color(c.red, c.green, c.blue, math::lerp(0, alpha, math::sin(t2*math::PI))) };

				batch->Draw(ChipColor::chipsTex, Render::ADD, math::Vector3(x1, y1, 0.0f), math::Vector3(x1, y2, 0.0f), 
					math::Vector3(x2, y2, 0.0f), math::Vector3(x2, y1, 0.0f), cv , FRect(u1, u2, v1, v2));
			}
		}
	
		//Render::device.SetBlendMode(Render::ALPHA);
	}

	void ChipColor::DrawLightEffect(int value, FRect draw_rect, FRect uv) const
	{
		ChipColor::chipsTex->BindAlpha();
		Render::device.PushMatrix();
		FPoint center_pos = draw_rect.CenterPoint();
		Render::device.MatrixTranslate(center_pos);
		float t = math::sin(_localTime * 3.0f);
		Render::device.MatrixScale(1.2f + 0.02f * t);
		FPoint center_frect = DRAW_FRECT.CenterPoint();
		Render::device.MatrixTranslate(-math::Vector3(center_frect.x, center_frect.y, -center_frect.y));
		Color col = GameSettings::chip_settings[value].color_wall;
		col.alpha = 210 + math::round(35.0f * t);
		Render::BeginColor(col);
		Render::DrawRect(DRAW_RECT, uv);
		Render::EndColor();

		Render::device.MatrixTranslate(math::Vector3(center_frect.x, center_frect.y, -center_frect.y));
		

		const float radius = DRAW_FRECT.Width() * 0.5f;
		const int BEAMS = 3;
		float angle = _localTime * 2.0f;
		float da = 2.0f * math::PI / BEAMS;
		Render::device.SetBlendMode(Render::ADD);
		Render::BeginAlphaMul(0.5f);
		for(int i = 0; i < BEAMS; ++i)
		{
			FPoint pt1 = FPoint(math::sin(angle + i*da), math::cos(angle + i*da));
			FPoint pt2 = FPoint(math::sin(angle + i*da + da*0.08f), math::cos(angle + i*da + da*0.08f));
			FPoint pt3 = FPoint(math::sin(angle + i*da + da*0.22f), math::cos(angle + i*da + da*0.22f));
			FPoint pt4 = FPoint(math::sin(angle + i*da + da*0.30f), math::cos(angle + i*da + da*0.30f));

			FPoint uv1 = pt1 * 0.5f + FPoint(0.5f, 0.5f);
			FPoint uv2 = pt2 * 0.5f + FPoint(0.5f, 0.5f);
			FPoint uv3 = pt3 * 0.5f + FPoint(0.5f, 0.5f);
			FPoint uv4 = pt4 * 0.5f + FPoint(0.5f, 0.5f);
			uv1.x = uv.xStart + uv1.x * uv.Width();
			uv1.y = uv.yStart + uv1.y * uv.Height();
			uv2.x = uv.xStart + uv2.x * uv.Width();
			uv2.y = uv.yStart + uv2.y * uv.Height();
			uv3.x = uv.xStart + uv3.x * uv.Width();
			uv3.y = uv.yStart + uv3.y * uv.Height();
			uv4.x = uv.xStart + uv4.x * uv.Width();
			uv4.y = uv.yStart + uv4.y * uv.Height();
			pt1 *= radius;
			pt2 *= radius;
			pt3 *= radius;
			pt4 *= radius;

			Color c0 = Color::WHITE_TRANSPARENT;
			Color c1 = Color::WHITE;

			Render::device.TrueDraw(math::Vector3::Zero, math::Vector3::Zero, pt1, pt2,
				c1, c1, c0, c1, uv.CenterPoint(), uv.CenterPoint(), uv1, uv2);

			Render::device.TrueDraw(math::Vector3::Zero, math::Vector3::Zero, pt2, pt3,
				c1, c1, c1, c1, uv.CenterPoint(), uv.CenterPoint(), uv2, uv3);

			Render::device.TrueDraw(math::Vector3::Zero, math::Vector3::Zero, pt3, pt4,
				c1, c1, c1, c0, uv.CenterPoint(), uv.CenterPoint(), uv3, uv4);
		}
		Render::EndAlphaMul();
		Render::device.SetBlendMode(Render::ALPHA);

		Render::device.PopMatrix();
	}

	FRect ChipColor::GetDrawRect(int type, bool in_ice, bool hang_pulse) const
	{
		FRect rect(DRAW_FRECT);

		rect.yEnd *= _scale.y;
		float dx = 0.5f * rect.Width() * (1.0f - _scale.x);
		rect.xStart += dx;
		rect.xEnd -= dx;

		//ToDo Все деформации можно перевести на _distortions

		if(_playSelectedTime > 0 && !in_ice)
		{
			//Добавляем деформацию анимации выделения в цепочке
			float t = _playSelectedTime;
			if(!_playSelected)
			{
				t = 4.5f*math::PI*_playSelectedTime;
				t = (cosf(t) + 1.f)*0.5f*_playSelectedTime;
			}
			float deform_value = 0.1f*rect.Height()*t;
			rect.yEnd = rect.yEnd - deform_value;
			rect.xStart = rect.xStart - deform_value*0.25f;
			rect.xEnd = rect.xEnd + deform_value*0.25f;
		}
	
		if(_fallingState == FALL_DOWN) {
			float deform = 0.08f * rect.Width() * math::sin(0.5f * math::PI * _timeChipFalling);
			rect.xStart += deform;
			rect.xEnd -= deform;
			rect.yEnd += 1.8f * deform;
		} else if(_fallingState == FALL_AFTER) {
			float deform = 0.08f * rect.Width() * math::cos(2.5 * math::PI * _timeChipFalling) * (1.0f - pow(_timeChipFalling, 3.0f));
			rect.xStart += deform;
			rect.xEnd -= deform;
			rect.yEnd += 1.8f * deform;
		}

		if( hang_pulse ) 
		{
			float t = 0.5f + 0.5f * math::sin(GameSettings::fieldTimer * 10.0f + math::PI * 0.25f);
			float dx = rect.Width() * 0.015f * t;
			float dy = rect.Height() * 0.015f * t;
			rect.yEnd -= dy;
			rect.xStart -= dx;
			rect.xEnd += dx;
		}

		for(auto &dist : _distortions)
		{
			dist->CorrectRect(rect);
		}

		return rect;
	}

	void ChipColor::DrawChipFixedColor(int value, const FPoint &pos, Render::SpriteBatch* batch, bool in_sequence, bool in_ice, bool has_hang) const
	{
		FRect f_rect = Game::GetChipRect(value, has_hang, in_ice, in_sequence);
		if(_mirror)
		{
			std::swap(f_rect.xStart, f_rect.xEnd);
		}
		FRect rect_for_draw_chip = GetDrawRect(value, in_ice, has_hang).MovedBy(pos);

		if(_light)
		{
			if(batch)
			{
				batch->Flush();
			}
			DrawLightEffect(value, rect_for_draw_chip, f_rect);
		}

		Render::BeginAlphaMul(_chipAlpha);
		if(batch) {
			batch->Draw(ChipColor::chipsTex, Render::ALPHA, rect_for_draw_chip, f_rect);
		} else {
			ChipColor::chipsTex->Draw(rect_for_draw_chip, f_rect);
		}
		Render::EndAlphaMul();

		if( has_hang ) //монстрик тоже подпрыгивает
		{
			float t = 0.5f + 0.5f * math::sin(GameSettings::fieldTimer * 10.0f + math::PI * 0.25f);
			Render::BeginAlphaMul(math::lerp(0.0f, 0.3f, t));
			Render::device.SetBlendMode(Render::ADD);
			if(batch){
				batch->Draw(ChipColor::chipsTex, Render::ADD, rect_for_draw_chip, f_rect);
			}else{
				ChipColor::chipsTex->Draw(rect_for_draw_chip, f_rect);
			}
			Render::device.SetBlendMode(Render::ALPHA);
			Render::EndAlphaMul();
		}
	}

	void ChipColor::DrawHangUnder(int value, const FPoint &pos, Render::SpriteBatch* batch, bool active, bool in_ice) const
	{
        /*
		FRect f_rect = Game::GetChipRect(value, true, in_ice, active);
		if(_mirror)
		{
			std::swap(f_rect.xStart, f_rect.xEnd);
		}
		FRect rect_for_draw_chip = GetDrawRect(value, in_ice, true).MovedBy(pos).Inflated(6);

		//batch->Draw(NULL, Render::ALPHA, rect_for_draw_chip, f_rect);
		Render::DrawRect(rect_for_draw_chip, f_rect);
        */
	}

	float ChipColor::GetDistortionsAlpha(bool &show_eye_for_anim)
	{
		float alpha = 1.f;
		for(auto i: _distortions)
		{
			show_eye_for_anim = show_eye_for_anim || i->ShowEye();
			alpha *= i->GetAlpha();
		}	
		return alpha;
	}

	void ChipColor::Draw(const FPoint &pos_, const Game::FieldAddress &address, Render::SpriteBatch* batch, bool in_ice)
	{
		bool show_eye_for_anim = false;
		float alpha_dist = GetDistortionsAlpha(show_eye_for_anim);

		Render::BeginAlphaMul(alpha_dist);
		FPoint pos = pos_ + GetPos() + FPoint(0.f, ChipColor::YOffset_Chip);
		_lastChipPos = pos + GameSettings::CELL_HALF;

		UpdateDrawHang(address);

		if(_isOnActiveZone && _hang_current)
		{
			// Рисуем навешиваемые бонусы
			if( IsEnergyBonus() || _hang_current->GetBonusByType(HangBonus::LIGHTNING) ) {
				_hang_current->DrawOnSquare(_lastChipPos, _localTime, batch);
			} else {
				_hang_current->DrawOnChip(_lastChipPos, _localTime, batch);
			}
		}

		if(_type == LICORICE) {
			DrawChipFixedColor(Game::LICORICE, pos, batch);
		} else if(_type == STAR) {
			//Тени рисуются за молниями
			for(Byte i = 0; i < _starAct; i++)
			{
				int id = _starAct*10 + i + 1;
				StarArrowInfo &info_shade = _starArrowsInfo[utils::lexical_cast(id) + "_shade"];
				info_shade.texture->Draw(pos + info_shade.pos);// + offset);
			}
			Byte order_draw[3][3] = {{0, 0, 0}, {0, 1, 0}, {1, 0, 2}};
			for(Byte k = 0; k < _starAct; k++)
			{
				Render::device.PushMatrix();
				Render::device.MatrixTranslate(pos);
				Game::MatrixSquareScale();
				Byte i = order_draw[_starAct-1][k];
				int id = _starAct*10 + i + 1;
				StarArrowInfo &info = _starArrowsInfo[utils::lexical_cast(id)];
				FPoint offset(0.f, 0.f);
				float star_max_time = 0.3f + i*0.2f;
				float t = (star_max_time - _starActLightTimer)/0.6f + 0.5f;
				float alpha_add = 0.f;
				if(0 < t && t < 1)
				{
					offset.y += 2.f*sinf(t*math::PI);
					alpha_add = math::clamp(0.f, 1.f, sinf(t*math::PI));
				}
				info.texture->Draw(info.pos + offset);
				if(alpha_add > 0)
				{
					Render::device.SetBlendMode(Render::ADD);
					Render::BeginAlphaMul(alpha_add*0.5f);
					info.texture->Draw(info.pos + offset);
					Render::EndAlphaMul();
					Render::device.SetBlendMode(Render::ALPHA);
				}
				Render::device.PopMatrix();
			}
		} else if(_type == TREASURE) {
			DrawChipFixedColor(Game::TREASURE, pos, batch);
		} else if(_type == KEY) {
			DrawChipFixedColor(Game::KEY, pos, batch);
		//} else if(_type == DIAMOND) {
		//	DrawChipFixedColor(Game::DIAMOND, pos, batch);
		} else if(IsAdapt()) {
			if(!_hang_current || !_hang_current->GetBonusByType(HangBonus::LIGHTNING))
				DrawChipFixedColor(ADAPT, pos, batch);
		} else if(IsChameleon() && EditorUtils::editor)	{ //Хамелеон - он хамелеон только в редакторе
			DrawChipFixedColor(Game::SNOW, pos, batch);
		} else if(_type == MUSOR) {
			DrawChipFixedColor(Game::MUSOR, pos, batch);
		} else if(GetColor() >= 0) {
			if(IsFutureHang())
			{
				DrawWhiteBorder(pos, in_ice);			
			}
			bool has_hang = _hang_current && !_hang_current->IsEmpty() && _isOnActiveZone;
			if(has_hang)
			{
				Game::ChipColor::chipsTex->Bind(0, Render::STAGE_A_MODULATE);
				Color color = Color::WHITE;
				Render::BeginColor(color);
				DrawHangUnder(GetColor(), pos, batch, _selected > 0, in_ice);
				Render::EndColor();
				Game::ChipColor::chipsTex->Bind(0);
				DrawChipFixedColor(GetColor(), pos, batch, false, in_ice, true);
			}
			else
			{
				DrawChipFixedColor(GetColor(), pos, batch, _selected > 0, in_ice && !show_eye_for_anim, show_eye_for_anim);
			}

			if (_blicked)
			{
				const float fi = 2.0f;
				if (_firstHighlighted) { 
					DrawDiagonalBlic(GetColor(), pos, 2, 2, fi, _blickTime, 150, Color::WHITE, batch, has_hang, in_ice);
				} else {
					DrawDiagonalBlic(GetColor(), pos, 2, 2, fi, _blickTime,  80, Color::WHITE, batch, has_hang, in_ice);
				}
			}
		}

		if(_chipAnim)
		{
			FPoint animPos = FPoint(-82.f, -82.f) + pos;
			_chipAnim->Draw(animPos.Rounded());
		}
		// Рисуем доп.ходы висящие на фишке
		if(_act_count > 0)
		{
			if(batch)
			{
				batch->Flush();
			}
			//Рисуем от правого верхнего края клетки
			FPoint text_pos = pos + FPoint(GameSettings::SQUARE_SIDEF - 12.f, GameSettings::SQUARE_SIDEF - 12.f);
			BACK_ACT->Draw(text_pos - BACK_ACT_CENTER);
			Render::FreeType::BindFont("HeroTooltip");
			Render::PrintString(text_pos, utils::lexical_cast(_act_count), 1.f, CenterAlign, CenterAlign);
		}
		// Рисуем кол-во ходов до взрыва бомбы
		if(_time_bomb > 0)
		{
			if(batch)
			{
				batch->Flush();
			}
			FPoint text_pos = pos + FPoint(GameSettings::SQUARE_SIDEF, GameSettings::SQUARE_SIDEF) * 0.5f;
			Render::device.PushMatrix();
			Render::device.MatrixTranslate(math::Vector3(text_pos.x, text_pos.y, -text_pos.y));
			Render::device.MatrixScale(0.8f + 0.06f * math::sin(_localTime * 5.0f));
			timeBombTex->Draw(-FPoint(timeBombTex->getBitmapRect().RightTop()) * 0.5f);
			Render::device.PopMatrix();
			Render::FreeType::BindFont("HeroTooltip");
			Render::PrintString(text_pos, utils::lexical_cast(_time_bomb), 1.0f + 0.05f * math::sin(_localTime * 5.0f), CenterAlign, CenterAlign);
		}
		// Рисуем замок
		if(_lock > 0)
		{
			FPoint lock_pos = pos_ + _chipPos;
			if(batch)
			{
				batch->Draw(lockTex1, Render::ALPHA, FRect(Square::DRAW_RECT).MovedTo(lock_pos), FRect(0.f, 1.f, 0.f, 1.f));
			}else{
				lockTex1->Draw(FRect(Square::DRAW_RECT).MovedTo(lock_pos), FRect(0.f, 1.f, 0.f, 1.f));
			}
			if(_lock == 2)
			{
				if(batch)
				{
					batch->Draw(lockTex2, Render::ALPHA, FRect(Square::DRAW_RECT).MovedTo(lock_pos), FRect(0.f, 1.f, 0.f, 1.f));
				}else{
					lockTex2->Draw(FRect(Square::DRAW_RECT).MovedTo(lock_pos), FRect(0.f, 1.f, 0.f, 1.f));
				}
			}
		}
		Render::EndAlphaMul();
	}

	void ChipColor::DrawOver()
	{
		if(_hang_current && _isOnActiveZone)
		{
			_hang_current->DrawOverChip(_lastChipPos, _localTime, 0);
		}
		_innerEffCont.Draw();
	}

	void ChipColor::DrawWhiteBorder(FPoint pos, bool in_ice)
	{
			Render::device.SetTexturing(false);
			Render::BeginColor(Color::WHITE);
			//Render::device.SetBlendMode(Render::ALPHA);
			{
				Render::device.Bind(Game::ChipColor::chipsTex, 0, Render::STAGE_A_MODULATE/* + Render::STAGE_C_SKIP*/);
				FRect frect = Game::GetChipRect(_value, true, in_ice, true);
				FRect rect = GetDrawRect(_value, in_ice);
				rect.Inflate(int(6*GameSettings::SQUARE_SCALE));
				rect.MoveBy(pos);
				Render::DrawRect(rect, frect);
			}
			//Render::device.SetBlendMode(Render::ALPHA);
			Render::EndColor();
			Render::device.SetTexturing(true);	
	}

	void ChipColor::BreakStarArrow()
	{
		int act = GetStarAct();
		int score(0);
		if( Gadgets::levelSettings.getString("LevelType") == "moves" )
		{
			Match3GUI::ActCounter::AddHiddenCounter(1);
			score = 1;
		}
		else // time
		{
			Match3GUI::TimeCounter::AddHiddenTime(5.0f);
			score = 5;
		}
			
		FPoint from_pos = _lastChipPos - GameSettings::CELL_HALF;

		float pause = 0.f;
		for(int i = 0; i < act; i++)
		{
			StarArrowInfo &info = _starArrowsInfo[utils::lexical_cast(act*10 + i + 1)];
			Match3::FlyStarArrowEffect* controller = new Match3::FlyStarArrowEffect(from_pos, info.pos, info.texture, info.angle, pause);
			controller->AddMessage(Message("ChangeCounter", score), Core::guiManager.getLayer("GameLayer")->getWidget("GameField"));
			Game::AddController(controller);
			pause += 0.4f;
		}
	}

	bool ChipColor::KillChip(const FPoint &center_exploid, const Game::FieldAddress &address_parent_cell, bool it_is_bonus, bool near_match, float pause)
	{
		//Тут можно убивать клетку - назначать ее исчезновение, разлетание эффекты...
		if(_type == LICORICE)
		{
			_type = Game::ChipColor::CHIP;
			if(it_is_bonus)
			{
				ParticleEffectPtr eff = Game::AddEffect(GameField::Get()->_effTopCont, "SpiritDestroy");
				eff->SetPos(_lastChipPos);
				eff->Reset();
			}
			Game::AddController(new FlashAnimationPlayer(Game::ANIM_RESOURCES["doll_kill"], _lastChipPos + FPoint(0.f, 10.f)*GameSettings::SQUARE_SCALE));
			Reset(true);
			Game::Orders::KillCell(Game::Order::LICORICE, address_parent_cell);
			GameField::Get()->AddScore(address_parent_cell, GameSettings::score.licorice);

			MM::manager.PlaySample("ClearDoll");

			return true;
		}
		if(_type == THIEF)
		{
			_thief->Kill();
			Game::Orders::KillCell(Game::Order::MOVING_MONSTER, address_parent_cell); //заказ
			GameField::Get()->AddScore(address_parent_cell, GameSettings::score.thief);

			MM::manager.PlaySample("MovingMonsterKill");

			return true;
		}		
		if(GetLock() > 0 && !near_match)
		{
			_lock--;
			return false;
		} else if(_type == KEY || _type == STAR || _type == TREASURE || _type == DIAMOND) {
			// Эти типы фишек не убиваются матчами или бонусами
			return false;
		}

		if(near_match)
		{
			return false;
		}

		if(_act_count > 0)
		{
			FPoint text_pos = _lastChipPos + FPoint(GameSettings::SQUARE_SIDE/2 - 12.f, GameSettings::SQUARE_SIDE/2 - 12.f);
			FPoint from_pos = GameSettings::ToScreenPos(text_pos.Rounded());
			if( Gadgets::levelSettings.getString("LevelType") == "moves" )
			{
				Match3GUI::ActCounter::AddHiddenCounter(_act_count);
			}
			else // "time"
			{
				Match3GUI::TimeCounter::AddHiddenTime((float)_act_count);
			}
			FPoint to_pos = Core::LuaCallFunction<FPoint>("gameInterfaceCounterPosition");
			Match3GUI::FlyTextBySpline *fly_text = new Match3GUI::FlyTextBySpline(utils::lexical_cast(_act_count), from_pos, to_pos, 1.f, GameField::gameField, true, BACK_ACT);
			fly_text->AddMessage(Message("ChangeCounter", _act_count), Core::guiManager.getLayer("GameLayer")->getWidget("GameField"));
			Game::AddController(fly_text);

			_act_count = 0;
		}
		//Убийство клетки
		float dX = _lastChipPos.x - center_exploid.x;
		float dY = _lastChipPos.y - center_exploid.y;

		bool chip_killed = false;
		int color = GetColor();
		if (color >= 0)
		{
			Game::FieldAddress orderAddress = Game::Orders::ChipIsOrdered(color, GameField::Get()->_chipSeqCopy.size());

			if(!_hangForSpirit.IsEmpty())
			{
				//Если фишка должна родить духа, то запускаем появление духа, с плавным перетеканием его из фишки
				Game::FieldAddress fa(-1,-1);
				if(!GameField::Get()->_tutorialHangBonusSquares.empty()) {
					fa = GameField::Get()->_tutorialHangBonusSquares.front();
					GameField::Get()->_tutorialHangBonusSquares.pop_front();
				}
				Game::AddController(new Game::Spirit(GameSettings::ToScreenPos(_lastChipPos), address_parent_cell.ToPoint(), "screen", _hangForSpirit, GameField::Get()->_chipSeqCopy, fa, pause, GameField::Get()->_chipSeqColor, *this, GameSettings::gamefield[address_parent_cell]->IsIce()));
				_hangForSpirit.Clear();	
				_timerForHideWhiteBack = pause + Game::ChipColor::CHIP_START_HIDE + 0.1f;
			} else {
				if(orderAddress.IsValid()) {
					Game::AddController(new Game::ChipOrderRemover(_lastChipPos, Game::GetCenterPosition(orderAddress.ToPoint()), color, address_parent_cell));
				} else if(it_is_bonus) {
					//Game::AddController(new Game::ChipExplodeRemover(color, address_parent_cell, _lastChipPos, dX, dY, GameField::gameField));
                    Game::AddController(new Game::ChipRemoverByBonus(_lastChipPos, color, 0.0f));
				} else {
					Game::AddController(new Game::ChipRemover(_lastChipPos, color, pause));
				}
			}

			if( it_is_bonus && Gadgets::levelSettings.getString("FILL_ItemType") == "Chips" )
				Match3GUI::FillBonus::Get().ChangeCounter(1);

			Game::Orders::KillCell( Game::Order::Objective(color), address_parent_cell);
			if(_time_bomb > 0) {
				Game::Orders::KillCell( Game::Order::TIME_BOMB, address_parent_cell );
			}

			if( Energy::field.EnergyExists(address_parent_cell) ) {
				GameField::Get()->AddEnergyWave(_lastChipPos, "EnergyWave");
			}

			chip_killed = true;
			MM::manager.PlaySample("ChipDrop" + utils::lexical_cast( math::random(1, 5) ));
		}
		else if(_type == MUSOR)
		{
			Game::AddController(new ChipPiecesFly(_lastChipPos, Game::MUSOR_PIECES_TEX, Color(255, 255, 255), 0.f, math::random(2.f, 5.f)));
			Game::Orders::KillCell( Game::Order::MUSOR, address_parent_cell );
			chip_killed = true;
		}

		//Assert2(_hang.IsEmpty() , "Untriggered bonus on killed chip");
		Reset(false);
		return chip_killed;
	}

	void ChipColor::RunBonus(const Game::FieldAddress &address_parent_cell)
	{
		Game::AddController(new CombinedBonus(address_parent_cell, _hang, GameField::Get(), false));
		_hang.Clear();
		_hang_current = &_hang;
		_hang_current_dir = 0;
	}

	bool ChipColor::HasDrawHang(const Game::FieldAddress &address)
	{
		return _hang_current && !_hang_current->IsEmpty();
	}

	void ChipColor::Update(float dt, const Game::FieldAddress &address, bool isOnScreen, bool isOnActiveZone)
	{
		_isOnActiveZone = isOnActiveZone;
		_innerEffCont.Update(dt);
		_localTime += dt;

		//if(_bounceTimer > 0.0f){
		//	_bounceTimer -= 5.0f * dt;
		//	_offset.y = _bounceAmp * math::sin(_bounceTimer * math::PI);
		//}

		if(IsThief())
		{
			_thief->Update(dt);
			if(_thief->IsKilled())
			{
				Reset(true);
				Match3::RunFallColumn(address.GetCol());
			}
		}
		if(!_fly)
		{
			if (_fallingState == FALL_DOWN)
			{
				if(	_fallingPause > 0)
				{
					_fallingPause -= dt;
				}
				else
				{
					_chipPos -= _velocity * dt;
					_velocity += _acc * dt;
					_timeChipFalling = std::min(_timeChipFalling + 10.0f * dt, 1.0f);

					FPoint origin = _flyPoints.empty() ? FPoint(0,0) : _flyPoints.back();
					bool stop_chip = (_chipPos.y <= origin.y);
					if (stop_chip) {
						_chipPos = origin;
						if( !_flyPoints.empty() ) {
							_flyPoints.pop_back();
							_fallingState = FALL_AFTER;
							RunFall(0.0f, _velocity.Length(), _acc.Length());
						} else {
							_fallingState = FALL_AFTER;
							_timeChipFalling = 0.f;
							if (ChipColor::timeAfterLastFallSound > 0.1f && isOnScreen) {
								ChipColor::timeAfterLastFallSound = 0.f;
								//MM::manager.PlaySample("ChipKnock");
							}
							checkFall = true;
							//_bounceTimer = 1.0f;
							//_bounceAmp = 0.1f * math::sqrt(_velocity.y);
						}
					}
				}
			}
			else if (_fallingState == FALL_MOVE)
			{
				if(	_fallingPause > 0)
				{
					_fallingPause -= dt;
				}
				else
				{
					_chipPos -= _velocity * dt;
					_velocity += _acc * dt;
					_timeChipFalling += 2.0f * dt;
					bool stop_chip = (_chipPos.x * _movingFromPos.x <= 0.0f && _chipPos.y * _movingFromPos.y <= 0.0f);
					//останавливаться когда будет с другой стороны (0,0) от исходной точки
					if (stop_chip) {
						_chipPos = FPoint(0,0);
						_fallingState = FALL_NO;
						_timeChipFalling = 0.f;
					}
				}
			}
			else
			{
				_velocity = FPoint(0, 0);
				if(_fallingState == FALL_AFTER)
				{
					if(_timeChipFalling < 1.0f)
					{
						_timeChipFalling += 3.0f * dt;				
						if(_timeChipFalling >= 1.0f)
						{
							_fallingState = FALL_NO;
							_timeChipFalling = 0.0f;
						}
					}
				}
			}
		}
		if(_selected)
		{
			_blicked = false;
			_blickTime = 0.f;
		}else if (_blicked || _firstHighlighted) {
			_blickTime += dt * (_firstHighlighted ? 1.2f : 1.0f);
			if (_blickTime >= 1.f) {
				if (_firstHighlighted)
					_blickTime -= math::floor(_blickTime);
				else
				{
					_blickTime = 0.f;
					_blicked = false;
				}
			}
		}
		if(_fallingPause <= 0)
		{
			if (_chipAlpha != _chipAlphaTo /*&& _fallingPause <= 0.f*/)
			{
				if (_chipAlpha > _chipAlphaTo) {
					_fChipAlpha -= dt * 6.0f;
				} else {
					_fChipAlpha += dt * 6.0f;
				}
				_fChipAlpha = math::clamp(0.0f, 1.0f, _fChipAlpha);
				_chipAlpha = _fChipAlpha;
			}
		}

		bool to = _playSelected;
		//Апдейт аниации включения в цепочку
		if(to && _playSelectedTime < 1)
		{
			_playSelectedTime += dt*6.f;
			if(_playSelectedTime >= 1.f)
			{
				_playSelectedTime = 1.f;
			}
		}else if(!to && _playSelectedTime > 0)
		{
			_playSelectedTime -= dt*2.f;
			if(_playSelectedTime <= 0)
			{
				_playSelectedTime = 0.f;
			}
		}
		if(_playSelectedTime == 0.f || _playSelectedTime == 1.f)
		{
			_playSelectedForward = _playSelected;
		}
		if(_starActLightTimer < 0)
		{
			_starActLightTimer += dt;
		}else{
			_starActLightTimer += dt*0.7f;
			if(_starActLightTimer >= 1)
			{
				_starActLightTimer = -math::random(5.f, 15.f);
			}
		}


		UpdateDrawHang(address);

		if(_hang_current)
		{
			_hang_current->Update(dt);
		}

		if(_chipAnim)
		{
			_chipAnim->Update(dt);
		}

		for(std::list<boost::intrusive_ptr<ChipDistortion>>::iterator i = _distortions.begin(); i != _distortions.end(); )
		{
			if((*i)->Update(dt)) {
				i = _distortions.erase(i);
			} else {
				i++;
			}
		}
		if(_timerForHideWhiteBack > 0 && _seqBonusEff.get())
		{
			_timerForHideWhiteBack -= dt;
			if(_timerForHideWhiteBack < 0)
			{
				CheckKillSeqEff();
			}
		}
	}

	void ChipColor::UpdateDrawHang(Game::FieldAddress address)
	{
		if( EditorUtils::editor ) {
			_hang_current = &_hang;
			return;
		}

		// ищем, есть ли эта клетка в каскаде бонусов, если да, то нужно отображать навешиваемый бонус из каскада, а не собственный
		// если фишка в выделенной цепочке, то в любом случае рисуется только каскадный бонус, и никогда собственный
		_hang_current = ((_selected > 0) && GameField::Get()->CanHighlightCurrentSequence()) ? NULL : &_hang;
		for(size_t i = 0; i < GameField::Get()->_bonusCascade.size(); ++i)
		{
			if(GameField::Get()->_bonusCascade[i].first == address) {
				_hang_current = &GameField::Get()->_bonusCascade[i].second;
			}
		}

		if(_hang_current)
		{
			ArrowBonus *arr = (ArrowBonus*) _hang_current->GetBonusByType(Game::HangBonus::ARROW);
			BYTE next_dir = arr ? arr->GetDirections() : 0;

			// проверка для того, чтобы стрелка не вертелась уже в момент уничтожения цепочки
			if( GameSettings::gamefield[address]->ToBeDestroyed() ) {
				next_dir = _hang_current_dir;
				if( arr )
					arr->SetDirections(next_dir);
			}

			if(_hang_current_dir && next_dir && _hang_current_dir != next_dir)
			{
				arr->StartRotateAnim(90.0f);
			}
			_hang_current_dir = next_dir;
		}
		else
		{
			_hang_current_dir = 0;
		}
	}

	void ChipColor::Select(int seqNumber, IPoint index, bool prev_exist, IPoint index_prev)
	{
		_selected = seqNumber;

		//_effInSelection = Game::AddEffect(GameField::Get()->_effCont, "ChipInSeq");
		//_effInSelection->SetPos(FPoint(index)*GameSettings::SQUARE_SIDEF);
		//_effInSelection->Reset();

		if(_chipInSequence)
		{
			_chipInSequence->Hide();
			_chipInSequence = 0;
		}
		_chipInSequence = new ChipInSequence(index, prev_exist, index_prev);
		Game::AddController(_chipInSequence);

		bool needEffect = false;
		std::string prefix = (Gadgets::levelSettings.getBool("FILL_bonus_allow") && Gadgets::levelSettings.getString("FILL_ItemType") == "Fires") ? "LSeq" : "Seq";

		if( Gadgets::levelSettings.getString("BonusRepeat") == "true" )
		{
			// при этой настройке мы вешаем бонус каждые N фишек
			int period = Gadgets::levelSettings.getInt(prefix + "0");
			needEffect = (_selected > 0) && (_selected % period == 0);
		}
		else
		{
			// при этой настройке вешаем до трех бонусов на заданные позиции в цепочке
			for(size_t i = 0; (i < 3) && !needEffect; i++)
			{
				int seq_size = Gadgets::levelSettings.getInt(prefix + utils::lexical_cast(i));
				if(_selected == seq_size)
					needEffect = true;
			}
		}

		if(needEffect && !_seqBonusEff)
		{
			_seqBonusEff = Game::AddEffect(GameField::Get()->_effContUpField, "BonusSequence");
			FPoint pos = FPoint(index)*GameSettings::SQUARE_SIDEF + GetPos() + GameSettings::CELL_HALF + FPoint(0.f, 10.f);
			_seqBonusEff->SetPos(pos);
			_seqBonusEff->Reset();

			_seqBonusEffDown = Game::AddEffect(GameField::Get()->_effUnderChipsCont, "BonusSequenceDown");
			_seqBonusEffDown->SetPos(pos);
			_seqBonusEffDown->Reset();

			MM::manager.PlaySample("SpiritFromChip");
		}
		else if(!needEffect)
		{
			CheckKillSeqEff();
		}
	}

	bool ChipColor::IsFutureHang() const
	{
		return _seqBonusEff.get() != nullptr;
	}

	void ChipColor::CheckKillSeqEff()
	{
		if(_hangForSpirit.IsEmpty() && _timerForHideWhiteBack <= 0)
		{
			//Эффект бонуса нужно удалять позже - в момент рождения духа "new Game::Spirit"
			if(_seqBonusEff.get())
			{
				_seqBonusEff->Kill();
			}

			if(_seqBonusEffDown.get())
			{
				_seqBonusEffDown->Kill();
			}

			_seqBonusEffDown.reset();
			_seqBonusEff.reset();
		}
	}

	void ChipColor::Deselect()
	{
		_selected = 0;
		_timerForHideWhiteBack = -1.f;

		if(_chipInSequence)
		{
			_chipInSequence->Hide();
			_chipInSequence = NULL;
		}

		CheckKillSeqEff();

		//if(_effInSelection.get()){
		//	_effInSelection->Kill();
		//}
		//_effInSelection.reset();
	}

	bool ChipColor::IsProcessedInSequence()
	{
		return _inSequence == CURRENT_IN_SEQUENCE_ITER;
	}

	void ChipColor::UpdateInSequence()
	{
		_inSequence = CURRENT_IN_SEQUENCE_ITER;
	}

	void ChipColor::ResetColor()
	{
		_starAct = 0;
		_starActLightTimer = -math::random(5.f, 15.f);
		_act_count = 0;
		_time_bomb = 0;
		_lock = 0;
		_type = CHIP;
		_thief.reset();
		_isAdapt = false;
		_value = -1;
		_isChameleon = false;
		_musor_index = 0;
		_mirror = false;
		_light = false;
		_preinstalled = false;
		
		_innerEffCont.KillAllEffects();
		//_effInSelection = NULL;

		_chipAnim.reset();
	}

	void ChipColor::Reset(bool reset_hard)
	{
		_isOnActiveZone = false;

		_selected = 0;
		_inSequence = -1;

		_tutorialHighlight = false;

		_hang.Clear();

		_hang_current = &_hang;
		_hang_current_dir = 0;

		ResetColor();

		_blicked = false;
		_blickTime = 0.f;

		_firstHighlighted = false;
		_playSelected = _playSelectedForward = false;
		_playSelectedTime = 0.f;

		_chipAlpha = 1;
		_fChipAlpha = 1;
		_chipAlphaTo = 1;
		_fallingPause = 0.f;

		_fallingState = FALL_NO;
		_fly = false;

		_acc = FPoint(0,0);
		_velocity = FPoint(0,0);
		//_bounceTimer = 0.0f;

		_chipPos = FPoint(0,0);
		ClearOffset();

		checkFall = false;

		_lastChipPos = FPoint(-1.f, -1.f);
		_localTime = 0.f;

		_preinstalled = false;
		if(reset_hard)
		{
			_timerForHideWhiteBack = -1.f;
			_hangForSpirit.Clear();
			CheckKillSeqEff();
		}
		if(_chipInSequence)
		{
			_chipInSequence->Hide();
			_chipInSequence = 0;
		}
		_distortions.clear();
	}

	void ChipColor::SetMusor(bool musor)
	{
		ResetColor();
		if(musor) {
			_type = MUSOR;
			_mirror = math::random(0, 99) < 50;
		}
	}

	bool ChipColor::IsMusor() const
	{
		return _type == MUSOR;
	}

	bool ChipColor::IsKey() const
	{
		return _type == KEY;
	}

	bool ChipColor::IsSimpleChip() const
	{
		if(IsEnergyBonus())
		{
			return false;
		}
		if(IsAdapt()) //true!!! это фишка!
		{
			return true;
		}
		return (_type == CHIP) && (0 <= _value) && (_value < 25);
	}

	bool ChipColor::IsExist() const 
	{
		return (_type != CHIP) || IsAdapt() || IsEnergyBonus() || GetColor() >= 0;
	}

	bool ChipColor::IsAdapt() const
	{
		return _isAdapt;
	}

	void ChipColor::SetAdapt(bool adapt)
	{
		_isAdapt = adapt;
	}

	bool ChipColor::IsChameleon() const 
	{
		return _isChameleon;
	}

	void ChipColor::SetChameleon(bool isChameleon)
	{
		_isChameleon = isChameleon;
	}

	int ChipColor::GetColor() const
	{
		return _isAdapt ? COLOR_FOR_ADAPT : _value;
	}

	bool ChipColor::IsColor(const int &value) const
	{
		return value == GetColor();
	}

	// Первая подcвеченная иконка в цепочке
	void ChipColor::setFirstHighlight(const bool &highlighted)
	{ 
		_firstHighlighted = highlighted;
	}

	void ChipColor::PlaySelected(bool selected)
	{
		_playSelected = selected;
		//_playSelectedTime = 0.f;
	}

	void ChipColor::HighlightChain()
	{
		_tutorialHighlight = true;
	}

	bool ChipColor::IsChainHighlighted() const
	{
		return _tutorialHighlight;
	}

	void ChipColor::RunFall(float pause, float velocity, float acceleration)
	{
		Assert(!IsStarAct());
		if (_fallingState != FALL_DOWN) //Из любого состояния можно начать падать кроме FALL_DOWN
		{
			FPoint origin = _flyPoints.empty() ? FPoint(0,0) : _flyPoints.back();
			FPoint n = (_chipPos - origin).Normalized();

			_fallingState = FALL_DOWN ;
			_fallingPause = pause;
			_timeChipFalling = 0.f;

			// если скорость ненулевая, то это цепочка падений (при скатывании по диагонали например)
			// и нужно сохранить набранную скорость
			if( _velocity != FPoint(0,0) )
				velocity = _velocity.Length();

			_velocity = n * velocity;
			_acc = n * acceleration;
		}
	}

	void ChipColor::AddFlyWaypoint(FPoint pos)
	{
		_flyPoints.push_back(pos);
	}

	void ChipColor::RunMove(float pause, float velocity, float acceleration)
	{
		MyAssert(!IsStarAct());
		if (_fallingState != FALL_MOVE) //Из любого состояния можно начать падать кроме FALL_DOWN
		{
			_movingFromPos = _chipPos;

			FPoint n = _chipPos.Normalized();
			_fallingState = FALL_MOVE;
			_fallingPause = pause;
			_timeChipFalling = 0.f;

			_velocity = n * velocity;
			_acc = n * acceleration;
		}
	}

	void ChipColor::ClearOffset()
	{
		SetOffset(0.0f, 0.0f);
		SetScale(1.0f, 1.0f);
	}

	void ChipColor::SetOffset(float x, float y)
	{
		_offset = FPoint(x,y);
	}

	void ChipColor::SetScale(float scale_x, float scale_y)
	{
		_scale = FPoint(scale_x, scale_y);
	}

	bool ChipColor::IsFly() const
	{
		return _fly;
	}

	bool ChipColor::IsStand() const
	{
		return !(IsFly() || IsFalling());
	}

	bool ChipColor::IsFalling() const
	{
		return _fallingState == FALL_DOWN;
	}

	void ChipColor::SetFly()
	{
		_fly = true;
		_fallingState = FALL_DOWN;
	}

	void ChipColor::ResetFly()
	{
		_fly = false;
		_fallingState = FALL_NO;
	}

	void ChipColor::SetPos(FPoint pos)
	{
		_chipPos = pos;
		ClearOffset();
	}

	void ChipColor::SetAlpha(float fChipAlpha, float chipAlpha, float chipAlphaTo)
	{
		_fChipAlpha = fChipAlpha;
		_chipAlpha = chipAlpha;
		_chipAlphaTo = chipAlphaTo;
	}

	void ChipColor::SetColor(int color)
	{
		bool chameleon = _isChameleon;
		ResetColor();
		_isChameleon = chameleon;
		_value = color;
	}

	void ChipColor::SetHang(const Hang &hang)
	{
		_hang = hang;
		_distortions.push_back(boost::intrusive_ptr<ChipJumpOnHang>(new ChipJumpOnHang(0.f, 1.f)));
		if(hang.transformChip == Hang::ENERGY_BONUS)
		{
			_value = -1;
			_isAdapt = false;
		}
		else
		{
			LightningBonus *lightning = (LightningBonus*)_hang.GetBonusByType(HangBonus::LIGHTNING);
			if(_isAdapt && (hang.transformChip != Hang::CHAMELEON) && !lightning)
			{
				_isAdapt = false;
				_value = Gadgets::levelColors.GetRandom();
			}
			_isAdapt = (hang.transformChip == Hang::CHAMELEON) || lightning;
		}
	}

	bool ChipColor::HasHang() const
	{
		return !_hang.IsEmpty();
	}

	Hang& ChipColor::GetHang()
	{
		return _hang;
	}

	void ChipColor::SetLicorice()
	{
		ResetColor();
		_type = LICORICE;
	}

	void ChipColor::SetThief(IPoint index)
	{
		Assert(!_thief);
		ResetColor();
		_type = THIEF;
		_thief.reset(new Thief(index));
	}

	Thief::HardPtr ChipColor::GetThief()
	{
		return _thief;
	}

	bool ChipColor::IsThief() const
	{
		return _type == THIEF;
	}

	void ChipColor::SetLight()
	{
		_light = IsSimpleChip();
	}

	bool ChipColor::IsLight() const
	{
		return _light;
	}

	void ChipColor::SetTimeBomb(int moves)
	{
		_time_bomb = moves;
	}

	bool ChipColor::IsTimeBomb() const
	{
		return _time_bomb > 0;
	}

	void ChipColor::SetInfo(const Gadgets::InfoC &info, IPoint index_parent_cell)
	{
		ResetColor();

		_type = info.type;

		if(_type == THIEF )
		{
			_thief.reset(new Thief(index_parent_cell));
			_thief->Appear();
		}
		else if(_type == TREASURE )
		{
			_treasureLevel = info.treasure;
		}
		else if(_type == STAR)
		{
			_starAct = info.star;
			_preinstalled = true;
		}
		else if(_type == KEY)
		{
		}
		else if(_type == DIAMOND)
		{
			_chipAnim = Render::StreamingAnimation::Spawn("CrystalRotate");
			_chipAnim->SetPlayback(true);
			_chipAnim->SetLoop(true);
		}
		else if(_type == LICORICE)
		{
			_preinstalled = true;
		}
		else if(_type == CHIP)
		{
			_act_count = info.act_count;
			_time_bomb = info.time_bomb;
			_lock = info.lock;
			_isAdapt = info.adapt;
			_hang = info.level_hang;
			_isChameleon = info.pre_chameleon;

			if(info.pre_chip >= 0) {
				int pre_chip = (Gadgets::levelColors.GetCountFull() > info.pre_chip) ? info.pre_chip : 0;
				_value = Gadgets::levelColors[pre_chip];
				_preinstalled = true;
			} else if(info.pre_chip == -2 && !EditorUtils::editor){
				_value = Gadgets::levelColors.GetRandom();
				_preinstalled = true;
			} else {
				_value = -1;
			}
			_preinstalled |= (_time_bomb > 0);
			_preinstalled |= (_act_count > 0);
		}
	}

	bool ChipColor::IsLock() const
	{
		return _lock > 0;
	}

	int ChipColor::GetLock() const
	{
		return _lock;
	}

	bool ChipColor::IsLicorice() const
	{
		return _type == LICORICE;
	}

	//есть ли навешенная бомба
	bool ChipColor::HasHangBomb() const
	{
		return (_hang.GetBonusByType(Game::HangBonus::BOMB) != NULL);
	}

	FPoint ChipColor::GetPos() const
	{
		return _chipPos + _offset;
	}

	float ChipColor::GetFallingPause() const
	{
		return _fallingPause;
	}

	void ChipColor::MoveBy(FPoint offset)
	{
		_chipPos = _chipPos + offset;
	}

	void ChipColor::CheckBlick()
	{
		if(!_blicked && !_selected)
		{
			_blicked = true;
			_blickTime = 0.f;
		}
	}

	bool ChipColor::IsStarAct() const
	{
		return (_type == STAR) && (_starAct > 0);
	}

	int ChipColor::GetStarAct() const
	{
		return _starAct;
	}

	bool ChipColor::IsPreinstalled() const
	{
		return _preinstalled;
	}

	void ChipColor::OnEndMove(bool onScreen)
	{
		if(onScreen && _time_bomb > 0 && !GameField::Get()->_endLevel.IsRunning())
		{
			_time_bomb--;
			if(_time_bomb == 0)
			{
				// GAME OVER!
				GameField::Get()->ClearChipSeq(false);
				GameField::Get()->_endLevel.SetLoseLifeReason(LevelEnd::TIME_BOMB);
			}
		}
	}

	bool ChipColor::IsEnergyBonus() const
	{
		return (_hang.transformChip == Hang::ENERGY_BONUS);
	}

	void ChipColor::SetSpirit(Game::Hang hang)
	{
		_hangForSpirit = hang;
	}

	int ChipColor::GetTreasure() const
	{
		return (_type == TREASURE) ? _treasureLevel : 0;
	}

	void ChipColor::SetDiamond()
	{
		ResetColor();
		_type = Game::ChipColor::DIAMOND;

		_chipAnim = Render::StreamingAnimation::Spawn("CrystalRotate");
		_chipAnim->SetPlayback(true);
		_chipAnim->SetLoop(true);
	}

	void ChipColor::AddDistortion(boost::intrusive_ptr<ChipDistortion> dist)
	{
		_distortions.push_back(dist);
	}

} // namespace Game