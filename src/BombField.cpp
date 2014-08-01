#include "stdafx.h"
#include "GameField.h"

#include "GameInfo.h"
#include "BombField.h"
#include "EditorUtils.h"
#include "Game.h"
#include "GameBonuses.h"
#include "Energy.h"

namespace Gadgets
{
	BombFields bombFields;
}

const float DEATH_TIME_SCALE = 2.f;
const float BOOM_TIME_SCALE = 0.9f;

static int BOMB_INDEX = 1;
	// Глобальный индекc бомб, cделан тупо для быcтроты, т.к. индекcа у бомб как таковых нет...

BombField::BombField()
	: _position(IPoint(-1, -1))
	, _state(WAIT)
	, selected(false)
	, _deathTimer(0.f)
	, _boomTimer(0.f)
	, _textureDelta(IPoint(-5,-5))
	, bangRadius(2)
	, _visible(false)
	, _eff(NULL)
{
	_texture = Core::resourceManager.Get<Render::Texture>("Bombonfield");
	_textureUp = Core::resourceManager.Get<Render::Texture>("BombonfieldUp");
	IRect rect =_textureUp->getBitmapRect();
	_textureDelta.x = -rect.width/2;
	_textureDelta.y = -rect.height/2;
}

void BombField::Draw()
{
	if (!_visible)
	{
		return;
	}
	if (_state == HIDE)	return;
	
	// отрисовка зоны поражения в редакторе	
	if (EditorUtils::editor)
	{
		Game::AffectedArea chips;
		Game::BombBonus bomb(bangRadius, 3);
		bomb.GetAffectedChips( Game::FieldAddress(_position), chips, 0.0f);

		IRect draw(GameSettings::SQUARE_SIDE / 2, GameSettings::SQUARE_SIDE / 2, 0, 0);
		draw.Inflate((GameSettings::SQUARE_SIDE / 2) - 4);
		
		Render::device.SetTexturing(false);
		Render::BeginColor(selected ? Color(255,50,50,128) : Color(150,10,10,80));

		for(Game::AffectedArea::iterator itr = chips.begin(); itr != chips.end(); ++itr)
		{
			Render::device.PushMatrix();
			FPoint pos = FPoint(itr->first.ToPoint()) * GameSettings::SQUARE_SIDEF;
			Render::device.MatrixTranslate(pos);
			Render::DrawRect(draw);
			Render::DrawFrame(draw);
			Render::device.PopMatrix();
		}

		Render::EndColor();
		Render::device.SetTexturing(true);
	}

	Render::device.PushMatrix();
	Render::device.MatrixTranslate(FPoint(_position) * GameSettings::SQUARE_SIDEF + GameSettings::CELL_HALF);

	if (EditorUtils::editor)
	{
		if (selected) 
		{
			Render::FreeType::BindFont("debug");
			Render::PrintString(0, -50, "Press 'e' to change explosive radius.");
		}
	}
	
	if (_state != ACTIVATE)
	{
		float scale = GameSettings::SQUARE_SIDEF / 44.0f;
		Render::device.PushMatrix();
		Render::device.MatrixScale(0.5f * scale);
		Render::device.MatrixTranslate(math::Vector3(_textureDelta));

		_texture->Draw();
		_effCont.Draw();
		_textureUp->Draw();

		Render::device.PopMatrix();
	}

	Render::device.PopMatrix();
}

void BombField::UpdateVisible()
{
	_visible = Game::visibleRect.Contains(_position);
	
	if (_visible && !_eff && _state != HIDE && _state != ACTIVATE)
	{
		_eff = _effCont.AddEffect("BombField");
		_eff->SetPos(GameSettings::CELL_HALF);
		_eff->Reset();
	}
	
}

void BombField::Update(float dt)
{
	if (!_visible)
	{
		return;
	}

	_effCont.Update(dt);

	if (_state == ACTIVATE)
	{
		_deathTimer += dt / DEATH_TIME_SCALE;

		if (_deathTimer > 1)
		{	
			_state = HIDE;
		}	
	}
	else if (_state == WAIT_FOR_BOOM)
	{
		_boomTimer += dt / BOOM_TIME_SCALE;

		if (_boomTimer > 1)
		{
			StartBang(true);
		}
	}
	else if (_state == WAIT)
	{
		if(CheckActivateZone())
		{
			_state = WAIT_FOR_BOOM;
			ParticleEffect *eff = _effCont.AddEffect("StartBombField");
			eff->SetPos(GameSettings::CELL_HALF);
			eff->Reset();
		}
	}
}

