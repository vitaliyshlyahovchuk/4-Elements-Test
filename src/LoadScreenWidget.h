#pragma once

// �������� ������� ������, ����� �� ������� ����� ��� 4EF � CoA
// (��������� ���������� �������� � MyApplication.cpp).
// ������� ����� �������� ������ Flash-��������.

class LoadScreenWidget
	: public GUI::Widget
{
public:
	LoadScreenWidget(const std::string& name_, rapidxml::xml_node<>* xmlElement)
		: Widget(name_)
	{
	}

	void Update(float dt) {}
	void Draw() {}
	void AcceptMessage(const Message& message) {}
};

