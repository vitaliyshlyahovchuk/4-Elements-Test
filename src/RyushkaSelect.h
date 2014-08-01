#pragma once
#include "GUI/Widget.h"
#include "Ryushka.h"

const int RYUSHKA_TABS_COUNT = 5;

class RyushkaSelect : public GUI::Widget
{
private:
	int _activeTab;

	std::vector<std::string> _tabs[RYUSHKA_TABS_COUNT];

	float _time;
	GUI::Widget *_list;
	Render::Texture *_texture;

	std::string  _activeRyushka;
	Ryushka::HardPtr _tempRyushka;

public:
	RyushkaSelect(std::string name_, rapidxml::xml_node<>* xmlElement);
	virtual void Draw();
	virtual void Update(float dt);
	virtual bool MouseDown(const IPoint &mouse_pos);
	virtual void MouseUp(const IPoint &mouse_pos);
	virtual void MouseMove(const IPoint &mouse_pos);
	virtual void AcceptMessage(const Message &message);
};
