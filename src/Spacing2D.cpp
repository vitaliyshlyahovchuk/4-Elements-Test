#include "stdafx.h"
#include "Spacing2D.h"

Spacing2D::Spacing2D(int left_, int right_, int bottom_, int top_)
	: left(left_)
	, right(right_)
	, bottom(bottom_)
	, top(top_)
{
}

Spacing2D::Spacing2D(Xml::TiXmlElement* xe)
	: left(Int::Parse(xe->Attribute("left")))
	, right(Int::Parse(xe->Attribute("right")))
	, bottom(Int::Parse(xe->Attribute("bottom")))
	, top(Int::Parse(xe->Attribute("top")))
{
}

Spacing2D::Spacing2D(rapidxml::xml_node<>* xe)
	: left(Int::Parse(xe->first_attribute("left")->value()))
	, right(Int::Parse(xe->first_attribute("right")->value()))
	, bottom(Int::Parse(xe->first_attribute("bottom")->value()))
	, top(Int::Parse(xe->first_attribute("top")->value()))
{
}
