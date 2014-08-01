/*
 *  Scroller.cpp
 *  80days
 *
 *  Created by Slava on 29.09.10.
 *  Copyright 2010 Playrix Entertainment. All rights reserved.
 *
 */

#include "stdafx.h"
#include "DynamicScroller.h"

const float DynamicScroller::RETURN_COEFF = 5.0f;
const size_t DynamicScroller::MAX_QUEUE_SIZE = 5;

DynamicScroller::DynamicScroller(float frame, float bounce, float sliderMinSize)
	: _frame(frame)
	, _full(frame)
	, _bounce(bounce)
	, _tapped(false)
	, _hasPrevPos(false)
	, _gotMouse(false)
	, _previousMousePos(0)
	, _currentMousePos(0)
	, _mouseDownPos(0)
	, _velocity(0)
	, _position(0)
	, _magnet_pos(0)
	, _magnet_net(0)
	, _sliderMinSize(sliderMinSize)
	, _scrollByPage(false)
	, _cyclic(false)
	, MIN_SCROLL_DELTA(0.9f)
	, MAX_VELOCITY(700.f)
	, DECELERATION(100.0f)
    , BOUNCE_DECELERATION(1000.0f)
	, _simpleMove(false)
	, _maxPosition(0)
	, _simpleMoveTimeScale(1.f)
	, _paused(false)

{
	Assert(_frame > 0);
	_speedBuf.resize(MAX_QUEUE_SIZE);
	_magnet_vel_lim = math::sqrt(_magnet_net * DECELERATION);
    _shiftPosition = 0;
}

void DynamicScroller::SetMaxPosition(float value) {
	_maxPosition = value;
}

void DynamicScroller::SetShiftPosition(float shift) {
    _shiftPosition = shift;
}

void DynamicScroller::SetSensetive(float sensetive) {
    MIN_SCROLL_DELTA = sensetive;
}

float DynamicScroller::Sensetive(int pos) {
	float fpos = static_cast<float>(pos);
    if (fpos - _mouseDownPos > MIN_SCROLL_DELTA) {
        return fpos - MIN_SCROLL_DELTA;
    } else if (fpos - _mouseDownPos < -MIN_SCROLL_DELTA) {
        return fpos + MIN_SCROLL_DELTA;
    } else if (MIN_SCROLL_DELTA == 0) {
        return fpos;
    }
    return 0;
}


void DynamicScroller::MouseCancel() 
{
	if(_simpleMove || IsPaused())
	{
		return;
	}
	_tapped = false;
	//_hasPrevPos = false;
	_gotMouse = false;
}

void DynamicScroller::MouseUp(int pos)
{
	if(_simpleMove || IsPaused())
	{
		return;
	}
	if ( _tapped ) {
		float fpos = Sensetive(pos);
        if (fpos != 0) {
            if ( !_hasPrevPos ) {
                _previousMousePos = _currentMousePos;
                _hasPrevPos = true;
            }
            _currentMousePos = fpos;
        }
		_tapped = false;
	}
	_gotMouse = false;
}

void DynamicScroller::MouseDown(int pos)
{
	if(_simpleMove || IsPaused())
	{
		return;
	}
	float fpos = static_cast<float>(pos);
	_tapped = true;
	_mouseDownPos = fpos;
	_previousMousePos = fpos;
	_currentMousePos = fpos;
	_hasPrevPos = false;
}

void DynamicScroller::MouseMove(int pos)
{
	if(_simpleMove || IsPaused())
	{
		return;
	}
	if(_tapped)
	{
		float fpos = Sensetive(pos);
        if (fpos != 0) {
            _currentMousePos = fpos;
            _hasPrevPos = true;
            if ( !_gotMouse && _currentMousePos != _mouseDownPos ) {
                _gotMouse = true;
            }
        }
	}
}

