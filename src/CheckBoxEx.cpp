#include "stdafx.h"

#include "CheckBoxEx.h"

namespace GUI
{

CheckBoxEx::CheckBoxEx(const std::string& name, rapidxml::xml_node<>* xmlElement)
	: Widget(name, xmlElement)
	, _checked(false)
    , _fontColor(Color::WHITE)
	,_checkSquareSize(18)
	,_font("editor")
	, _offsetCaption(0,0)
{
	_checkSquareSize = Xml::GetIntAttributeOrDef(xmlElement, "size", _checkSquareSize);
	_inner_inflate = Xml::GetIntAttributeOrDef(xmlElement, "inner_inflate", _checkSquareSize/4);
	_send_message = Xml::GetBoolAttributeOrDef(xmlElement, "send_message", false);
	_isFreeze = true;
	rapidxml::xml_node<>*elem = xmlElement->first_node();

	while (elem) {
		std::string elementName(elem->name());
		if (elementName == "font") {
			_font = Xml::GetStringAttribute(elem, "name");
			std::string color = Xml::GetStringAttributeOrDef(elem, "color", "");
            if (!color.empty()) {
                _fontColor = Color(color);
            }
		}
		elem = elem->next_sibling();
	}

	Assert(!_font.empty());

	rapidxml::xml_node<>*xml_caption = xmlElement->first_node("caption");
	if(xml_caption)
	{
		_caption = Xml::GetStringAttributeOrDef(xml_caption, "text", "");
		_offsetCaption.x = Xml::GetIntAttributeOrDef(xml_caption, "x", _checkSquareSize + 5);
		_offsetCaption.y = Xml::GetIntAttributeOrDef(xml_caption, "y", 0);
		_hor_align = TextAlign(Xml::GetIntAttributeOrDef(xml_caption, "align", LeftAlign));

	}

	_rect = IRect(position.x, position.y, _checkSquareSize, _checkSquareSize);
}

bool CheckBoxEx::MouseDown(const IPoint &mouse_pos)
{
	if(_rect.Contains(mouse_pos))
	{
//		Core::messageManager.putMessage(Message("SetDeactiveEditBoxesEx"));
//		Core::messageManager.putMessage(Message("SetActive", name));
		_checked = !_checked;
		if(_send_message)
		{
			Core::messageManager.putMessage(Message(name, _checked ? "true":"false"));
		}
		return true;
	}
	return false;
}

void CheckBoxEx::Draw()
{
	//рамка
	Render::device.SetTexturing(false);
	Render::BeginColor(Color(100, 100, 100));
	Render::DrawRect(_rect);
	Render::EndColor();
	Render::device.SetTexturing(true);
	//рамка
	if (_checked)
	{
		Render::device.SetTexturing(false);
		Render::BeginColor(Color::WHITE);
		Render::DrawRect(_rect.Inflated(-_inner_inflate));
		Render::EndColor();
		Render::device.SetTexturing(true);
	}
	//caption
	if(!_caption.empty())
	{
		Render::BindFont(_font);
		Render::PrintString(position + _offsetCaption, _caption, 1.0f, _hor_align, BottomAlign);
	}
	
}

Message CheckBoxEx::QueryState(const Message& message) const
{
	if (message.is("GetState"))
	{
		int state = _checked;
		return Message(name, utils::lexical_cast(state));
	}
	return Message();
}

void CheckBoxEx::AcceptMessage(const Message& message)
{
	if (message.is("SetState"))
	{
		int state = utils::lexical_cast<int>(message.getData());
		_checked = (state>0);
	}else if(message.is("SetCaption"))
	{
		_caption = message.getData();
	}
}

void CheckBoxEx::Update(float dt)
{
	return;
}









}