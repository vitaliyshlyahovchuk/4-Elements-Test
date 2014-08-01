#include "stdafx.h"
#include "GameBonus.h"
#include "GameBonuses.h"
#include "GameColor.h"
#include "LevelEndEffects.h"

namespace Game
{

BYTE RotateArrow(BYTE dir, int a)
{
	return (dir << a) | (dir >> (8-a));
}

void HangBonus::AddChip(Game::FieldAddress cell, const ClearCellInfo& info, AffectedArea &chips)
{
	std::pair<AffectedArea::iterator, bool> res = chips.insert( std::make_pair(cell, info) );
	if( !res.second )
	{
		// элемент с таким адресом в мапе уже был, корректируем только время задержки если надо
		res.first->second.delay = std::min(res.first->second.delay, info.delay);
		res.first->second.kill_near |= info.kill_near;
	}
}

/******************************************************************************************************/

Hang::Hang()
	: transformChip(NONE)
	, autoRun(false)
{
}

Hang::Hang(const Hang& other)
	: transformChip(other.transformChip)
	, autoRun(other.autoRun)
{
	DeepCopyBonuses(other);
}

Hang::Hang(const std::string& type, int radius, int level, IPoint arrowDir, TransformChip transform, bool autorun)
	: transformChip(transform)
	, autoRun(autorun)
{
	if( type == "" || type == "None" )
	{
	}
	else if(type == "Lightning")
	{
		MakeLightning(0);
	}
	else if( type == "Bomb" )
	{
		MyAssert(1 <= level && level <= 3);
		MakeBomb(radius, level);
	}
	else // Arrow 1x, 2x, 4x
	{
		BYTE dir(0);
		const int dx[] = {1,  1,  0, -1, -1, -1,  0,  1};
		const int dy[] = {0,  1,  1,  1,  0, -1, -1, -1};
		for(dir = 0; dir < sizeof(dx) / sizeof(dx[0]); ++dir) {
			if( arrowDir.x == dx[dir] && arrowDir.y == dy[dir])
				break;
		}
		Assert(dir < sizeof(dx));

		BYTE dirs_arrow = 0;
		if(type == "Arrow 4x"){
			dirs_arrow = ARROW_L | ARROW_R | ARROW_D | ARROW_U;
		} else if(type == "Arrow 4x D") {
			dirs_arrow = ARROW_UL | ARROW_UR | ARROW_DL | ARROW_DR;
		} else if(type == "Arrow 2x"){
			dirs_arrow = ARROW_R | ARROW_L;
		} else if(type == "Arrow 2x R") {
			dirs_arrow = math::random(0,1) ? (ARROW_R|ARROW_L) : (ARROW_U|ARROW_D);
			dir = 0;
		} else if( type == "Arrow 2x H") {
			dirs_arrow = ARROW_R | ARROW_L;
			dir = 0;
		} else if( type == "Arrow 2x V") {
			dirs_arrow = ARROW_U | ARROW_D;
			dir = 0;
		} else { // Arrow 1x
			dirs_arrow = ARROW_R;
		}

		MakeArrow(radius, RotateArrow(dirs_arrow, dir));
	}
}

void Hang::MakeArrow(int radius, BYTE dir)
{
	_bonuses.push_back( boost::make_shared<ArrowBonus>(dir, radius, 0) );
}

void Hang::MakeBomb(int radius, int level)
{
	_bonuses.push_back( boost::make_shared<BombBonus>(radius, level) );
}

void Hang::MakeLightning(ColorMask colors)
{
	_bonuses.push_back( boost::make_shared<LightningBonus>(colors) );
}

void Hang::MakeLittleBomb()
{
	_bonuses.push_back( boost::make_shared<LittleBombBonus>() );
}

void Hang::MakeBonus(HangBonus::HardPtr bonus)
{
	_bonuses.push_back( bonus );
}

void Hang::GetAffectedChips(Game::FieldAddress from, AffectedArea &chips, float startTime)
{
	for(BonusList::const_iterator itr = _bonuses.begin(); itr != _bonuses.end(); ++itr)
	{
		(*itr)->GetAffectedChips(from, chips, startTime);
	}
}

void Hang::StartEffects(Game::FieldAddress cell, float startTime) const
{
	for(BonusList::const_iterator itr = _bonuses.begin(); itr != _bonuses.end(); ++itr)
	{
		(*itr)->StartEffect(cell, startTime);
	}
}

void Hang::Clear()
{
	transformChip = NONE;
	autoRun = false;
	_bonuses.clear();
}

bool Hang::IsEmpty() const
{
	return _bonuses.empty();
}

/***************************************************************************************************
	Комбинирование бонусов между собой
***************************************************************************************************/

static ColorMask AddRandomColorToLightning(ColorMask orig)
{
	ColorMask all = Gadgets::levelColors.GetAllColors();
	ColorMask result = orig;
	while( result == orig && ((result & all) != all) ) {
		result = orig | ColorToMask(Gadgets::levelColors.GetRandom());
	}
	return result;
}

static unsigned int ColorsInMask(ColorMask colors)
{
	unsigned int count = 0;
	while( colors )
	{
		count += colors & 1u;
		colors >>= 1u;
	}
	return count;
}

bool Hang::Arrow_Arrow(ArrowBonus *arrow1, ArrowBonus *arrow2)
{
	// стрелки образуют кресты
	int radius = std::max(arrow1->_radius, arrow2->_radius);
	int level = std::max(arrow1->_level, arrow2->_level) + 1;
	BYTE dirs = (arrow1->_dirs > 0 && arrow2->_dirs > 0) ? ARROW_4 : (arrow1->_dirs | arrow2->_dirs);
	_bonuses.push_back( boost::make_shared<ArrowBonus>(dirs, radius, level) );
	return true;
}

bool Hang::Bomb_Bomb(BombBonus *bomb1, BombBonus *bomb2)
{
	// радиусы бомб складываются, но возможный итоговый радиус ограничен
	int radius = std::min(2, bomb1->_radius + bomb2->_radius);
	int level = std::min(2, bomb1->_level + bomb2->_level);
	_bonuses.push_back( boost::make_shared<BombBonus>(radius, level) );
	return true;
}

bool Hang::Light_Light(LightningBonus *l1, LightningBonus *l2)
{
	// две молнии убивают фишки всех цветов
	_bonuses.push_back( boost::make_shared<LightningBonus>(Gadgets::levelColors.GetAllColors()) );
	return true;
}

bool Hang::LArrow_LArrow(LightningArrowBonus *la1, LightningArrowBonus *la2)
{
	_bonuses.push_back( boost::make_shared<LightningBonus>(Gadgets::levelColors.GetAllColors()) );
	return true;
}

bool Hang::Arrow_Bomb(ArrowBonus *arrow, BombBonus *bomb)
{
	// стрелка становится крестовой, бомба остается без изменения
	BYTE dirs = arrow->_dirs | ARROW_4;
	if( dirs != arrow->_dirs )
	{
		_bonuses.push_back( boost::make_shared<ArrowBonus>(dirs, arrow->_radius, 0) );
		_bonuses.push_back( boost::make_shared<BombBonus>(*bomb) );
		return true;
	}
	return false;
}

bool Hang::Arrow_Light(ArrowBonus *arrow, LightningBonus *light)
{
	_bonuses.push_back( boost::make_shared<LightningArrowBonus>(light->_colors, arrow->_dirs, arrow->_radius) );
	return true;
}

bool Hang::Arrow_LArrow(ArrowBonus *arrow, LightningArrowBonus *la)
{
	BYTE dirs = (arrow->_dirs | la->_dirs | ARROW_4);
	int radius = std::max(arrow->_radius, la->_radius);
	_bonuses.push_back( boost::make_shared<LightningArrowBonus>(la->_lightning._colors, dirs, radius) );
	return false;
}

bool Hang::Bomb_Light(BombBonus *bomb, LightningBonus *light)
{
	// молния убивает 2-3 цвета, бомба остается без изменений
	int seqColor = (Game::ChipColor::COLOR_FOR_ADAPT > 0) ? Game::ChipColor::COLOR_FOR_ADAPT : Gadgets::levelColors.GetRandom();
	ColorMask col = light->_colors | ColorToMask(seqColor);

	if( ColorsInMask(col) <= static_cast<unsigned int>(bomb->_radius) )
	{
		col = AddRandomColorToLightning(col);
		_bonuses.push_back( boost::make_shared<LightningBonus>(col) );
		_bonuses.push_back( boost::make_shared<BombBonus>(*bomb) );
		return true;
	}
	return false;
}

bool Hang::Bomb_LArrow(BombBonus *bomb, LightningArrowBonus *la)
{
	int seqColor = (Game::ChipColor::COLOR_FOR_ADAPT > 0) ? Game::ChipColor::COLOR_FOR_ADAPT : Gadgets::levelColors.GetRandom();
	ColorMask col = la->_lightning._colors | ColorToMask(seqColor);

	if( ColorsInMask(col) <= static_cast<unsigned int>(bomb->_radius) )
	{
		col = AddRandomColorToLightning(col);
		_bonuses.push_back( boost::make_shared<LightningArrowBonus>(col, la->_dirs, la->_radius) );
		_bonuses.push_back( boost::make_shared<BombBonus>(*bomb) );
		return true;
	}
	return false;
}

bool Hang::Light_LArrow(LightningBonus *light, LightningArrowBonus *la)
{
	_bonuses.push_back( boost::make_shared<LightningBonus>(Gadgets::levelColors.GetAllColors()) );
	return true;
}

void Hang::CombineBonuses()
{
	bool changed = true;
	while( changed )
	{
		changed = false;
		for(BonusList::iterator itr1 = _bonuses.begin(); itr1 != _bonuses.end(); ++itr1)
		{
			BonusList::iterator itr2 = itr1;
			++itr2;
			for( ; itr2 != _bonuses.end(); ++itr2)
			{
				if( AddCombination(*itr1, *itr2) )
				{
					_bonuses.erase(itr1);
					_bonuses.erase(itr2);
					changed = true;
					break;
				}
			}

			if(changed)
				break;
		}
	}
}

bool Hang::AddCombination(HangBonus::HardPtr b1, HangBonus::HardPtr b2)
{
	HangBonus::Type type1 = b1->GetType();
	HangBonus::Type type2 = b2->GetType();

	if( type1 == HangBonus::BOMB && type2 == HangBonus::BOMB ) // две бомбы
	{
		return Bomb_Bomb((BombBonus*)b1.get(), (BombBonus*)b2.get());
	}
	else if( type1 == HangBonus::ARROW && type2 == HangBonus::ARROW) // две стрелки
	{
		return Arrow_Arrow((ArrowBonus*)b1.get(), (ArrowBonus*)b2.get());
	}
	else if( type1 == HangBonus::LIGHTNING && type2 == HangBonus::LIGHTNING ) // две молнии
	{
		return Light_Light((LightningBonus*)b1.get(), (LightningBonus*)b2.get());
	}
	else if( type1 == HangBonus::LIGHTNING_ARROW_COMBO && type2 == HangBonus::LIGHTNING_ARROW_COMBO ) // два комбо (молния+стрелка)
	{
		return LArrow_LArrow((LightningArrowBonus*)b1.get(), (LightningArrowBonus*)b2.get());
	}
	else if( type1 == HangBonus::ARROW && type2 == HangBonus::BOMB) // бомба + стрела
	{
		return Arrow_Bomb((ArrowBonus*)b1.get(), (BombBonus*)b2.get());
	}
	else if( type1 == HangBonus::BOMB && type2 == HangBonus::ARROW)
	{
		return Arrow_Bomb((ArrowBonus*)b2.get(), (BombBonus*)b1.get());
	}
	else if( type1 == HangBonus::BOMB && type2 == HangBonus::LIGHTNING ) // молния + бомба
	{
		return Bomb_Light((BombBonus*)b1.get(), (LightningBonus*)b2.get());
	}
	else if( type1 == HangBonus::LIGHTNING && type2 == HangBonus::BOMB )
	{
		return Bomb_Light((BombBonus*)b2.get(), (LightningBonus*)b1.get());
	}
	else if( type1 == HangBonus::ARROW && type2 == HangBonus::LIGHTNING ) // молния + стрелка
	{
		return Arrow_Light((ArrowBonus*)b1.get(), (LightningBonus*)b2.get());
	}
	else if( type1 == HangBonus::LIGHTNING && type2 == HangBonus::ARROW )
	{
		return Arrow_Light((ArrowBonus*)b2.get(), (LightningBonus*)b1.get());
	}
	else if( type1 == HangBonus::ARROW && type2 == HangBonus::LIGHTNING_ARROW_COMBO ) // (молния+стрелка) + стрелка
	{
		return Arrow_LArrow((ArrowBonus*)b1.get(), (LightningArrowBonus*)b2.get());
	}
	else if( type1 == HangBonus::LIGHTNING_ARROW_COMBO && type2 == HangBonus::ARROW )
	{
		return Arrow_LArrow((ArrowBonus*)b2.get(), (LightningArrowBonus*)b1.get());
	}
	else if( type1 == HangBonus::BOMB && type2 == HangBonus::LIGHTNING_ARROW_COMBO ) // (молния+стрелка) + бомба
	{
		return Bomb_LArrow((BombBonus*)b1.get(), (LightningArrowBonus*)b2.get());
	}
	else if( type1 == HangBonus::LIGHTNING_ARROW_COMBO && type2 == HangBonus::BOMB )
	{
		return Bomb_LArrow((BombBonus*)b2.get(), (LightningArrowBonus*)b1.get());
	}
	return false;
}

void Hang::Add(const Hang &hang, bool inChain, IPoint relativePos)
{
	if( inChain )
	{
		DeepCopyBonuses(hang);
		CombineBonuses();
	}
	else
	{
		ArrowBonus *arr1 = (ArrowBonus*) GetBonusByType(HangBonus::ARROW);
		ArrowBonus *arr2 = (ArrowBonus*) hang.GetBonusByType(HangBonus::ARROW);

		// каскадные бонусы суммируются и становятся крестами
		//if( arr1 && arr2 )
		//{
		//	arr1->_radius = std::max(arr1->_radius, arr2->_radius);
		//	arr1->_dirs = (arr1->_dirs | arr2->_dirs | ARROW_4);
		//}

		// каскадные стрелки меняют свое направление (аля Jelly Splash)
		if(arr1 && arr2)
		{
			// в обоих стрелках есть горизонтальное направление и они находятся в одной строке
			if( ((arr1->_dirs & arr2->_dirs) & (ARROW_R | ARROW_L))
				&& (math::abs(relativePos.y) <= std::max(arr1->_level, arr2->_level)) )
			{
				arr1->_dirs = RotateArrow(arr1->_dirs, 2);
			}
			// в обоих стрелках есть вертикальное направление и они находятся в одном столбце
			else if( ((arr1->_dirs & arr2->_dirs) & (ARROW_U | ARROW_D))
				&& (math::abs(relativePos.x) <= std::max(arr1->_level, arr2->_level)) )
			{
				arr1->_dirs = RotateArrow(arr1->_dirs, 2);
			}
		}
	}
}

void Hang::Update(float dt)
{
	for(BonusList::iterator itr = _bonuses.begin(); itr != _bonuses.end(); ++itr)
	{
		(*itr)->Update(dt);
	}
}

void Hang::Load(rapidxml::xml_node<> *xml_elem)
{
	rapidxml::xml_node<> *xml_hang = xml_elem->first_node("Hang");
	if(!xml_hang)
	{
		return;
	}
	int dirs = Xml::GetIntAttributeOrDef(xml_hang, "dirs", 0);
	int arrow_radius = Xml::GetIntAttributeOrDef(xml_hang, "ar", 0);
	int bomb_level = Xml::GetIntAttributeOrDef(xml_hang, "blevel", 0);
	int bomb_radius = Xml::GetIntAttributeOrDef(xml_hang, "br", 0);

	if(dirs > 0 && arrow_radius > 0) {
		MakeArrow(arrow_radius, dirs);
	}

	if(bomb_level > 0 && bomb_radius > 0) {
		MakeBomb(bomb_radius, bomb_level);
	}
}

void Hang::Save(Xml::TiXmlElement *xml_elem)
{
	Xml::TiXmlElement *xml_hang = xml_elem->InsertEndChild(Xml::TiXmlElement("Hang"))->ToElement();
	for(BonusList::iterator itr = _bonuses.begin(); itr != _bonuses.end(); ++itr)
	{
		(*itr)->Save(xml_hang);
	}
}

void Hang::DrawOnChip(FPoint chipPos, float localTime, Render::SpriteBatch *batch)
{
	for(BonusList::iterator itr = _bonuses.begin(); itr != _bonuses.end(); ++itr)
	{
		(*itr)->DrawOnChip(chipPos, localTime, batch);
	}
}

void Hang::DrawOverChip(FPoint chipPos, float localTime, Render::SpriteBatch *batch)
{
	for(BonusList::iterator itr = _bonuses.begin(); itr != _bonuses.end(); ++itr)
	{
		(*itr)->DrawOverChip(chipPos, localTime, batch);
	}
}


void Hang::DrawOnSquare(FPoint chipPos, float localTime, Render::SpriteBatch *batch)
{
	for(BonusList::iterator itr = _bonuses.begin(); itr != _bonuses.end(); ++itr)
	{
		(*itr)->DrawOnSquare(chipPos, localTime, batch);
	}
}

HangBonus *Hang::GetBonusByType(HangBonus::Type type) const
{
	for(BonusList::const_iterator itr = _bonuses.begin(); itr != _bonuses.end(); ++itr)
	{
		if( (*itr)->GetType() == type )
			return (*itr).get();
	}
	return NULL;
}

ColorMask Hang::GetColor() const
{
	ColorMask color = ColorToMask(ChipColor::COLOR_FOR_ADAPT);
	LightningBonus *lightning = (LightningBonus*) GetBonusByType(HangBonus::LIGHTNING);
	if( lightning )
		color |= lightning->_colors;
	return color;
}

void Hang::DeleteBonusType(HangBonus::Type type)
{
	for(BonusList::iterator itr = _bonuses.begin(); itr != _bonuses.end(); )
	{
		if( (*itr)->GetType() == type ) {
			itr = _bonuses.erase(itr);
		} else {
			++itr;
		}
	}
}

void Hang::DeepCopyBonuses(const Hang& other)
{
	for(BonusList::const_iterator itr = other._bonuses.begin(); itr != other._bonuses.end(); ++itr)
	{
		switch( (*itr)->GetType() )
		{
			case HangBonus::ARROW:
			{
				ArrowBonus *bonus = (ArrowBonus*)(*itr).get();
				_bonuses.push_back( boost::make_shared<ArrowBonus>(*bonus) );
				break;
			}
			case HangBonus::BOMB:
			{
				BombBonus *bonus = (BombBonus*)(*itr).get();
				_bonuses.push_back( boost::make_shared<BombBonus>(*bonus) );
				break;
			}
			case HangBonus::LIGHTNING:
			{
				LightningBonus *bonus = (LightningBonus*)(*itr).get();
				_bonuses.push_back( boost::make_shared<LightningBonus>(*bonus) );
				break;
			}
			case HangBonus::LIGHTNING_ARROW_COMBO:
			{
				LightningArrowBonus *bonus = (LightningArrowBonus*)(*itr).get();
				_bonuses.push_back( boost::make_shared<LightningArrowBonus>(*bonus) );
				break;
			}
			case HangBonus::LEVEL_END:
			{
				LevelEndBonus *bonus = (LevelEndBonus*)(*itr).get();
				_bonuses.push_back( boost::make_shared<LevelEndBonus>(*bonus) );
				break;
			}
			case HangBonus::LITTLE_BOMB_BOOST:
			{
				LittleBombBonus *bonus = (LittleBombBonus*)(*itr).get();
				_bonuses.push_back( boost::make_shared<LittleBombBonus>(*bonus) );
				break;
			}
		}
	}
}

Hang& Hang::operator= (const Hang &other)
{
	transformChip = other.transformChip;
	autoRun = other.autoRun;

	_bonuses.clear();
	DeepCopyBonuses(other);

	return *this;
}

HangBonus::HangBonus()
	:_localTime(0.f)
{

}

void HangBonus::Update(float dt)
{
	_localTime += dt;
}

} // end of namespace