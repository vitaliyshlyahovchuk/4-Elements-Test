#pragma once

#include "Ryushka.h"

class RyushkaFaerie
	: public Ryushka
{
	IRect _clientRect;
	enum{
		RF_HIDE,
		RF_SHOW,
	}_state;
	bool _debug_editor;
	std::string  _type;
	std::string  _function;

	void Show();
	void Hide();

public:
	RyushkaFaerie(rapidxml::xml_node<>* xe);
	~RyushkaFaerie();
	void Update(float dt);
	void OnDraw();
	void SaveToXml(Xml::TiXmlElement *parentElem);
	IRect getClientRect();
	IRect getVisibleRect();
	void setPosition(const IPoint &);
};