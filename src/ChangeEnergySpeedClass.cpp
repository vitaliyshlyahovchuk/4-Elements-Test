#include "stdafx.h"

#include "ChangeEnergySpeedClass.h"
#include "GameField.h"
#include "Game.h"
#include "Energy.h"

namespace Gadgets
{
	ChangeEnergySpeedItems energySpeedChangers;
}

/* --- SnapGadgets container functions --- */

ChangeEnergySpeed::ChangeEnergySpeed()
	: _sliderTime(IPoint(330, 85), 150, false, 6, 14)
{
	Gamefield = NULL;
	_snapPoint = IPoint(-1, -1);
	
	_energyTimeScale = 1.f;
	_activated = false;
	_selected = false;

	_sliderTime.SetFactor(0.1f);
}

ChangeEnergySpeed::~ChangeEnergySpeed()
{
}



void ChangeEnergySpeed::Draw()
{
	Render::device.SetTexturing(false);

	// Точка центрирования (краcная)
	Render::device.SetTexturing(false);
	IRect draw (_snapPoint.x * GameSettings::SQUARE_SIDE + (GameSettings::SQUARE_SIDE / 2), _snapPoint.y * GameSettings::SQUARE_SIDE + (GameSettings::SQUARE_SIDE / 2), 0, 0);
	draw.Inflate(GameSettings::SQUARE_SIDE / 2);

	if (_selected)
	{
		Render::BeginColor(Color(50,255,50,128));
	}
	else
	{
		Render::BeginColor(Color(20,80,20,128));
	}

	Render::DrawRect(draw);
	Render::DrawFrame(draw);

	Render::EndColor();
/*
	Render::BeginColor(Color(255,80,20,128));

	Render::DrawRect(_sliderRect);
	//Render::DrawFrame(sliderRect);

	Render::EndColor();
*/
	Render::device.SetTexturing(true);
	Render::BeginColor(Color(255,255,255,255));

	Render::FreeType::BindFont("debug");
	
	Render::PrintString(draw.x + 2, draw.y + 35, "Energy", 1.0f, LeftAlign, BaseLineAlign);
	Render::PrintString(draw.x + 2, draw.y + 25, "speed:", 1.0f, LeftAlign, BaseLineAlign);
	Render::PrintString(draw.x + 2, draw.y + 5, Float::ToStringF(_energyTimeScale), 1.0f, LeftAlign, BaseLineAlign);
	
	Render::EndColor();
	
	if (_selected)
	{
		_sliderTime.Draw();
	}
	
}

void ChangeEnergySpeed::Update(float dt)
{
	Game::Square *s = GameSettings::gamefield[_snapPoint.x + 1][_snapPoint.y + 1];

	if (!Game::isBuffer(s) && Energy::field.EnergyExists(s->address) && !_activated)
	{
		// в клетке появилаcь энергия
		_activated = true;
		Gamefield -> SetEnergyTimeScale(_energyTimeScale);
	}
	_sliderTime.SetLabel(std::string("speed : ") + Float::ToStringF(1.f + _sliderTime.GetFactor() * 50.f));
}

void ChangeEnergySpeed::SaveLevel(Xml::TiXmlElement *root)
{
	//по алфавиту!
	root -> SetAttribute("speed", utils::lexical_cast<float>(_energyTimeScale));
	root -> SetAttribute("x", utils::lexical_cast(_snapPoint.x));
	root -> SetAttribute("y", utils::lexical_cast(_snapPoint.y));
		
}

void ChangeEnergySpeed::SetPosition(const IPoint& point)
{
	_snapPoint = point;
	_sliderTime.SetPosition((point * GameSettings::SQUARE_SIDE) + IPoint(0,-20));
}

void ChangeEnergySpeed::Init(GameField *field)
{
	Gamefield = field;
}

void ChangeEnergySpeed::Editor_MoveElements(const IRect& part, const IPoint& delta)
{
}

void ChangeEnergySpeed::Editor_MoveGadget(const IPoint& mouse_pos, int x, int y)
{
	if (_selected)
	{
		_sliderTime.MouseMove(mouse_pos);
	}
}

