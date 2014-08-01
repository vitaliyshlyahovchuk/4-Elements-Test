#include "stdafx.h"

#include "EditBoxEx.h"
#include <utf8.h>

namespace GUI {

EditBoxEx::EditBoxEx(const std::string& name, rapidxml::xml_node<>* xmlElement)
	: Widget(name, xmlElement)
	, _limit(10)
	, _timer(0.f)
    , _fontColor(Color::WHITE)
	, _active(false)
    , _ibeamPos(0)
	, _canEdit(true)
{
	_isFreeze = true;
	rapidxml::xml_node<>*elem = xmlElement->first_node();

	_limit = Xml::GetIntAttributeOrDef(xmlElement, "limit", _limit);
	_active = Xml::GetBoolAttributeOrDef(xmlElement, "active", _active);
	_canEdit = Xml::GetBoolAttributeOrDef(xmlElement, "can_edit", _canEdit);

	while (elem) {
		std::string elementName(elem->name());
		if (elementName == "font") {
			_font = Xml::GetStringAttribute(elem, "name");
			std::string color = Xml::GetStringAttributeOrDef(elem, "color", "");
            if (!color.empty()) {
                _fontColor = Color(color);
            }
		} else if (elementName == "input") {
			std::string denied = Xml::GetStringAttributeOrDef(elem, "denied", "");
			if (!denied.empty()) {
				utf8::iterator<std::string::const_iterator> it(denied.begin(), denied.begin(), denied.end());
				utf8::iterator<std::string::const_iterator> end(denied.end(), denied.begin(), denied.end());
				for ( ; it != end; ++it) {
					_denied.insert(*it);
				}
			}
		}
		elem = elem->next_sibling();
	}

	Assert(!_font.empty());

	rapidxml::xml_node<>*xml_caption = xmlElement->first_node("caption");
	if(xml_caption)
	{
		_caption = Xml::GetStringAttributeOrDef(xml_caption, "text", "");
		_caption_offset = xml_caption;
	}

	_rect = IRect(position.x, position.y, std::min(64, _limit*12), Render::FreeType::getFontHeight(_font));
}

bool EditBoxEx::MouseDown(const IPoint &mouse_pos)
{
	if(_rect.Contains(mouse_pos) && _canEdit)
	{
		Core::messageManager.putMessage(Message("SetDeactiveEditBoxesEx"));
		Core::messageManager.putMessage(Message("SetActive", name));
		return true;
	}
	return false;
}

void EditBoxEx::Draw()
{
	if(_canEdit)
	{
		Render::device.SetTexturing(false);
		Render::BeginColor(Color(100, 100, 100));
		Render::DrawRect(_rect);
		Render::EndColor();
		Render::device.SetTexturing(true);
	}
	if(!_caption.empty())
	{
		Render::FreeType::BindFont(_font);
		Render::PrintString(position + _caption_offset, _caption, 1.0f, LeftAlign, BottomAlign);
	}
	
	Render::FreeType::BindFont(_font);
    Render::BeginColor(_fontColor);
	Render::PrintString(position, _text, 1.0f, LeftAlign, BottomAlign);
    Render::EndColor();
	float t = _timer;
	if (t < 0) {
		t = 0;
	}
	if(_active)
	{
		Render::BeginAlphaMul(0.5f + 0.5f * cosf(t));
		Render::PrintString(position+IPoint(Render::getStringWidth(Utf8_Substr(_text, 0, _ibeamPos), _font), 0), "|", 1.0f, LeftAlign, BottomAlign);
		Render::EndAlphaMul();
	}
}

void EditBoxEx::Update(float dt)
{
	_timer += 7.0f * dt;
}

void EditBoxEx::AcceptMessage(const Message& message)
{
	if (message.getPublisher() == "KeyPress" && _active) 
	{
		_timer = -3;
		
		int code = utils::lexical_cast<int>(message.getData());
		
		// Не принимаем начальные пробелы
		if (_text.empty() && code == 32) {
			return;
		}

		size_t utf8Length = Utf8_Length(_text);
		
#if !defined(ENGINE_TARGET_ANDROID) && !defined(ENGINE_TARGET_IPHONE)
		if (code == -VK_LEFT) {
			if (_ibeamPos > 0) {
				_ibeamPos--;
			}
		} else if (code == -VK_RIGHT) {
			if (_ibeamPos < utf8Length) {
				_ibeamPos++;
			}
		} else if (code == -VK_HOME) {
			_ibeamPos = 0;
		} else if (code == -VK_END) {
			_ibeamPos = utf8Length;
		} else if (code == -VK_DELETE) {
			if (_ibeamPos < utf8Length) {
				_text = Utf8_Erase(_text, _ibeamPos, 1);
			}
		} else
#endif
		if (code == VK_BACK) {
			if (_ibeamPos > 0) {
				_text = Utf8_Erase(_text, _ibeamPos - 1, 1);
				_ibeamPos--;
			}
		} else if (code == VK_RETURN) {
			Core::messageManager.putMessage(Message("Ok", "press"));
		} else if (code > 0 && int(utf8Length) < _limit) {
			if ((code == VK_SPACE || Render::FreeType::IsCharSet(_font, code)) && _denied.find(code) == _denied.end()) {
				_text = Utf8_Insert(_text, _ibeamPos, CodeToUtf8(code));
				_ibeamPos++;
			}
		}
	} 
	else if (message.getPublisher() == "Set") 
	{
		_text.clear();
		const std::string& data = message.getData();
		utf8::iterator<std::string::const_iterator> it(data.begin(), data.begin(), data.end());
		utf8::iterator<std::string::const_iterator> end(data.end(), data.begin(), data.end());
		for ( ; it != end; ++it) 
		{
			int code = *it;
			if (code == 32 || Render::FreeType::IsCharSet(_font, code)) {
				utf8::append(*it, std::back_inserter(_text));
			} else {
				Log::Warn("EditBoxEx: code #" + Int::ToString(*it) + " is not set in font " + _font);
			}
		}
		_ibeamPos = Utf8_Length(_text);
	}
	else if(message.is("SetActive", name))
	{
		_active = true;
	}
	else if(message.is("SetDeactiveEditBoxesEx"))
	{
		_active = false;
	}
	else if (message.getPublisher() == "Clear") 
	{
		_text.clear();
		_ibeamPos = 0;
	}
}

Message EditBoxEx::QueryState(const Message& message) const
{
	if (message.getPublisher() == "Text") {
		return Message(name, _text);
	}
	return Message();
}

} // namespace GUI
