#ifndef __TIMEFACTORWIDGET_H__
#define __TIMEFACTORWIDGET_H__

#pragma once

namespace GUI {

//
// Виджет, отвечающий за ускорение/замедление времени
// и за отображение соответствующей информации.
// Работает так: при скролле мыши с зажатым Ctrl ускоряет (или замедляет)
// время и выводит на некоторое время соответствующую надпись.
// 
class TimeFactorWidget
	: public Widget
{

public:

	// Конструктор - чтение из XML
	TimeFactorWidget(const std::string& name, rapidxml::xml_node<>* xmlElement);

	// Рисуем
	virtual void Draw();

	// Обновляем
	virtual void Update(float dt);

	// Реакция на вращение колесика мыши
	virtual void MouseWheel(int delta);

private:

	const float SHOW_TIME;
		// время показа надписи

	const float HIDE_TIME;
		// время скрытия надписи

	float _timer;
		// таймер

	IPoint _position;
		// позиция текста;
		// какой именно части текста касается позиция, зависит от _textFormat;

	enum {
		
		STATE_HIDDEN,
			// состояние "не показывать ничего"

		STATE_SHOWING,
			// состояние "показываем надпись"

		STATE_HIDING,
			// состояние "скрываем надпись быстро"

	} _state;
		// текущее состояние

	std::string _text;
		// текст, который выводим;
		// кэшируется, чтобы не создавать каждый кадр

	// Показать надпись с текущим множителем
	void ShowTitle();
	
	void AcceptMessage(const Message& message);

};

} // namespace GUI

#endif // __TIMEFACTORWIDGET_H__