bool BombField::CheckActivateZone(const IPoint &pos)
{
	if(_state != WAIT)
	{
		return false;
	}
	return (_position == pos);
}

bool BombField::CheckActivateZone()
{
	// проверяет клетки внутри радиуcа на наличие энергии
	if(_state != WAIT)
	{
		return false;
	}

	return Energy::field.EnergyExists(Game::FieldAddress(_position));
}

void BombField::Reset()
{
	_state = WAIT;
	_boomTimer =  0.f;
	_deathTimer = 0.f;
	selected = false;
	_visible = false;
}

void BombField::Save(Xml::TiXmlElement *root)
{
	//по алфавиту!
	root->SetAttribute("bangRadius", utils::lexical_cast(bangRadius));
	root->SetAttribute("x", utils::lexical_cast(_position.x));
	root->SetAttribute("y", utils::lexical_cast(_position.y));
}

void BombField::Load(rapidxml::xml_node<> *root)
{
	bangRadius = Xml::GetIntAttributeOrDef(root, "bangRadius", 3);
	SetPosition(IPoint(root));
}

void  BombField::IncBangRadius(int delta)
{
	bangRadius = std::max(1, bangRadius + delta);
}

void BombField::SetPosition(const IPoint& point)
{
	_position = point;
}

IPoint BombField::GetPosition() const
{
	return _position;
}

bool BombField::Editor_CaptureBomb(const IPoint& mouse_pos, int x, int y)
{
	Game::FieldAddress index = GameSettings::GetMouseAddress(mouse_pos);
	return (_position == index.ToPoint());
}

void BombField::StartBang(bool explode)
{
	_state = ACTIVATE;

	Game::FieldAddress oldAddress = Game::FieldAddress(_position);

	Game::Square *oldSquare = GameSettings::gamefield[oldAddress];	
	if( !Game::isBuffer(oldSquare) )
	{
		oldSquare->SetBomb(0);
	}

	if (_eff)
	{
		_eff->Finish();
		_eff = NULL;
	}

	if( explode )
	{
		GameField::Get()->DoBigBomb(_position, bangRadius);
	}
}

/**
*
*		BombFields Class
*
**/

BombFields::BombFields()
	: _dragBomb(NULL)
	, _selectedBomb(NULL)
	, _startDragPos(IPoint(0,0))
{
	_allBombs.clear();
}

BombFields::~BombFields()
{
}

void BombFields::Reset()
{
	size_t count = _allBombs.size();
	for (size_t i = 0; i < count; i++)
	{
		_allBombs[i]->Reset();
	}
}

void BombFields::Clear()
{
	_dragBomb = NULL;
	_selectedBomb = NULL;

	_allBombs.clear();
}

void BombFields::Draw()
{
	UpdateVisibility();

	for (size_t i = 0; i < _allBombs.size(); i++)
	{
		_allBombs[i]->Draw();
	}
}

void BombFields::Update(float dt)
{
	for (size_t i = 0; i < _allBombs.size(); i++)
	{
		_allBombs[i]->Update(dt);
	}
}

void BombFields::UpdateVisibility()
{
	for (size_t i = 0; i < _allBombs.size(); i++)
	{
		_allBombs[i]->UpdateVisible();
	}
}

void BombFields::AddBomb(BombField::HardPtr bomb)
{
	bomb->selected = true;

	_selectedBomb = bomb.get();
	BOMB_INDEX++;
	
	Game::FieldAddress newAddress = Game::FieldAddress(_selectedBomb->GetPosition());
	GameSettings::gamefield[newAddress]->SetBomb(BOMB_INDEX);

	_allBombs.push_back(bomb);
}

void BombFields::IncBangRadius(int delta)
{
	if (_selectedBomb)
		_selectedBomb->IncBangRadius(delta);
}

void BombFields::Editor_MoveBomb(const IPoint& mouse_pos, int x, int y)
{
	if(_dragBomb)
		_dragBomb->SetPosition(IPoint(x,y));
}