void DynamicScroller::UpdatePosition(float dt)
{
	if(_simpleMove)
	{
		_simpleMoveTime += dt*_simpleMoveTimeScale;
		if(_simpleMoveTime >= 1)
		{
			_simpleMoveTime = 1.f;			
			_simpleMove = false;

			Core::guiManager.getLayer("CardLayer")->getWidget("CardWidget")->AcceptMessage(Message("EndVisualization", "ScrollTo"));
		}
		//float t = math::sin(_simpleMoveTime * math::PI * 0.5f);
		float t = math::ease(_simpleMoveTime, 0.3f, 0.3f);
		SetPosition(math::lerp(_simpleStartPos, _simpleFinishPos, t));
		return;
	}
	float minPosition = GetMinPosition();
	if (_cyclic) {
		_maxPosition += _magnet_net;
		minPosition -= _magnet_net;
	}
	const float maxBouncedPosition = _maxPosition + _bounce;
	const float minBouncedPosition = minPosition - _bounce;
	bool was_in_tapped = false;
	bool free_scroll = false;
	if (_tapped || _hasPrevPos) {
		if ( _hasPrevPos ) {
			_velocity = (_currentMousePos - _previousMousePos) / dt;
		}
		_previousMousePos = _currentMousePos;
		_hasPrevPos = _tapped;
		
		// Если докрутили до границы и продолжаем от нее удаляться, замедляем скорость
		if (_position > _maxPosition && _velocity > 0) {
			_velocity *= (maxBouncedPosition - _position) / _bounce;
		} else if (_position < minPosition && _velocity < 0) {
			_velocity *= (_position - minBouncedPosition) / _bounce;
		}
		was_in_tapped = true;
	} else {
		if (_position > _maxPosition) {
			// Если достигли верхней границы, баунсим назад
			if (_velocity <= 0) {
				// Плавно возвращаемся к нулю
				_velocity = RETURN_COEFF * (_maxPosition - _position);
			} else {
				// Замедляемся, чтобы вернуться назад
				float change = BOUNCE_DECELERATION * dt;
				_velocity -= change;
			}
			if ( _velocity * dt < (_maxPosition - _position) ) {
				_velocity = (_maxPosition - _position)/dt;
			}
		} else if (_position < minPosition) {
			// Если достигли нижней границы, баунсим назад
			if (_velocity >= 0) {
				// Возвращаемся к нижней границе
				_velocity = RETURN_COEFF * (minPosition - _position);
			} else {
				// Замедляемся
				float change = BOUNCE_DECELERATION * dt;
				_velocity += change;
			}
			if ( _velocity * dt > (minPosition - _position) ) {
				_velocity = (minPosition - _position)/dt;
			}
		} else {
			_velocity = math::clamp(-MAX_VELOCITY, MAX_VELOCITY, _velocity);
			if ( math::abs(_velocity) > _magnet_vel_lim ) {
				// Обычный скролл с замедлением
				float changeVelocity = DECELERATION * dt;
				if (changeVelocity > math::abs(_velocity)) {
					_velocity = 0;
				} else {
					_velocity -= (_velocity > 0 ? changeVelocity : -changeVelocity);
				}
				free_scroll = true;
			}
			else if ( _velocity != 0.f || _magnet_pos != _position ) {
				_velocity += BOUNCE_DECELERATION * dt * math::sign(_magnet_pos - _position);
				if ( math::abs(_velocity * dt) > math::abs(_magnet_pos - _position) ) {
					_velocity = (_magnet_pos - _position) / dt;
				}
				if ( math::abs(_velocity) > _magnet_vel_lim ) {
					_velocity = _magnet_vel_lim * math::sign(_velocity);
				}
			}
		}
	}
	float delta_pos = _velocity * dt;
	_position += delta_pos;
	_speedBuf.erase(_speedBuf.begin());
	_speedBuf.push_back(_velocity);
	if (_cyclic) {
		if (_velocity > 0) {
			if (_position > 0) {
				_position = GetMinPosition() - (_magnet_net - (_position - 0));
				_magnet_pos = (GetMinPosition()/_magnet_net) * _magnet_net;
				_maxPosition = 0;
			}
		} else if (_velocity < 0) {
			if (_position < GetMinPosition()) {
				_position = 0 + (_magnet_net - (GetMinPosition() - _position));
				_magnet_pos = (0/_magnet_net)*_magnet_net;
				minPosition = GetMinPosition();
			}
		}
	}
	if ( was_in_tapped ) {
		_velocity = SmoothVelocity(); // restore velocity
	}
	if (_position > maxBouncedPosition) {
		_position = maxBouncedPosition;
		_velocity = 0;
	}
	else if (_position < minBouncedPosition) {
		_position = minBouncedPosition;
		_velocity = 0;
	}
	if ( was_in_tapped ) {
		_magnet_pos = math::round(_position / _magnet_net) * _magnet_net;
		if (_scrollByPage) {
			float old_magnet_pos = _magnet_pos;
			if (math::abs(_velocity) > 100.f) {
				_magnet_pos = (math::round(_position / _magnet_net) + math::sign(_velocity)) * _magnet_net;
			} else {
				_magnet_pos = math::round(_position / _magnet_net) * _magnet_net;
			}
			if (old_magnet_pos - _magnet_pos > 0) {
				_magnet_pos = old_magnet_pos - _magnet_net;
			} else if (old_magnet_pos - _magnet_pos < 0) {
				_magnet_pos = old_magnet_pos + _magnet_net;
			}
		}
	}
	if ( free_scroll ) {
		_magnet_pos = math::round(_position / _magnet_net + math::sign(_velocity)*0.5f) * _magnet_net;
	}
	if ( _magnet_pos > _maxPosition ) {
		_magnet_pos = (_maxPosition/_magnet_net)*_magnet_net;
	}
	if ( _magnet_pos < minPosition ) {
		_magnet_pos = (minPosition/_magnet_net)*_magnet_net;
	}
	if ( math::abs(_magnet_pos - _position) < 0.1f ) {
        _position = _magnet_pos; // На случай, если _position == 1e-18.
    }
}