int ChangeEnergySpeed::Editor_CaptureGadget(const IPoint& mouse_pos, int x, int y)
{
	// Преобразовываем координаты к координатам на поле
	int mx = mouse_pos.x + GameSettings::FieldCoordMouse().x;
	int my = mouse_pos.y + GameSettings::FieldCoordMouse().y;

	IRect r (mx - (GameSettings::SQUARE_SIDE / 2), my - (GameSettings::SQUARE_SIDE / 2), 0, 0);
	r.Inflate((GameSettings::SQUARE_SIDE / 2));
	
	_sliderRect = IRect(_snapPoint.x * GameSettings::SQUARE_SIDE,_snapPoint.y * GameSettings::SQUARE_SIDE - 30,200,30); 

	if (r.Contains(_snapPoint * GameSettings::SQUARE_SIDE)) 
	{
		return (-2);
	}
	else if (_sliderRect.Contains(IPoint(mx,my))) 
	{

		return (-3);
	}
	else 
	{
		return (-1);
	}
}
void ChangeEnergySpeed::Editor_ReleaseGadget(const IPoint& mouse_pos, int x, int y)
{
	_selected = false;
}

ChangeEnergySpeedItems::ChangeEnergySpeedItems()
{
	Gamefield = NULL;
	_activeGadget = NULL;
	
	_dragGadget = NULL;
}

ChangeEnergySpeedItems::~ChangeEnergySpeedItems()
{
}

void ChangeEnergySpeedItems::Release()
{

}


void ChangeEnergySpeedItems::Reset()
{
	size_t count = _gadgets.size();
	for (size_t i = 0; i < count; i++)
	{
		ChangeEnergySpeed *item = _gadgets[i];
		item -> _selected = false;
		item -> _activated = false;
	}
}

void ChangeEnergySpeedItems::Clear()
{
	_activeGadget = NULL;

	_dragGadget = NULL;
	_selectedGadget = NULL;

	size_t count = _gadgets.size();
	for (size_t i = 0; i < count; i++)
	{
		delete _gadgets[i];
		_gadgets[i] = NULL;
	}

	_gadgets.clear();
}

void ChangeEnergySpeedItems::Update(float dt)
{
	size_t count = _gadgets.size();
	for (size_t i = 0; i < count; i++)
	{
		ChangeEnergySpeed *item = _gadgets[i];
		item -> Update(dt);
	}
}

void ChangeEnergySpeedItems::AddGadget(ChangeEnergySpeed *gadget)
{
	gadget -> Init(Gamefield);
	_gadgets.push_back(gadget);
}
void ChangeEnergySpeedItems::Init(GameField *field)
{
	Gamefield = field;
}

void ChangeEnergySpeedItems::Editor_Draw()
{
	size_t count = _gadgets.size();
	for (size_t i = 0; i < count; i++)
	{
		ChangeEnergySpeed *item = _gadgets[i];
		item -> Draw();
	}
}

void ChangeEnergySpeedItems::Editor_MoveItem(const IPoint& mouse_pos, int x, int y)
{
	if (!_dragGadget) return;
	
	IPoint p;

	p.x = x;
	p.y = y;

	_dragGadget -> SetPosition(p);

}

bool ChangeEnergySpeedItems::Editor_CaptureItem(const IPoint& mouse_pos, int x, int y)
{
	int captured = -1;

	size_t count = _gadgets.size();
	for (size_t i = 0; i < count; i++)
	{
		ChangeEnergySpeed *g = _gadgets[i];
		captured = g -> Editor_CaptureGadget(mouse_pos, x, y);

		if (captured != -1) 
		{
			if (captured == -3)
			{
				return true;			
			}
			
			Reset();
			
			g -> _selected = true;

			_dragGadget = g;
			_selectedGadget = g;
			break;
		}
	}

	return (captured != -1);
}

