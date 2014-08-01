#include "stdafx.h"
#include "Combobox.h"

void EditList::Draw()
{
	Render::device.SetTexturing(false);
	Render::BeginColor(Color(100,100,100,255));
	Render::DrawRect(_rect);
	Render::EndColor();
	Render::device.SetTexturing(true);	

	Render::FreeType::BindFont(_font);
	int count = (int)_values.size();
	IPoint pos;
	pos.x = _rect.x + 1;
	pos.y = _rect.y - 1 ;
	for(int i = count-1; i >= 0; i--)
	{
		if(i == _selected)
		{
			Render::device.SetTexturing(false);
			Render::BeginColor(Color(50,50,50,255));
			Render::DrawRect(IRect(pos, _rect.width,_cellHeight));
			Render::EndColor();
			Render::device.SetTexturing(true);
		}
		Render::PrintString(pos, _values[i], 1.f, LeftAlign, BottomAlign);
		pos.y += _cellHeight;
	}
}


void EditList::Init(const std::string &font)
{
	_font = font;
	_values.clear();
	_cellHeight = Render::getFontHeight(_font) + 2;
}

void EditList::Add(const std::string &value)
{
	_values.push_back(value);
}

void EditList::Calc(const IPoint &pos)
{
	_rect.x = pos.x;
	_rect.y = pos.y;
	_rect.width = 0;
	size_t count = _values.size();
	for(size_t i = 0; i < count; i++)
	{
		_rect.width = math::max(_rect.width, (int)Render::getStringWidth(_values[i], _font) + 2);
	}
	//_rect.x += _cellHeight*count;
	_rect.height = _cellHeight*count;

	_rect.x = math::clamp(0, Render::device.Width() - _rect.width, _rect.x); 
	_rect.y = math::clamp(0, Render::device.Height() - _rect.height, _rect.y); 
	_selected = -1;
}

void EditList::MouseMove(const IPoint &mouse_pos)
{
	_selected = -1;
	if(_rect.Contains(mouse_pos))
	{
		int selected = (_rect.height - (mouse_pos.y - _rect.y))/_cellHeight;
		if(0 <= selected && selected < (int)_values.size())
		{
			_selected = selected;
		}
	}
}

std::string EditList::MouseDown(const IPoint &mouse_pos)
{
	if(_rect.Contains(mouse_pos))
	{
		int selected = (_rect.height - (mouse_pos.y - _rect.y))/_cellHeight;
		if(0 <= selected && selected < (int)_values.size())
		{
			return _values[selected];
		}	
	}
	return "no_selected";
}


Combobox::Combobox(const std::string &name_, rapidxml::xml_node<>* xmlElement)
	: GUI::Widget(name_, xmlElement)
{
	_itemList.Init("editor");
}

Combobox::~Combobox()
{
}

void Combobox::Draw()
{	
	//Render::device.SetTexturing(false);
	//Render::BeginColor(Color(0,0,0,200));
	//Render::DrawRect(IRect(0,0, Render::device.Width(), Render::device.Height()));
	//Render::EndColor();
	//Render::device.SetTexturing(true);
	_itemList.Draw();
	Render::FreeType::BindFont("editor");
	Render::PrintString(_itemList._rect.x, _itemList._rect.y + _itemList._rect.height, _title, 1.f, LeftAlign, BottomAlign); 

}

void Combobox::Update(float dt)
{
}

void Combobox::AcceptMessage(const Message &message)
{
	if(message.is("AddItem")){
		_itemList._values.push_back(message.getData());
		_itemList.Calc(Core::mainInput.GetMousePos());
	}else if(message.is("Init")){
		_itemList._values.clear();
		_title = message.getData();
		//_itemList.Calc();
		//_parent = message.getVariables().getWidget("parent");
		//_command = message.getVariables().getString("command");
		//_itemList.SetPos(Core::mainInput.GetMousePos());
		//Assert(_parent);
	}
}

void Combobox::MouseMove(const IPoint &mouse_pos)
{
	_itemList.MouseMove(mouse_pos);
}

bool Combobox::MouseDown(const IPoint &mouse_pos)
{
	std::string selected = _itemList.MouseDown(mouse_pos);
	Core::messageManager.putMessage(Message("Select", selected));
	return false;
}

void Combobox::MouseUp(const IPoint &mouse_pos)
{
}

void Combobox::MouseDoubleClick(const IPoint &mouse_pos)
{

}
