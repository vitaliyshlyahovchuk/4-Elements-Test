#ifndef COMBOBOX_H
#define _H

#include "GUI/Widget.h"

struct EditList
{
	std::vector<std::string> _values;
	std::string _font;
	int _cellHeight;
	IRect _rect;
	int _selected;
	void Draw();
	void Init(const std::string &font);
	void Add(const std::string &value);
	void Calc(const IPoint &pos);
	std::string MouseDown(const IPoint &mouse_pos);
	void MouseMove(const IPoint &mouse_pos);
};

class Combobox
	:public GUI::Widget
{
	EditList _itemList;
	std::string _title;
public:
	Combobox(const std::string &name_, rapidxml::xml_node<>* xmlElement);
	~Combobox();
	void Draw();
	void Update(float dt);
	void AcceptMessage(const Message &message);
	void MouseMove(const IPoint &mouse_pos);
	bool MouseDown(const IPoint &mouse_pos);
	void MouseUp(const IPoint &mouse_pos);
	void MouseDoubleClick(const IPoint &mouse_pos);

};

#endif //COMBOBOX_H