bool BombFields::Editor_CaptureBomb(const IPoint& mouse_pos, int x, int y)
{
	bool captured = false;

	size_t count = _allBombs.size();
	for (size_t i = 0; i < count; i++)
	{
		BombField::HardPtr bomb = _allBombs[i];
		captured = bomb->Editor_CaptureBomb(mouse_pos, x, y);

		if (captured) 
		{
			Reset();

			bomb -> selected = true;
			_dragBomb = bomb.get();
			_selectedBomb = bomb.get();
			
			_startDragPos = bomb->GetPosition();

			break;
		}
	}

	return captured;
}

bool BombFields::Editor_CheckRemove(const IPoint& mouse_pos, int x, int y)
{
	return false;
}

void BombFields::Editor_ReleaseBomb()
{
	if (!_dragBomb) return;

	Game::FieldAddress oldAddress = Game::FieldAddress(_startDragPos);

	int bomb_index = GameSettings::gamefield[oldAddress]->GetBombIndex();

	GameSettings::gamefield[oldAddress]->SetBomb(0);	

	// обнуляем ячейки под бомбой

	Game::FieldAddress newAddress = Game::FieldAddress(_dragBomb->GetPosition());

	GameSettings::gamefield[newAddress]->SetBomb(bomb_index);	

	_dragBomb = NULL;
	_startDragPos = IPoint(0,0);
}

bool BombFields::Editor_RemoveUnderMouse(const IPoint& mouse_pos, int x, int y)
{
	for (size_t i = 0; i < _allBombs.size(); i++)
	{
		BombField::HardPtr bomb = _allBombs[i];

		if (bomb->Editor_CaptureBomb(mouse_pos, x, y)) 
		{
			Game::FieldAddress oldAddress = Game::FieldAddress(bomb->GetPosition());

			GameSettings::gamefield[oldAddress]->SetBomb(0);

			_allBombs.erase(_allBombs.begin() + i);
			return true;
		}
	}
	return false;
}

void BombFields::Editor_CutToClipboard(IRect part)
{
	_clipboard.clear();
	for(BombFieldVector::iterator itr = _allBombs.begin(); itr != _allBombs.end(); )
	{
		IRect r = IRect(0, 0, 2, 2).MovedBy((*itr)->GetPosition());
		if( part.Contains(r) ) {
			(*itr)->SetPosition( r.LeftBottom() - part.LeftBottom() );
			_clipboard.push_back(*itr);
			itr = _allBombs.erase(itr);
		} else {
			++itr;
		}
	}
}

bool BombFields::Editor_PasteFromClipboard(IPoint pos)
{
	for(BombFieldVector::iterator itr = _clipboard.begin(); itr != _clipboard.end(); ++itr)
	{
		(*itr)->SetPosition(pos + (*itr)->GetPosition());
		_allBombs.push_back(*itr);
	}
	_clipboard.clear();
	return true;
}

void BombFields::SaveLevel(Xml::TiXmlElement *root)
{
	if(_allBombs.empty())
	{
		return;
	}
	Xml::TiXmlElement *bombs = root -> InsertEndChild(Xml::TiXmlElement("BombFields"))->ToElement();

	size_t count = _allBombs.size();
	for (size_t i = 0; i < count; i++)
	{
		Xml::TiXmlElement *bomb = bombs->InsertEndChild(Xml::TiXmlElement("Bomb"))->ToElement();
		_allBombs[i]->Save(bomb);
	}
}

void BombFields::LoadLevel(rapidxml::xml_node<> *root)
{
	Clear(); // Удаляем, еcли еcть что-то...

	rapidxml::xml_node<> *bobms = root -> first_node("BombFields");
	if (!bobms) return;

	rapidxml::xml_node<> *bomb = bobms -> first_node("Bomb");

	while (bomb)
	{
		BombField::HardPtr b = boost::make_shared<BombField>();
		b->Load(bomb);
		AddBomb(b);

		bomb = bomb->next_sibling("Bomb");
	}
}


void BombFields::GetBombInPoint(const IPoint &pos, std::vector<BombField::HardPtr> &bombs)
{
	size_t count = _allBombs.size();
	for (size_t i = 0; i < count; i++)
	{
		if(_allBombs[i]->CheckActivateZone(pos))
		{
			bombs.push_back(_allBombs[i]);
		}
	}
}

void BombFields::StartBang(const IPoint &pos, bool explode)
{
	size_t count = _allBombs.size();
	for (size_t i = 0; i < count; i++)
	{
		if(_allBombs[i]->CheckActivateZone(pos))
		{
			_allBombs[i]->StartBang(explode);
		}
	}
}