bool DynamicScroller::Update(float dt)
{
	if (!IsPaused() && dt > 0) {
		float position = GetPosition();
		UpdatePosition(dt);
		return (math::abs(position - GetPosition()) > 1e-5);
	}
	return false;
}

void DynamicScroller::SetPosition(float position)
{
	_position = position;
	_magnet_pos = math::round(_position / _magnet_net) * _magnet_net;
	_velocity = 0;
}

void DynamicScroller::SetContentsSize(float full)
{
	Assert(full >= 0);
	_full = full;
}

float DynamicScroller::GetContentsSize() const
{
	return _full;
}

float DynamicScroller::GetVelocity() const
{
	return _velocity;
}
	
void DynamicScroller::SetFrameSize(float size) {
	_frame = size;
	SetMagnetNet(_magnet_net);
}

void DynamicScroller::SetBounce(float bounce) {
	_bounce = bounce;
}

void DynamicScroller::SetMagnetNet(float n) {
	_magnet_net = n;
	_magnet_vel_lim = math::sqrt(_magnet_net * DECELERATION);
	_frame = math::round(_frame / _magnet_net) * _magnet_net;
}

float DynamicScroller::MoveMagnetPos(float m) {
	if ( math::abs(_magnet_pos - _position) > _magnet_net ) return 0;
	_magnet_pos += m * _magnet_net;
	if (!_cyclic) {
		if ( _magnet_pos < GetMinPosition() ) {
			_magnet_pos = GetMinPosition();
		}
		if ( _magnet_pos > 0 ) {
			_magnet_pos = 0;
		}
	}
	float res = _magnet_pos / _magnet_net;
	_magnet_pos = res * _magnet_net;
	return res;
}

void DynamicScroller::GetSliderShiftAndSize(float & shift, float & size) {
	size = static_cast<float>(math::round(_frame / _full * _frame));
	if (_position > 0) {
		size -= _position;
	} else if (_position < GetMinPosition()) {
		size -= (GetMinPosition() - _position);
	}
	size = std::max(_sliderMinSize, size);
	
	shift = static_cast<float>(math::round((-_position / _full) * _frame));
	if (_position > 0) {
		shift = 0;
	} else if (_position < GetMinPosition()) {
		shift += (GetMinPosition() - _position);
	}
	shift = std::min(shift, _frame - size);
}

float DynamicScroller::SmoothVelocity() {
	float sum = 0.f;
	for ( std::vector<float>::iterator it = _speedBuf.begin(); it != _speedBuf.end(); ++it ) {
		sum += *it;
	}
	return sum/_speedBuf.size();
}

void DynamicScroller::SetMaxVelocity(float velocity) {
	MAX_VELOCITY = velocity;
}

void DynamicScroller::SetDeceleration(float deceleration) {
	DECELERATION = deceleration;
}

void DynamicScroller::SetBounceDeceleration(float deceleration) {
	BOUNCE_DECELERATION = deceleration;
}

void DynamicScroller::AcceptMessage(const Message &message)
{
	if(message.is("ScrollTo"))
	{
		float new_pos = message.getVariables().getFloat("position");
		if(new_pos > _maxPosition)
		{
			MyAssert(false);
		}
		if(GetPosition() != new_pos)
		{
			MouseCancel();
			_velocity = 0.0f;
			_simpleMove = true;
			_simpleMoveTime = 0.f;
			_simpleStartPos = GetPosition();
			_simpleFinishPos = new_pos;
			if( message.getVariables().findName("timeScale") )
				_simpleMoveTimeScale = message.getVariables().getFloat("timeScale");
			else
				_simpleMoveTimeScale = 1.0f;
		}
	}
}

void DynamicScroller::Pause()
{
	_paused = true;

	_velocity = 0.0f;
}

void DynamicScroller::Continue()
{
	_paused = false;
}

bool DynamicScroller::IsPaused() const
{
	return _paused;
}