bool ChangeEnergySpeedItems::Editor_RemoveUnderMouse(const IPoint& mouse_pos, int x, int y)
{
	int captured = -1;

	size_t count = _gadgets.size();
	for (size_t i = 0; i < count; i++)
	{
		ChangeEnergySpeed *g = _gadgets[i];
		captured = g -> Editor_CaptureGadget(mouse_pos, x, y);

		if (captured != -1) 
		{
			_gadgets.erase(_gadgets.begin() + i);
			return true;
		}
	}
	return false;
}

bool ChangeEnergySpeedItems::Editor_CheckRemove(const IPoint& mouse_pos, int x, int y)
{
	return false;
}

void ChangeEnergySpeedItems::Editor_ReleaseItem()
{
	_dragGadget = NULL;
}

void ChangeEnergySpeedItems::Editor_MoveElements(const IRect& part, const IPoint& delta)
{
}

void ChangeEnergySpeedItems::SaveLevel(Xml::TiXmlElement *root)
{
	if(_gadgets.empty())
	{
		return;
	}
	Xml::TiXmlElement *gadgets = root -> InsertEndChild(Xml::TiXmlElement("EnergySpeed")) -> ToElement();

	size_t count = _gadgets.size();
	for (size_t i = 0; i < count; i++)
	{
		ChangeEnergySpeed *g = _gadgets[i];
		
		Xml::TiXmlElement *gadget = gadgets -> InsertEndChild(Xml::TiXmlElement("Item")) -> ToElement();
		g -> SaveLevel(gadget);
	}
}
void ChangeEnergySpeedItems::LoadLevel(rapidxml::xml_node<> *root)
{
	Clear(); // Удаляем, еcли еcть что-то...

	rapidxml::xml_node<> *gadgets = root -> first_node("EnergySpeed");
	if (!gadgets) return;

	rapidxml::xml_node<> *gadget = gadgets -> first_node("Item");

	while (gadget)
	{
		ChangeEnergySpeed *g = new ChangeEnergySpeed();

		int x = 0;
		int y = 0;
		float speed = 0.f;

		x = utils::lexical_cast<int> (std::string(gadget ->first_attribute("x")->value()));
		y = utils::lexical_cast<int> (std::string(gadget ->first_attribute("y")->value()));
		speed = utils::lexical_cast<float> (std::string(gadget ->first_attribute("speed")->value()));
		
		g -> SetPosition(IPoint(x, y));
		g -> SetEnergyTimeScale(speed);
		g -> _sliderTime.SetFactor((speed - 1.f) / 50.f);

		Gadgets::energySpeedChangers.AddGadget(g);

		gadget = gadget ->next_sibling("Item");
	}
}

void ChangeEnergySpeedItems::Editor_MouseDown(const IPoint& mouse_pos)
{
	IPoint p;

	p.x = mouse_pos.x + GameSettings::FieldCoordMouse().x;
	p.y = mouse_pos.y + GameSettings::FieldCoordMouse().y;

	size_t count = _gadgets.size();
	for (size_t i = 0; i < count; i++)
	{
		ChangeEnergySpeed *g = _gadgets[i];
		
		g -> _sliderTime.MouseDown(p);
	}
}

void ChangeEnergySpeedItems::Editor_MouseUp(const IPoint& mouse_pos)
{
	IPoint p;

	p.x = mouse_pos.x + GameSettings::FieldCoordMouse().x;
	p.y = mouse_pos.y + GameSettings::FieldCoordMouse().y;
	
	size_t count = _gadgets.size();
	for (size_t i = 0; i < count; i++)
	{
		ChangeEnergySpeed *g = _gadgets[i];
		
		g -> _sliderTime.MouseUp(p);
		g -> SetEnergyTimeScale(1.f + g -> _sliderTime.GetFactor() * 50.f);
	}
}

void ChangeEnergySpeedItems::Editor_MouseMove(const IPoint& mouse_pos)
{
	IPoint p;

	p.x = mouse_pos.x + GameSettings::FieldCoordMouse().x;
	p.y = mouse_pos.y + GameSettings::FieldCoordMouse().y;
	
	size_t count = _gadgets.size();
	for (size_t i = 0; i < count; i++)
	{
		ChangeEnergySpeed *g = _gadgets[i];
		
		g -> _sliderTime.MouseMove(p);
	}
}