#pragma once
#include "Place2D.h"
#include "GameFieldController.h"

namespace DetectBorder2D
{
	void InitGame(rapidxml::xml_node<> *description_xml);
	void Draw();

}//namespace DetectBorder2D