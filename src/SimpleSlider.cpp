#include "stdafx.h"
#include "SimpleSlider.h"

#include "SomeOperators.h"

SimpleSlider::SimpleSlider(IPoint pos, int size, bool isVertical, int begunWidth, int begunHeight)
	: _pos(pos)
	, _size(size)
	, _isVertical(isVertical)
	, _begunWidth(begunWidth)
	, _begunHeight(begunHeight)
	, _begunPos(0)
	, _isMouseDown(false)
	, _isMouseOnSlider(false)
{
	Assert(_size > 0);
	Assert(_begunWidth > 0);
	Assert(_begunHeight > 0);
}

float SimpleSlider::GetFactor()
{
	return (_begunPos+0.f)/_size;
}

void SimpleSlider::SetFactor(float factor)
{
	Assert(0.0f <= factor && factor <= 1.0f);
	_begunPos = math::round(_size * factor);
}

void SimpleSlider::SetPosition(IPoint pos) {
	_pos = pos;
}

void SimpleSlider::SetSize(int size) {
	Assert(size > 0);
	
	// переcчитываем новое положение бегунка:
	_begunPos = _begunPos * size / _size;

	_size = size;
}

void SimpleSlider::Draw()
{
	if (_isMouseOnSlider) {
		Render::BeginColor(Color(0, 200, 0));
	} else {
		Render::BeginColor(Color(255, 255, 255));
	}
	Render::device.SetTexturing(false);
	if (_isVertical) {
		Render::DrawRect(IRect(_pos.x, _pos.y, 1, _size));
		Render::DrawRect(IRect(_pos.x - _begunWidth/2, _pos.y + _begunPos - _begunHeight/2, _begunWidth, _begunHeight));
	} else {
		Render::DrawRect(IRect(_pos.x, _pos.y, _size, 1));
		Render::DrawRect(IRect(_pos.x + _begunPos - _begunWidth/2, _pos.y - _begunHeight/2, _begunWidth, _begunHeight));
	}
	Render::device.SetTexturing(true);
	if (!_isVertical) {
		std::string  label = _label;
		if (label == "") {
			label = _labelPrefix + FloatToString(GetFactor(), 2);
		}
		// подпиcь выводим только для горизонтальных cлайдеров
		Render::FreeType::BindFont("debug");
		Render::PrintString(_pos.x + _size + 10, _pos.y, label, 1, LeftAlign, BaseLineAlign);
	}
	Render::EndColor();
}


void SimpleSlider::SetBegunPos(const IPoint& mousePos) {
	if (_isVertical) {
		if (mousePos.y < _pos.y) {
			_begunPos = 0;
		} else if (mousePos.y < _pos.y + _size) {
			_begunPos = mousePos.y - _pos.y;
		} else {
			_begunPos = _size;
		}
	} else {
		if (mousePos.x < _pos.x) {
			_begunPos = 0;
		} else if (mousePos.x < _pos.x + _size) {
			_begunPos = mousePos.x - _pos.x;
		} else {
			_begunPos = _size;
		}
	}
}

bool SimpleSlider::IsMouseOnSlider(const IPoint &mousePos) {
	if (_isVertical) {
		if (mousePos.x >= _pos.x - _begunWidth/2 && mousePos.x < _pos.x + _begunWidth/2 &&
			mousePos.y >= _pos.y - _begunHeight/2 && mousePos.y < _pos.y + _size + _begunHeight/2)
		{
			return true;
		}
	} else {
		if (mousePos.x >= _pos.x - _begunWidth/2 && mousePos.x < _pos.x + _size + _begunWidth/2 &&
			mousePos.y >= _pos.y - _begunHeight/2 && mousePos.y < _pos.y + _begunHeight/2)
		{
			return true;
		}
	}
	return false;
}

bool SimpleSlider::GetIsMouseDown() {
	return _isMouseDown;
}

bool SimpleSlider::GetIsMouseOnSlider() {
	return _isMouseOnSlider;
}

void SimpleSlider::MouseWheel(int delta)
{
	if(!_isMouseDown){
		_begunPos = math::clamp(0, _size, _begunPos + delta*20);
	}
}

bool SimpleSlider::MouseDown(const IPoint &mousePos)
{
	_isMouseDown = IsMouseOnSlider(mousePos);
	if (_isMouseDown) {
		SetBegunPos(mousePos);
	}
	return _isMouseDown;
}

void SimpleSlider::MouseUp(const IPoint &mousePos) {
	_isMouseDown = false;
}

bool SimpleSlider::MouseMove(const IPoint &mousePos)
{
	if (_isMouseDown) {
		SetBegunPos(mousePos);
		return true;
	} else {
		_isMouseOnSlider = IsMouseOnSlider(mousePos);
	}
	return false;
}

void SimpleSlider::SetLabelPrefix(std::string  labelPrefix) {
	_labelPrefix = labelPrefix;
}

void SimpleSlider::SetLabel(std::string  label) {
	_label = label;
}

IPoint SimpleSlider::GetBegunPos()
{
	if (_isVertical) {
		return IPoint(0, _begunPos);
	} else {
		return IPoint(_begunPos, 0);
	}
}


float SimpleSlider::GetSize()
{
	return _size + 0.f;
}