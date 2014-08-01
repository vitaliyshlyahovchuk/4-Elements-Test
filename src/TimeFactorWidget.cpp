#include "stdafx.h"
#include "TimeFactorWidget.h"

namespace GUI {

TimeFactorWidget::TimeFactorWidget(const std::string& name, rapidxml::xml_node<>* xmlElement)
	: Widget(name)
	, SHOW_TIME(1.5f)
	, HIDE_TIME(0.1f)
	, _position(xmlElement->first_node("position"))
	, _timer(0.0f)
	, _state(STATE_HIDDEN)
{
}

void TimeFactorWidget::Draw()
{
	if (_state == STATE_SHOWING) {
		Render::FreeType::BindFont("EdirotTexts");
		Render::PrintString(_position, _text, 1.0f, CenterAlign, CenterAlign, false);
	} else if (_state == STATE_HIDING) {
		Render::FreeType::BindFont("EdirotTexts");
		Render::BeginAlphaMul(1 - _timer / HIDE_TIME);
		Render::PrintString(_position, _text, 1.0f, CenterAlign, CenterAlign, false);
		Render::EndAlphaMul();
	}
}

void TimeFactorWidget::Update(float dt)
{
	if (_state == STATE_SHOWING) {
		dt = dt / Core::timeFactor.GetValue();
		_timer += dt;
		if (_timer > SHOW_TIME) {
			_state = STATE_HIDING;
			_timer = 0;
		}
	} else if (_state == STATE_HIDING) {
		dt = dt / Core::timeFactor.GetValue();
		_timer += dt;
		if (_timer > HIDE_TIME) {
			_state = STATE_HIDDEN;
			_timer = 0;
		}
	}
}

void TimeFactorWidget::MouseWheel(int delta)
{
	if (Core::mainInput.IsControlKeyDown()) {
		if (delta < 0) {
			Core::timeFactor.Decrease();
			ShowTitle();
		} else if (delta > 0) {
			Core::timeFactor.Increase();
			ShowTitle();
		}
	}
}

void TimeFactorWidget::ShowTitle()
{
	_state = STATE_SHOWING;
	_timer = 0.0f;
	_text = "Speed: " + Core::timeFactor.ToString() + " x";
}
	
void TimeFactorWidget::AcceptMessage(const Message& message)
{
	if(message.is("TimeFactor:Inc"))
	{
		Core::timeFactor.Increase();
	}
	else if(message.is("TimeFactor:Dec"))
	{
		Core::timeFactor.Decrease();
	}
}


} // namespace GUI