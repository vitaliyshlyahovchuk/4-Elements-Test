#pragma once

// Временно оставил только, чтобы не сломать общий код 4EF и CoA
// (конкретно объявления виджетов в MyApplication.cpp).
// Текущий экран загрузки сделан Flash-виджетом.

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

