#include "stdafx.h"

#include "GameBonuses.h"
#include "Match3Gadgets.h"
#include "GameInfo.h"
#include "GameLightningController.h"
#include "GameField.h"
#include "ArrowEffects.h"

namespace Game
{

	Render::Texture* arrowTex = NULL;
	Render::Texture* hang_bomb_tex[3];
	Render::Texture* hang_lightning_tex = NULL;
	Render::Texture* arrowBack = NULL;
	Render::Texture *arrowFront = NULL;

	void InitHangBonusTextures()
	{
		arrowTex = Core::resourceManager.Get<Render::Texture>("Arrows");
		arrowBack = Core::resourceManager.Get<Render::Texture>("arrow_back");
		arrowFront = Core::resourceManager.Get<Render::Texture>("arrow_front");
		hang_bomb_tex[0] = Core::resourceManager.Get<Render::Texture>("Boost_icon4");
		hang_bomb_tex[1] = Core::resourceManager.Get<Render::Texture>("Boost_icon5");
		hang_bomb_tex[2] = Core::resourceManager.Get<Render::Texture>("Boost_icon6");
		hang_lightning_tex = Core::resourceManager.Get<Render::Texture>("Boost_icon8");
	}

	float ArrowBonus::DELAY_FOR_CHIP_AFTER_ARROW = 0.f;
	float ArrowBonus::DELAY_FOR_RUN_ARROW_BONUS = 0.f;

	std::map<int, std::vector<float> > arrowOffsets;

