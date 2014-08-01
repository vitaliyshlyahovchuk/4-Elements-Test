#include "stdafx.h"
#include "FieldBears.h"
#include "EditorUtils.h"
#include "Energy.h"
#include "Match3Gadgets.h"
#include "Game.h"

namespace Gadgets
{

FieldBears bears;

bool Bear::IsFilled() const
{
	for(int x = 0; x < rect.width; ++x) {
		for(int y = 0; y < rect.height; ++y) {
			Game::FieldAddress fa(x + rect.x, y + rect.y);
			if( !Energy::field.FullOfEnergy(fa) ) {
				return false;
			}
		}
	}
	return true;
}

FieldBears::BearList::iterator FieldBears::FindBearOnSquare(IPoint pos)
{
	BearList::iterator itr = _bears.begin();
	while(itr != _bears.end())
	{
		if( itr->rect.Contains(pos) )
			break;
		++itr;
	}
	return itr;
}

void FieldBears::Init(GameField *field, const bool &with_editor)
{
	BaseEditorMaker::Init(field, with_editor);

	_type = 0;
	_totalBears = 0;

	_bearTypes.clear();
	_bearTypes.push_back( BearType(IPoint(1,2), "FieldBear1") );
	_bearTypes.push_back( BearType(IPoint(2,4), "FieldBear1") );
	_bearTypes.push_back( BearType(IPoint(2,1), "FieldBear2") );
	_bearTypes.push_back( BearType(IPoint(4,2), "FieldBear2") );
	_bearTypes.push_back( BearType(IPoint(2,3), "FieldBear1") );
	_bearTypes.push_back( BearType(IPoint(3,2), "FieldBear2") );

	_currentBear = Bear(IPoint(0,0), _bearTypes[_type], _type);

	_drawOverlay = false;
}

void FieldBears::LoadLevel(rapidxml::xml_node<> *root)
{
	Clear();
	rapidxml::xml_node<> *xml_cells = root->first_node("Bears");
	if( xml_cells )
	{
		rapidxml::xml_node<> *xml_cell = xml_cells->first_node("Bear");
		while( xml_cell )
		{
			IPoint pos(xml_cell);
			int type = Xml::GetIntAttributeOrDef(xml_cell, "type", 0);
			_bears.push_back( Bear(pos, _bearTypes[type], type) );

			xml_cell = xml_cell->next_sibling("Bear");
		}
	}
	_totalBears = _bears.size();

	_drawOverlay = Gadgets::levelSettings.getString("ShowBears") == "true";
}

void FieldBears::SaveLevel(Xml::TiXmlElement *root)
{
	Xml::TiXmlElement *xml_cells = root->InsertEndChild(Xml::TiXmlElement("Bears"))->ToElement();
	for(Bear &bear : _bears)
	{
		Xml::TiXmlElement *xml_cell = xml_cells->InsertEndChild(Xml::TiXmlElement("Bear"))->ToElement();
		xml_cell->SetAttribute("x", bear.rect.x);
		xml_cell->SetAttribute("y", bear.rect.y);
		xml_cell->SetAttribute("type", bear.type);
	}
}

void FieldBears::Clear()
{
	_bears.clear();
}

bool FieldBears::CheckRect(IRect area)
{
	for(int x = 0; x < area.width; ++x) {
		for(int y = 0; y < area.height; ++y) {
			Game::FieldAddress fa(x + area.x, y + area.y);
			Game::Square *sq = GameSettings::gamefield[fa];
			if( !Game::isVisible(sq) )
				return false;
		}
	}
	return true;
}

bool FieldBears::Editor_LeftMouseDown(const IPoint &mouse_pos, Game::Square *sq)
{
	if( EditorUtils::activeEditBtn == EditorUtils::FieldBears)
	{
		Game::FieldAddress fa = GameSettings::GetMouseAddress(mouse_pos);
		Bear bear(fa.ToPoint(), _bearTypes[_type], _type);

		if( CheckRect(bear.rect) ) {
			_bears.push_back( bear );
		}
		return true;
	}
	return false;
}

bool FieldBears::Editor_RightMouseDown(const IPoint &mouse_pos, Game::Square *sq)
{
	if( EditorUtils::activeEditBtn == EditorUtils::FieldBears)
	{
		IPoint pos = GameSettings::GetMouseAddress(mouse_pos).ToPoint();
		BearList::iterator itr = FindBearOnSquare(pos);
		if( itr != _bears.end() ) {
			_bears.erase(itr);
		}
		return true;
	}
	return false;
}

bool FieldBears::Editor_MouseUp(const IPoint &mouse_pos, Game::Square *sq)
{
	if( EditorUtils::activeEditBtn == EditorUtils::FieldBears)
	{
		return true;
	}
	return false;
}

bool FieldBears::Editor_MouseMove(const IPoint &mouse_pos, Game::Square *sq)
{
	if( EditorUtils::activeEditBtn == EditorUtils::FieldBears)
	{
		Game::FieldAddress fa = GameSettings::GetMouseAddress(mouse_pos);
		_currentBear.rect.MoveTo(fa.ToPoint());
		_currentRectValid = CheckRect(_currentBear.rect);

		return true;
	}
	return false;
}

bool FieldBears::Editor_MouseWheel(int delta, Game::Square *sq)
{
	if( EditorUtils::activeEditBtn == EditorUtils::FieldBears)
	{
		_type = (_type + 1) % _bearTypes.size();
		_currentBear = Bear(_currentBear.rect.LeftBottom(), _bearTypes[_type], _type);
		return true;
	}
	return false;
}

void FieldBears::Draw(bool on_energy)
{
	Render::device.PushMatrix();
	Render::device.MatrixTranslate(math::Vector3(0.0f, 0.0f, ZBuf::ENERGY - 2.0f * ZBuf::Z_EPS));

	for(Bear &bear : _bears)
	{
		bear.tex->Bind();
		IPoint pos = bear.rect.LeftBottom() * GameSettings::SQUARE_SIDE;
		IRect draw_rect(pos.x, pos.y, bear.rect.width * GameSettings::SQUARE_SIDE, bear.rect.height * GameSettings::SQUARE_SIDE);
		Render::DrawRect(draw_rect);
	}

	Render::device.PopMatrix();
}

void FieldBears::DrawOverlay()
{
	if(_drawOverlay)
	{
		Render::device.SetTexturing(false);
		Render::BeginColor(Color(0,80,255,50));
		for(Bear &bear : _bears)
		{
			IPoint pos = bear.rect.LeftBottom() * GameSettings::SQUARE_SIDE;
			IRect draw_rect(pos.x, pos.y, bear.rect.width * GameSettings::SQUARE_SIDE, bear.rect.height * GameSettings::SQUARE_SIDE);
			Render::DrawRect(draw_rect);
		}
		Render::EndColor();
		Render::device.SetTexturing(true);
	}
}

void FieldBears::DrawEdit()
{
	if(EditorUtils::activeEditBtn == EditorUtils::FieldBears)
	{
		Draw(false);

		Render::device.PushMatrix();
		Render::device.MatrixTranslate(math::Vector3(0.0f, 0.0f, ZBuf::ENERGY - 2.0f * ZBuf::Z_EPS));

		_currentBear.tex->Bind();
		Color col = _currentRectValid ? Color::WHITE : Color::RED;
		col.alpha = 128;
		Render::BeginColor(col);
		IPoint pos = _currentBear.rect.LeftBottom() * GameSettings::SQUARE_SIDE;
		IRect draw_rect(pos.x, pos.y, _currentBear.rect.width * GameSettings::SQUARE_SIDE, _currentBear.rect.height * GameSettings::SQUARE_SIDE);
		Render::DrawRect(draw_rect);
		Render::EndColor();

		Render::device.PopMatrix();
	}
}

void FieldBears::Update()
{
	for(BearList::iterator itr = _bears.begin(); itr != _bears.end(); )
	{
		if( itr->IsFilled() ) {
			Game::Orders::KillCell(Game::Order::BEAR, Game::FieldAddress(itr->rect.LeftBottom()));
			itr = _bears.erase(itr);
		} else {
			++itr;
		}
	}
}

int FieldBears::TotalBears() const
{
	return _totalBears;
}

int FieldBears::BearsCount() const
{
	return _bears.size();
}

int FieldBears::FoundBears() const
{
	return (_totalBears - _bears.size());
}

} // Gadgets