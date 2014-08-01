#pragma once

#include "GUI/Widget.h"
#include "DynamicScroller.h"
//#include "CheckBoxEx.h"

class DebugSelectLevel : public GUI::Widget
{
public:
	DebugSelectLevel(std::string name, rapidxml::xml_node<>* xmlElement);
	virtual void Draw();
	virtual bool MouseDown(const IPoint &mouse_pos);
	virtual void MouseUp(const IPoint &mouse_pos);
	void MouseMove(const IPoint &mouse_pos);
	virtual void AcceptMessage(const Message &message);
	void MouseWheel(int delta);
	virtual Message QueryState(const Message& message) const;
	void Update(float dt);

private:

	struct Button {
		Button(const std::string& message, const std::string& text, const Color clr = Color(0, 100, 0, 255));
		void Draw();
		bool MouseDown(const IPoint&);
		void MouseUp(const IPoint&);
		void SetRect(const IRect&);

		Color _color;
		IRect rect;
		bool pressed;
		std::string message;
		std::string text;
	};
	bool _alphaBetType;
	bool _appearMove;
	bool _appearOnScreen;
	float _appearTimer;
	Button select;
	Button left;
	Button right;
	int current;
	IRect _screenRect;
	int _font_h;
	bool _fileListActive;
	DynamicScroller _scroll;

	bool _mouseDown;
	IPoint _mouseDownPos;

	std::vector<std::string> fileNames;
	TextAlign fileNameAlign;

	void ReloadNames();
	void SetLevel(const std::string& levelName);
	bool IsButtonsActive() const;
	bool IsAlphaBetType();
};