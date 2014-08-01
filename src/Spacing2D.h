#pragma once

//
// ���cc ��� ��c����� �� ���c��c��
//
struct Spacing2D
{
	
	int left;
	
	int right;
	
	int bottom;

	int top;
	
	Spacing2D(int left_, int right_, int bottom_, int top_);
	
	//
	// ������ ���c���� XML
	//
	Spacing2D(Xml::TiXmlElement* xe);
	Spacing2D(rapidxml::xml_node<>* xe);
};
