#pragma once

#include "GUI/Widget.h"

class ClickMessageEdit : public GUI::Widget
{
private:

	std::string  _tempStr;
	std::string  _from;

	float _cursorTime;

	int _cursorPos;

	int _deltaX;
	int _deltaY;

public:
	ClickMessageEdit(std::string name_, rapidxml::xml_node<>* xmlElement);

	virtual void Draw();
	virtual void Update(float dt);

	virtual void AcceptMessage(const Message &message);
};