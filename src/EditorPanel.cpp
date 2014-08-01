#include "stdafx.h"
#include "EditorPanel.h"
#include "Game.h"
#include "MyApplication.h"

namespace EditorUtils
{
	ButtonInfo::ButtonInfo(IRect rect, rapidxml::xml_node<>* xml_elem)
		: _rect(rect)
	{
		_id = static_cast<EditButtons>(Xml::GetIntAttributeOrDef(xml_elem, "id", -1));
		_tooltip_text = Xml::GetStringAttributeOrDef(xml_elem, "tooltip", "");
	}

	void ButtonInfo::Draw()
	{
		if(_id == EditorUtils::None)
		{
			return;
		}
		if(EditorUtils::activeEditBtn == _id)
		{
			Render::device.SetTexturing(false);
			Render::device.SetBlendMode(Render::ADD);
			Render::BeginColor(Color(255, 100, 100, 120));
			Render::DrawRect(_rect.Inflated(-1));
			Render::EndColor();
			Render::device.SetBlendMode(Render::ALPHA);
			Render::device.SetTexturing(true);
		}
	}

	std::string ButtonInfo::MouseMove(const IPoint &mouse_pos)
	{
		if(_rect.Contains(mouse_pos))
		{
			return _tooltip_text;//Показать тултип
		}
		return "";
	}

	bool ButtonInfo::MouseDown(const IPoint &mouse_pos)
	{
		if(_rect.Contains(mouse_pos))
		{
			if(EditorUtils::activeEditBtn != _id){
				EditorUtils::activeEditBtn = _id;
				Core::LuaCallVoidFunction("OnActivateEditorButton", int(_id)); 
			} else {
				EditorUtils::activeEditBtn = EditorUtils::None;
			}
			return true;
		}
		return false;
	}


	EditorPanel::EditorPanel(const std::string& name_, rapidxml::xml_node<>* elem_)
		: GUI::Widget(name_, elem_)
		, _tooltipExist(false)
		, _tooltip(Core::resourceManager.Get<Render::Text>("EditorTooltipText"))
	{
		std::string textureID = Xml::GetStringAttributeOrDef(elem_, "tex", "EditPanel");
		_texture = Core::resourceManager.Get<Render::Texture>(textureID);
		int col_count = Xml::GetIntAttributeOrDef(elem_, "col_count", 19);
		_cellSize = Xml::GetIntAttributeOrDef(elem_, "side", 19);
		_scrollable = Xml::GetBoolAttributeOrDef(elem_, "scrollable", false);

		_buttons.clear();
		rapidxml::xml_node<>* xml_button = elem_->first_node("button");
		int i = 0;
		while(xml_button)
		{
			IRect rect(i % col_count * _cellSize, i / col_count * _cellSize, _cellSize, _cellSize);
			_buttons.push_back( ButtonInfo(rect, xml_button) );
			xml_button = xml_button->next_sibling("button");
			i++;
		}

		IPoint pos(elem_);
		pos.y += MyApplication::GAME_HEIGHT;
		int width = col_count * _cellSize;
		int height = (i - 1) / col_count * _cellSize + _cellSize;

		setClientRect(IRect(pos.x, pos.y, width, height) );
	}

	void EditorPanel::Update(float dt)
	{
	}

	void EditorPanel::Draw()
	{
		Render::device.PushMatrix();
		Render::device.MatrixTranslate(position + offset_panel);

		_texture->Draw();
		size_t count = _buttons.size();
		for(size_t i = 0; i < count; i++)
		{
			_buttons[i].Draw();
		}
		if(_tooltipExist)
		{
			IPoint text_size = _tooltip->GetSize();
			IRect rect(_tooltipPos.x, _tooltipPos.y, text_size.x, text_size.y);
			rect.Inflate(4);
			Render::device.SetTexturing(false);
			Render::BeginColor(Color(130, 130, 130, 220));
			Render::DrawRect(rect);
			Render::EndColor();
			Render::device.SetTexturing(true);
			_tooltip->Draw(_tooltipPos);			
		}

		Render::device.PopMatrix();
	}

	bool EditorPanel::MouseDown(const IPoint &mouse_pos_)
	{
		IPoint mouse_pos = mouse_pos_ - position - offset_panel;
		if(mouse_pos.y < 0)
		{
			return false;
		}
		EditorUtils::activeEditBtn = None;
		size_t count = _buttons.size();
		for(size_t i = 0; i < count; i++)
		{
			if(_buttons[i].MouseDown(mouse_pos))
			{				
				return true;
			}
		}
		return true;
	}

	void EditorPanel::MouseWheel(int delta)
	{
		if(_scrollable && clientRect.Contains(Core::mainInput.GetMousePos()))
		{
			offset_panel.x = math::clamp(Render::device.Width() - clientRect.width, 0, offset_panel.x + delta * _cellSize);
		}
	}

	void EditorPanel::MouseMove(const IPoint &mouse_pos_)
	{
		IPoint mouse_pos = mouse_pos_ - position - offset_panel;
		size_t count = _buttons.size();
		std::string tooltip = "";
		for(size_t i = 0; i < count; i++)
		{
			tooltip = _buttons[i].MouseMove(mouse_pos);
			if(!tooltip.empty())
			{
				_tooltipPos = IPoint(_buttons[i]._rect.x + 20, -_tooltip->getHeight() - 4);
				break;
			}
		}
		_tooltipExist = !tooltip.empty();
		if(_tooltipExist)
		{
			_tooltip->SetSource(tooltip);
			_tooltipPos.x = math::clamp(-EditorPanel::offset_panel.x, -EditorPanel::offset_panel.x + GameSettings::FIELD_SCREEN_CONST.width - (int)_tooltip->getWidth(), _tooltipPos.x);
		}
	}

	void EditorPanel::MouseUp(const IPoint &mouse_pos_)
	{
	}

	void EditorPanel::AcceptMessage(const Message &message)
	{
	}

} //EditorUtils