	void ArrowBonus::InitGame(rapidxml::xml_node<> *xml_info)
	{
		rapidxml::xml_node<> *xml_arrows_info = xml_info->first_node("ChipArrowsInfo");
		rapidxml::xml_node<> *xml_arrow_info = xml_arrows_info->first_node("chip");
		while(xml_arrow_info)
		{
			int id = Xml::GetIntAttribute(xml_arrow_info, "n");
			for(size_t i = 0; i < 8; i++)
			{
				arrowOffsets[id].push_back(Xml::GetFloatAttributeOrDef(xml_arrow_info, "dir" + utils::lexical_cast(i), 21.f));
			}
			xml_arrow_info = xml_arrow_info->next_sibling("chip");
		}
		InitArrowEffects(xml_info);
		ArrowBonus::DELAY_FOR_CHIP_AFTER_ARROW = gameInfo.getConstFloat("DELAY_FOR_CHIP_AFTER_ARROW");
		ArrowBonus::DELAY_FOR_RUN_ARROW_BONUS = gameInfo.getConstFloat("DELAY_FOR_RUN_ARROW_BONUS");
	}

ArrowBonus::ArrowBonus(BYTE dir, int radius, int level)
	: _dirs(dir)
	, _radius(radius)
	, _level(level)
	, _color_id(-1)
	, _angleOffset(0.0f)
{
	if(_level < 2) // обычная стрела
	{
		_arrows.push_back(Arrow(_radius, 0, 0));
	}
	else if(_level == 2) // стрелка шириной 3
	{
		_arrows.push_back(Arrow(_radius, 0, 0));
		_arrows.push_back(Arrow(_radius-1, 1, 1));
		_arrows.push_back(Arrow(_radius-1, -1, 1));
		_arrows.push_back(Arrow(_radius-1, -1, -1));
		_arrows.push_back(Arrow(_radius-1, 1, -1));
	}
	else if(_level >= 3) // весь экран в стрелах
	{
		_dirs = Game::Hang::ARROW_L | Game::Hang::ARROW_R;
		_radius = 9;
		_arrows.push_back(Arrow(9, -1, 0));
		_arrows.push_back(Arrow(9, -1, 2));
		_arrows.push_back(Arrow(9, -1, 4));
		_arrows.push_back(Arrow(9, -1, 6));
		_arrows.push_back(Arrow(9, -1, 8));
		_arrows.push_back(Arrow(9, -1, 10));
		_arrows.push_back(Arrow(9, 8, 1));
		_arrows.push_back(Arrow(9, 8, 3));
		_arrows.push_back(Arrow(9, 8, 5));
		_arrows.push_back(Arrow(9, 8, 7));
		_arrows.push_back(Arrow(9, 8, 9));
	}
}

static bool SquareLimitsArrow(Game::Square *sq)
{
	return sq->IsStone() || sq->GetChip().IsMusor() || sq->GetWood() > 0;
}

void ArrowBonus::GetArrowAffectedChips(Arrow &arrow, int radius, Game::FieldAddress cell, AffectedArea &chips, float startTime)
{
	const int dx[] = {1,  1,  0, -1, -1, -1,  0,  1};
	const int dy[] = {0,  1,  1,  1,  0, -1, -1, -1};

	bool limitArrowSquare = (Gadgets::levelSettings.getString("LimitArrowsSquares") == "true");
	bool limitArrowLicorice = (Gadgets::levelSettings.getString("LimitArrowsLicorice") == "true");

	ClearCellInfo info(0.0f, false);

	cell = cell.Shift(arrow.x, arrow.y);

	for(size_t i = 0; i < 8; i++)
	{
		if((_dirs & (1 << i)) > 0)
		{
			// просчитываем стрелу в одном из направлений

			// ищем начальную клетку
			int rb = 0;
			while( rb <= radius && !Game::activeRect.Contains( cell.Shift(dx[i]*rb, dy[i]*rb).ToPoint() ) )
				++rb;

			bool interrupted = false;
			int r1 = rb;
			int r2 = rb;
			while(r1 <= radius)
			{
				int r = r1 + 1;
				Game::FieldAddress fa = cell.Shift(dx[i]*r, dy[i]*r);
				Game::Square *sq = GameSettings::gamefield[fa];

				if( !Game::activeRect.Contains(fa.ToPoint()) )
					break;

				r1 = r;

				if( Game::isVisible(sq) )
					r2 = r1;

				// убив лакрицу/мусор/шоколад/плиту стрела дальше не летит (если включена соответствующая настройка)
				if( (_level < 3) && ( (limitArrowSquare && SquareLimitsArrow(sq)) || (limitArrowLicorice && sq->GetChip().IsLicorice()) ) ) {
					interrupted = true;
					break;
				}
			}
			r1 = std::min(r1, r2);

			for(int r = rb; r <= r1; ++r)
			{
				Game::FieldAddress fa = cell.Shift(dx[i]*r, dy[i]*r);
				info.delay = math::clamp(0.f, 10000.f, startTime + float(r) / ARROW_BONUS_SPEED + Game::ArrowBonus::DELAY_FOR_CHIP_AFTER_ARROW);
				HangBonus::AddChip(fa, info, chips);
			}
			arrow.radius[i] = interrupted ? r1 : radius;
		}
	}
}

void ArrowBonus::GetAffectedChips(Game::FieldAddress cell, AffectedArea &chips, float startTime)
{
	if(_level >= 3)
	{
		cell = Game::FieldAddress(Game::activeRect.LeftBottom());
	}

	for( size_t i = 0; i < _arrows.size(); ++i ) {
		int r = (i == 0) ? _radius : _radius-1;
		GetArrowAffectedChips(_arrows[i], r, cell, chips, startTime);
	}
}

void ArrowBonus::StartArrowEffect(Arrow &arrow, IPoint pos, ArrowBonusController *c)
{
	pos.x += arrow.x;
	pos.y += arrow.y;

	if(_dirs & Game::Hang::ARROW_R)
		c->AddArrow(pos, IPoint(1,0), arrow.radius[0], 0);
	if(_dirs & Game::Hang::ARROW_UR)
		c->AddArrow(pos, IPoint(1,1), arrow.radius[1], 1);
	if(_dirs & Game::Hang::ARROW_U)
		c->AddArrow(pos, IPoint(0,1), arrow.radius[2], 2);
	if(_dirs & Game::Hang::ARROW_UL)
		c->AddArrow(pos, IPoint(-1,1), arrow.radius[3], 3);
	if(_dirs & Game::Hang::ARROW_L)
		c->AddArrow(pos, IPoint(-1,0), arrow.radius[4], 4);
	if(_dirs & Game::Hang::ARROW_DL)
		c->AddArrow(pos, IPoint(-1,-1), arrow.radius[5], 5);
	if(_dirs & Game::Hang::ARROW_D)
		c->AddArrow(pos, IPoint(0,-1), arrow.radius[6], 6);
	if(_dirs & Game::Hang::ARROW_DR)
		c->AddArrow(pos, IPoint(1,-1), arrow.radius[7], 7);
}

int DirsToHangArrowType(BYTE dirs)
{
	if((dirs ^ Game::Hang::ARROW_4) == 0) {
		return 3;
	}
	if((dirs ^ (Game::Hang::ARROW_R + Game::Hang::ARROW_L)) == 0) {
		return 1;
	}
	if((dirs ^ (Game::Hang::ARROW_U + Game::Hang::ARROW_D)) == 0) {
		return 2;
	}
	return 0;
}

void ArrowBonus::StartEffect(Game::FieldAddress from, float startTime)
{
	FPoint pos_bonus = FPoint(from.ToPoint())*GameSettings::SQUARE_SIDEF + GameSettings::CELL_HALF;
	ArrowBonusController *c = new ArrowBonusController(startTime + Game::ArrowBonus::DELAY_FOR_RUN_ARROW_BONUS, pos_bonus, DirsToHangArrowType(_dirs));

	IPoint pos = (_level < 3) ? from.ToPoint() : Game::activeRect.LeftBottom();
	for(size_t i = 0; i < _arrows.size(); ++i)
	{
		StartArrowEffect(_arrows[i], pos, c);
	}

	if(_level < 3) {
		FPoint wavePos = FPoint(pos) * GameSettings::SQUARE_SIDEF + GameSettings::CELL_HALF;
		GameField::Get()->AddEnergyArrowWave(wavePos, _dirs);
	}

	Game::AddController(c);

	MM::manager.PlaySample("ArrowBonusStart");
}

void ArrowBonus::DrawArrows(FPoint chipPos, float localTime, Render::SpriteBatch *batch, bool front)
{
	const float angles[8] = {0.f, 45.0f, 90.f, 135.0f, 180.f, 225.0f, 270.f, 315.f};

	//Рисуем стрелы
	
	Render::device.PushMatrix();
	Render::device.MatrixTranslate(chipPos + FPoint(0.f, 18.f + Game::ChipColor::YOffset_Chip));
	Game::MatrixSquareScale();
	float t = math::sin(localTime * 5.0f);

	FRect uv_rect_0(0.f, 0.25f, 0.f, 1.f);
	for(size_t i = 0; i < 8; i++)
	{
		BYTE check = 1 << i;
		if( (check & _dirs) != 0)
		{
			float angle = angles[i] + _angleOffset;
			if(angle < 0)
			{
				angle += 360.f;
			}
			if( (10.f < angle && angle < 170.f) == front)
			{
				continue;
			}
			FRect uv_rect = uv_rect_0.MovedTo(FPoint(i/2.f*0.25f, 0.f)); //Выбираем нужную нам стрелу в зависимости от направления. Стрелы отличаются - есть тенюшки, блики, свечения.
			//В этом направлении есть стрела			
			Render::device.PushMatrix();
			Render::device.MatrixRotate(math::Vector3::UnitZ, angle);
			
			float width = 90.f + 10.f*t;			
			float height = 128.f + 10.f*t;

			//Масштабирвуем
			if( ((i/2) % 2) == 0)
			{
				height *= math::lerp(1.f, 1.f/0.6f, abs(_angleOffset/90.f));
			}else{
				height *= math::lerp(1.f, 0.6f, abs(_angleOffset/90.f));
			}
			FRect rect(-width/2.f, width/2.f, -height/2.f, height/2.f);
			//Перемещаем
			rect.MoveBy(FPoint(40.f, 0.f).Rotated(angle*math::PI/180.f).Scaled(1.f, 0.75f).Rotated(-angle*math::PI/180.f));

			arrowTex->Draw(rect, uv_rect);
			Render::device.PopMatrix();
		}
	}
	Render::device.PopMatrix();
}

void ArrowBonus::PushMatrix(FPoint chipPos)
{
	Render::device.PushMatrix();
	Render::device.MatrixTranslate(chipPos + FPoint(0.f, 18.f + Game::ChipColor::YOffset_Chip)-GameSettings::CELL_HALF);
    //Render::device.MatrixScale(1.f, 0.6f, 1.f);
	Game::MatrixSquareScale();


	//Render::device.MatrixTranslate(GameSettings::CELL_HALF);
	//Тут можно масштабировать крутить
	//Render::device.MatrixTranslate(-GameSettings::CELL_HALF);
}

void ArrowBonus::PopMatrix()
{
	Render::device.PopMatrix();
}

void ArrowBonus::DrawOnChip(FPoint chipPos, float localTime, Render::SpriteBatch *batch)
{
    
	PushMatrix(chipPos);
	//Ректы взяты оносительно начала клетки как в PSD
	arrowBack->Draw(IRect(-12,37, 100,39), FRect(0.f, 1.f, 0.f, 1.f));
	PopMatrix();
	DrawArrows(chipPos, localTime, batch, false);
}


void ArrowBonus::DrawOverChip(FPoint chipPos, float localTime, Render::SpriteBatch *batch)
{
	PushMatrix(chipPos);
	//Ректы взяты оносительно начала клетки как в PSD
	arrowFront->Draw(IRect(-12,-2, 102,59), FRect(0.f, 1.f, 0.f, 1.f));
	PopMatrix();
	DrawArrows(chipPos, localTime, batch, true);
}

void ArrowBonus::DrawOnSquare(FPoint chipPos, float localTime, Render::SpriteBatch *batch)
{
	FRect f_rect = Game::GetChipRect(Game::SNOW, true, false, false);
	FRect draw_rect = ChipColor::DRAW_FRECT.MovedBy(chipPos - ChipColor::DRAW_FRECT.RightTop() * 0.5f);
	if(batch)
	{
		batch->Draw(ChipColor::chipsTex, Render::ALPHA, draw_rect, f_rect);
	}else{
		ChipColor::chipsTex->Draw(draw_rect, f_rect);
	}
	DrawOnChip(chipPos, localTime, batch);
}

void ArrowBonus::Save(Xml::TiXmlElement *xml_hang)
{
	//по алфавиту!
	xml_hang->SetAttribute("ar", _radius);
	xml_hang->SetAttribute("dirs", _dirs);
}

void ArrowBonus::Update(float dt)
{
	HangBonus::Update(dt);

	if(_angleOffset != 0.0f)
	{
		float da = -360.0f * math::sign(_angleOffset) * dt;
		if( math::abs(da) >= math::abs(_angleOffset) )
			_angleOffset = 0.0f;
		else
			_angleOffset += da;
	}
}

void ArrowBonus::StartRotateAnim(float fromAngle)
{
	_angleOffset = fromAngle;
}

/******************************************************************************************************/

BombBonus::BombBonus(int radius, int level)
	: _radius(radius)
	, _level(level)
{
}

void BombBonus::GetAffectedChips(Game::FieldAddress cell, AffectedArea &chips, float startTime)
{
	bool squareShape = (Gadgets::levelSettings.getString("BombShape") == "square");

	ClearCellInfo info(startTime + 0.1f, false);

	for (int i = -_radius; i <= _radius; i++)
	{
		for (int j = _radius; j >= -_radius; j--)
		{
			Game::FieldAddress fa = cell + Game::FieldAddress(i, j);
			Game::Square *sq = GameSettings::gamefield[fa];			
			if( /*Game::isVisible(sq) &&*/
			       (squareShape || Game::CheckContainInRadius((float)_radius, cell.ToPoint(), fa.ToPoint(), sq)) 
				&& Game::activeRect.Contains(fa.ToPoint()))
			{
				HangBonus::AddChip(fa, info, chips);
			}
		}
	}
}

void BombBonus::StartEffect(Game::FieldAddress from, float startTime)
{
	Game::AddController(new BombBonusController(_level, _radius, from.ToPoint(), startTime));
}

void BombBonus::Save(Xml::TiXmlElement *xml_hang)
{
	//по алфавиту!
	xml_hang->SetAttribute("blevel", _level);
	xml_hang->SetAttribute("br", _radius);
}

void BombBonus::DrawOnChip(FPoint chipPos, float localTime, Render::SpriteBatch *batch)
{
	int tex_id = math::clamp(0, 2, _level);
	float t = math::sin(localTime * 6.0f);
	float scale = GameSettings::SQUARE_SIDEF / 44.0f;
	Render::device.PushMatrix();
	Render::device.MatrixTranslate(chipPos + FPoint(7.0f, 7.0f) * scale);
	Game::MatrixSquareScale();
	Render::device.MatrixScale(scale * (0.5f + t * 0.03f));
	FPoint offset = hang_bomb_tex[tex_id]->getBitmapRect().RightTop()/2.f;
	if(batch)
	{
		batch->Draw(hang_bomb_tex[tex_id], Render::ALPHA, -offset);
	}else{
		hang_bomb_tex[tex_id]->Draw(-offset);
	}
	Render::device.PopMatrix();
}

void BombBonus::DrawOnSquare(FPoint chipPos, float localTime, Render::SpriteBatch *batch)
{
	int tex_id = math::clamp(0, 2, _level);
	float t = math::sin(localTime * 5.0f);
	float scale = GameSettings::SQUARE_SIDEF / 44.0f;
	Render::device.PushMatrix();
	Render::device.MatrixTranslate(chipPos);
	Render::device.MatrixScale(scale * (0.7f + t * 0.02f));
	FPoint offset = hang_bomb_tex[tex_id]->getBitmapRect().RightTop()/2.f;
	if(batch)
	{
		batch->Draw(hang_bomb_tex[tex_id], Render::ALPHA, -offset);
	}else{
		hang_bomb_tex[tex_id]->Draw(-offset);
	}
	Render::device.PopMatrix();
}

BombBonusController::BombBonusController(int level, int radius, IPoint center, float startTime)
	: GameFieldController("BombBonus", 1.0f, GameField::Get())
	, _center(center)
	, _level(level)
	, _radius(radius)
{
	local_time = -startTime;
}

void BombBonusController::Update(float dt)
{
	local_time += dt;
	if( local_time >= 0.0f )
	{
//		std::string soundName = std::string("BombExplode") + utils::lexical_cast(_level);
		std::string effName = std::string("BombBoom") + utils::lexical_cast(_level);

//		MM::manager.PlaySample(soundName);

		Game::FieldAddress address(_center);
		ParticleEffectPtr eff = Game::AddEffect(gameField->_effTopCont, effName);
		eff->SetPos(GameSettings::gamefield[address]->GetCellPos() + GameSettings::CELL_HALF);
		eff->Reset();

		gameField->addWave(IPoint((int)eff->posX, (int)eff->posY), 0.03f, 1000.0f, 125.0f * _radius);
	}
}

void BombBonusController::Draw()
{
}

bool BombBonusController::isFinish()
{
	return (local_time > 0.0f);
}

/******************************************************************************************************/

LightningBonus::LightningBonus(ColorMask colors)
		: _colors(colors)
{
}

LightningBonus::LightningBonus(const LightningBonus &other)
	: _colors(other._colors)
{
}

LightningBonus::~LightningBonus()
{
	for(std::vector<GameLightningController*>::iterator itr = _controllers.begin(); itr != _controllers.end(); ++itr){
		delete *itr;
	}
}

LightningBonus& LightningBonus::operator= (const LightningBonus &other)
{
	// не копируем указатели, только цвет
	_colors = other._colors;
	return *this;
}

void LightningBonus::GetAffectedChips(Game::FieldAddress from, AffectedArea &chips, float startTime)
{
	bool improved = (Gadgets::levelSettings.getString("ImprovedLightning") == "true");

	for(std::vector<GameLightningController*>::iterator itr = _controllers.begin(); itr != _controllers.end(); ++itr){
		delete *itr;
	}
	_controllers.clear();
	int seqColor = (Game::ChipColor::COLOR_FOR_ADAPT > 0) ? Game::ChipColor::COLOR_FOR_ADAPT : Gadgets::levelColors.GetRandom();

	ClearCellInfo info(0.0f, true);

	for(int col = 0; col < 25; ++col)
	{
		if( ColorInMask(_colors | ColorToMask(seqColor), col) )
		{
			GameLightningController *controller = new GameLightningController(from.ToPoint(), col, startTime);
			_controllers.push_back(controller);
			std::vector<Game::Square*> *csquares = controller->GetSquares();
			size_t count = csquares->size();
			for(size_t i = 0; i < count; i++)
			{
				info.delay = startTime + i/(count + 1.f) * controller->GetTimeScale();
				AddChip(csquares->at(i)->address, info, chips);
				if( improved )
				{
					AddChip(csquares->at(i)->address.Up(), info, chips);
					AddChip(csquares->at(i)->address.Down(), info, chips);
					AddChip(csquares->at(i)->address.Left(), info, chips);
					AddChip(csquares->at(i)->address.Right(), info, chips);
				}
			}
		}
	}
}

void LightningBonus::StartEffect(Game::FieldAddress from, float startTime)
{
	for(std::vector<GameLightningController*>::iterator itr = _controllers.begin(); itr != _controllers.end(); ++itr)
	{
		Game::AddController(*itr);
	}
	_controllers.clear();
}

void LightningBonus::DrawOnChip(FPoint chipPos, float localTime, Render::SpriteBatch *batch)
{
	float t = math::sin(localTime*6.f);
	float scale = GameSettings::SQUARE_SIDEF / 44.0f;
	Render::device.PushMatrix();
	Render::device.MatrixTranslate(chipPos + FPoint(7.0f, 7.0f) * scale);
	Game::MatrixSquareScale();
	Render::device.MatrixScale(scale * (0.5f + t * 0.02f));
	FPoint offset = hang_lightning_tex->getBitmapRect().RightTop()/2.f;
	if(batch)
	{
		batch->Draw(hang_lightning_tex, Render::ALPHA, -offset);
	}else{
		hang_lightning_tex->Draw(-offset);
	}
	Render::device.PopMatrix();
}

void LightningBonus::DrawOnSquare(FPoint chipPos, float localTime, Render::SpriteBatch *batch)
{
	float t = math::sin(localTime*6.f);
	float scale = GameSettings::SQUARE_SIDEF / 44.0f;
	Render::device.PushMatrix();
	Render::device.MatrixTranslate(chipPos);
	Render::device.MatrixScale(scale * (0.8f + t * 0.03f));
	FPoint offset = hang_lightning_tex->getBitmapRect().RightTop()/2.f;
	if(batch)
	{
		batch->Draw(hang_lightning_tex, Render::ALPHA, -offset);
	}else{
		hang_lightning_tex->Draw(-offset);
	}
	Render::device.PopMatrix();
}

/******************************************************************************************************/

LightningArrowBonus::LightningArrowBonus(ColorMask colors, BYTE dirs, int radius)
	: _dirs(dirs)
	, _radius(radius)
	, _lightning(colors)
{
	// в комбинации все стрелы превращаем в прямые
	if(_dirs & Hang::ARROW_4D)
		_dirs = RotateArrow(_dirs, 1);
}

void LightningArrowBonus::GetAffectedChips(Game::FieldAddress from, AffectedArea &chips, float startTime)
{
	if(_lightningChips.empty())
	{
		_lightning.GetAffectedChips(from, _lightningChips, startTime);
	}

	ClearCellInfo info(0.0f, false);

	for(AffectedArea::iterator itr = _lightningChips.begin(); itr != _lightningChips.end(); ++itr)
	{
		BYTE dir = _arrows.insert( std::make_pair(itr->first, RotateArrow(_dirs, math::random(0,3) * 2)) ).first->second;

		AffectedArea arrChips;
		ArrowBonus arr(dir, _radius, 0);
		arr.GetAffectedChips(itr->first, arrChips, startTime);
		
		for(AffectedArea::iterator aitr = arrChips.begin(); aitr != arrChips.end(); ++aitr)
		{
			info.delay = aitr->second.delay + itr->second.delay;
			AddChip(aitr->first, info, chips);
		}	
	}
}

void LightningArrowBonus::StartEffect(Game::FieldAddress from, float startTime)
{
	_lightning.StartEffect(from, startTime);
	Game::AddController(new LightningArrowController(_lightningChips, _arrows, _radius, startTime) );
}

LightningArrowController::LightningArrowController(const AffectedArea &chips, const std::map<Game::FieldAddress, BYTE> &arrows, int radius, float startTime)
	: GameFieldController("LightningArrowBonus", 1.0f, GameField::Get())
	, _radius(radius)
	, _arrows(arrows)
	, _chips(chips)
{
	local_time = -startTime;
}

void LightningArrowController::Update(float dt)
{
	if( local_time < 0.0f)
	{
		local_time += dt;
	}
	else
	{
		local_time += time_scale * dt;

		for(AffectedArea::iterator itr = _chips.begin(); itr != _chips.end();  )
		{
			if( itr->second.delay <= local_time )
			{
				std::map<Game::FieldAddress, BYTE>::iterator dir = _arrows.find(itr->first);
				Assert(dir != _arrows.end());
				ArrowBonus arr(dir->second, _radius, 0);
				arr.StartEffect(itr->first, 0.0f);
				_chips.erase(itr++);
				_arrows.erase(dir);
			}
			else
			{
				++itr;
			}
		}
	}
}

void LightningArrowController::Draw()
{
}

bool LightningArrowController::isFinish()
{
	return _chips.empty();
}

/////////////////////////////////////////////////////

LittleBombBonus::LittleBombBonus()
{
}

void LittleBombBonus::DrawOnChip(FPoint chipPos, float localTime, Render::SpriteBatch *batch)
{
	Render::device.PushMatrix();
	Game::MatrixSquareScale();
	Render::device.PopMatrix();
}

void LittleBombBonus::DrawOnSquare(FPoint chipPos, float localTime, Render::SpriteBatch *batch)
{
}

void LittleBombBonus::Save(Xml::TiXmlElement *xml_elem)
{
}

void LittleBombBonus::GetAffectedChips(Game::FieldAddress cell, AffectedArea &chips, float startTime)
{
	ClearCellInfo info(startTime + 0.1f, false);

	for (int i = -1; i <= 1; i++)
	{
		for (int j = -1; j <= 1; j++)
		{
			if (i * j == 0 && (i!=0 || j!=0)) {  //берем только 4 клетки
				Game::FieldAddress fa = cell + Game::FieldAddress(i, j);		
				if(Game::activeRect.Contains(fa.ToPoint()))
				{
					HangBonus::AddChip(fa, info, chips);
				}
			}
		}
	}
}



} // end of namespace