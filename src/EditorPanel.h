#pragma once
#include "GUI/Widget.h"
#include "EditorUtils.h"

namespace EditorUtils
{
	struct ButtonInfo
	{
		EditButtons _id;
		IRect _rect;
		std::string _tooltip_text;

		ButtonInfo(IRect rect, rapidxml::xml_node<>* xml_elem);
		std::string MouseMove(const IPoint &mouse_pos);
		bool MouseDown(const IPoint &mouse_pos);
		void MouseUp(const IPoint &mouse_pos);
		void Draw();
	};

	class EditorPanel
		: public GUI::Widget
	{
		Render::Texture *_texture;			
		std::vector<ButtonInfo> _buttons;
		Render::Text *_tooltip;
		IPoint _tooltipPos;
		bool _tooltipExist;
		bool _scrollable;
		int _cellSize;
		IPoint offset_panel;

	public:
		EditorPanel(const std::string& name_, rapidxml::xml_node<>* elem_);
		void Update(float dt);
		void Draw();
		bool MouseDown(const IPoint &mouse_pos);
		void MouseMove(const IPoint &mouse_pos);
		void MouseWheel(int delta);
		void MouseUp(const IPoint &mouse_pos);
		void AcceptMessage(const Message &message);
	};

}//EditorUtils