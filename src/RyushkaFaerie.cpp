#include "stdafx.h"
#include "RyushkaFaerie.h"
#include "GameInfo.h"

RyushkaFaerie::RyushkaFaerie(rapidxml::xml_node<>* xe)
	: Ryushka(xe, 1.f)
	, _clientRect(0,0,140,140)
{
	_function = xe->first_attribute("function")->value();
	_type = xe->first_attribute("name")->value();
	_state = RF_HIDE;
	IPoint pos(xe->first_node("Position"));
	_clientRect.x = pos.x;
	_clientRect.y = pos.y;
	_visible = true;

}
RyushkaFaerie::~RyushkaFaerie()
{
}

void RyushkaFaerie::Show()
{
	Core::LuaCallVoidFunction(_function.c_str(), _type);
	_state = RF_SHOW;
}

void RyushkaFaerie::Hide()
{
	_state = RF_HIDE;
}

void RyushkaFaerie::Update(float dt)
{
	if(_state == RF_HIDE){
		if(_visible){
			Show();
		}
	}else if(_state == RF_SHOW){
		if(!_visible){
			Hide();
		}
	}
}

void RyushkaFaerie::OnDraw()
{
}

void RyushkaFaerie::SaveToXml(Xml::TiXmlElement *parentElem)
{
	
Xml::TiXmlElement *elem = Ryushka::CreateXmlElement(parentElem, "RyushkaFaerie");
	elem->SetAttribute("function", _function);
	elem->SetAttribute("tab", 1);
	Xml::TiXmlElement *elem2 = elem->InsertEndChild(Xml::TiXmlElement("Position"))->ToElement();
	elem2->SetAttribute("x", _clientRect.x);
	elem2->SetAttribute("y", _clientRect.y);
}

IRect RyushkaFaerie::getClientRect()
{
	return _clientRect;
}

IRect RyushkaFaerie::getVisibleRect()
{
	return _clientRect;
}

void RyushkaFaerie::setPosition(const IPoint &position)
{
	_clientRect =	_clientRect.MovedTo(position);
}