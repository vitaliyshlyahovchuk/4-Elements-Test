#pragma once

#include "GUI/Widget.h"

class EditorFileList : public GUI::Widget
{
	std::vector<std::string> _names;
	std::string  _fileName;
	int _cursorPos;
	float _cursorTime;
	int _currentName;
	int _underMouseName;
	int textXLeft, textYUp, textYDown, textDx, textDy;
	std::string  _destLayer;
	std::string  _destWidget;
	int _offsetColumn, _columnCount, _columnMax;
public:
	EditorFileList(std::string name_, rapidxml::xml_node<>* xmlElement);

	virtual bool MouseDown(const IPoint &mouse_pos);
	virtual void MouseUp(const IPoint &mouse_pos);
	virtual void MouseMove(const IPoint &mouse_pos);
	virtual void MouseDoubleClick(const IPoint &mouse_pos);
	virtual void Draw();
	virtual void Update(float dt);
	virtual void AcceptMessage(const Message &message);
	void MouseWheel(int delta);
};

// Окошко в редакторе, в котором происходит выбор набора фишек для уровня
class ChipSelecter : public GUI::Widget
{
	Render::Texture *_chipsTex;
	std::vector<int> _chipColors;
	int _chipXLeft;
	int _chipYDown;
	float _timer;
	int _underMouseChip;

public:
	ChipSelecter(std::string name_, rapidxml::xml_node<>* xmlElement);
	void DrawRamka(int x, int y);

	virtual bool MouseDown(const IPoint &mouse_pos);
	virtual void MouseDoubleClick(const IPoint &mouse_pos);
	virtual void MouseUp(const IPoint &mouse_pos);
	virtual void MouseMove(const IPoint &mouse_pos);

	virtual void Draw();
	virtual void Update(float dt);
	virtual void AcceptMessage(const Message &message);
};
