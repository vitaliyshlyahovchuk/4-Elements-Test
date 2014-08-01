#ifndef DynamicScroller_h
#define DynamicScroller_h

/*
 *  Scroller.h
 *  80days
 *
 *  Created by Slava on 29.09.10.
 *  Copyright 2010 Playrix Entertainment. All rights reserved.
 *
 */

#include <vector>

// Класс для iphone-подобного скроллинга (вертикального или горизонтального)
// Использование:
//  создаем объект
//  инициализируем размер контента функцией SetContentsSize()
//  посылаем ему MouseMove, MouseUp, MouseDown, Update
//  рисуем контент, используя значение GetPosition()
//  рисуем слайдер, используя GetSliderShiftAndSize()
// Алгоритм позаимствован отсюда: http://www.macresearch.org/dynamics-scrolling
class DynamicScroller
{
public:
	
	// Конструктор:
	// frame - размер видимой части в пикселах
	// bounce - максимально возможный заход контента за границу
	// sliderMinSize - минимальный размер слайдера прокрутки
	//    (для функции GetSliderShiftAndSize())
	DynamicScroller(float frame, float bounce, float sliderMinSize);
	
	// Функции реакции на мышь принимают только одну компоненту точки
	// Причем можно слать тупо mousePos.x, то есть не важно где начало
	// координат, лишь бы оно не изменялось.
	// Внимание! В случае вертикального слайдера, так как по y координаты
	// инвертированы, нужно посылать их с минусом.
	void MouseDown(int pos);
	void MouseUp(int pos);
	void MouseMove(int pos);
	void MouseCancel();
	
    float Sensetive(int pos);
    
	bool Update(float dt);

	// Задать размер контента в пикселах
	void SetContentsSize(float full);
	float GetContentsSize() const;
	float GetVelocity() const;
		
	// Вернуть размер видимого фрейма в пикселах
	// Это актуально для вертикального слайдера
	float GetFrameSize() const { return _frame; }
	void SetFrameSize(float size);
	
    void SetSensetive(float sensetive); //задать расстояние после которого начнется перемещение скролла
	void SetBounce(float bounce);
	void SetMagnetNet(float n);
	void SetMaxPosition(float value);
	float MoveMagnetPos(float m);
	float GetMagnetPos() const { return _magnet_pos; }
	bool IsMaxMagnetPos() const { return _magnet_pos + _magnet_net > 0; }
	bool IsMinMagnetPos() const { return _magnet_pos - _magnet_net < GetMinPosition(); }
	
	// Вернуть сдвиг контента (< 0, если сдвинут выше (правее), 0 - начальное состояние)
	float GetPosition() const { return _position; }
	float GetShiftedPosition() const { return _position - _shiftPosition; }
	float GetShift() const { return static_cast<float>(_shiftPosition); }
	void SetPosition(float position);

	// Возвращает минимально возможное стабильное значение _position
	float GetMinPosition() const { return static_cast<float>(_frame - _full); }

	// Возвращает смещение слайдера вправо (вниз) от начальной позиции и размер в пикселах
	// Внимание: в случае вертикального слайдера для определения абсолютной позиции
	// слайдера вам понадобится GetFrameSize() или заране сохраненное значение высоты
	// видимой части
	void GetSliderShiftAndSize(float &shift, float &size);
	
	bool gotMouse() const { return _gotMouse; }
	void SetMaxVelocity(float velocity);
	void SetDeceleration(float deceleration);
	void SetBounceDeceleration(float deceleration);
    
    void SetShiftPosition(float shift);

	// Постраничный скроллинг
	void SetScrollByPage(bool scrollByPage) { _scrollByPage = scrollByPage; }
	
	// Циклический скроллинг
	void SetCyclic(bool cyclic) { _cyclic = cyclic; }

	void AcceptMessage(const Message &message);

	void Pause();
	void Continue();
	bool IsPaused() const;

private:
	float _maxPosition;
	float _drawPause;
	float _simpleStartPos, _simpleFinishPos;
	bool _simpleMove;
	float _simpleMoveTime, _simpleMoveTimeScale;

    float MIN_SCROLL_DELTA;
	// коэффициент возврата: чем больше, тем быстрее будет возвращаться из-за границы
	static const float RETURN_COEFF;
	// коэффициент торможения или трения: чем больше, тем быстрее будет останавливаться "сам собой"
	float DECELERATION;
	// коэффициент торможения при заходе за границу: должен быть существенно больше чем DECELERATION
	float BOUNCE_DECELERATION;
	// максимальная скорость прокрутки; речь идет только о "свободной" прокрутке, ведь когда пользователь
	// удерживает область мышью (пальцем или что у него там), мы должны сместиться туда мгновенно
	float MAX_VELOCITY;
	// размер очереди последних позиций мыши
	static const size_t MAX_QUEUE_SIZE;
	// размер кадра в пикселах
	float _frame;
	// Полная высота содержимого
	float _full;
    //
    float _shiftPosition;
	// зажат ли палец
	bool _tapped;
	//
	bool _hasPrevPos;
	//
	bool _gotMouse;
	// максимальный "выход" контента за границы
	float _bounce;
	// предыдущая позиция мыши (или пальца, вотэвер)
	float _previousMousePos;
	// текущая позиция мыши
	float _currentMousePos;
	// позиция нажатия
	float _mouseDownPos;
	// текущая скорость
	float _velocity;
	// текущая позиция (от 0 до GetMinPosition())
	float _position;
	
	float _magnet_pos;
	float _magnet_net;
	float _magnet_vel_lim;
	// минимальный размер слайдера
	float _sliderMinSize;
	
	std::vector<float> _speedBuf;
	
	// Обновляем позицию скролла и скорость
	void UpdatePosition(float dt);
	
	// Возвращает сглаженную позицию мыши
	float SmoothVelocity();

	bool _scrollByPage;

	bool _cyclic;

	bool _paused;
};

#endif