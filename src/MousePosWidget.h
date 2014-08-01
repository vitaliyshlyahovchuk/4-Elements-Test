#pragma once

// Контрол для отриcовки положения мыши
// Позиция контрола задаёт левый нижний угол выводимых координат
class MousePosWidget
	: public GUI::Widget
{
public:
	MousePosWidget(const std::string& name, rapidxml::xml_node<>* xe);
	void AcceptMessage(const Message &message);
private:

	// Хэлп если нужен
	// Указывается путь к файлу относительно папки "base" (с расширением) Например: Editor/Help.txt
	std::string _helpFile;

	// название шрифта для отображения положения мыши
	std::string  _fontName;

	virtual void Draw();
};
