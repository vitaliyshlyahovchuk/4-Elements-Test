﻿
#pragma once

namespace Match3
{
	// Вызывается при старте уровня
	void Init();

	// Задает прямоугольник в котором будут обновляться фишки
	// genChipsOnTop - будут генерироваться фишки по всей верхней границе заданного прямоуголника,
	// даже если обычно их там быть не должно (для окончания уровня)
	void SetRect(int left, int right, int bottom, int top, bool genChipsOnTop = false);

	// Запланировать обновление колонки в конце текущего кадра
	void RunFallColumn(int col);

	// Запустить просчет всех колонок
	void FallColumnsOnUpdate();
	
	// Мгновенно заполнить уровень фишками с учетом того, где они могут или не могут оказаться
	// при диагональной механике и обычном заполнении
	void FillLevel();

	// если возможно, очищает прямоугольник и запускает генерацию и падение новых фишек в нем
	void RefillRect();

	// есть ли столбцы, которые возможно упадут
	bool IsActive();

	// Рассчитывает куда могут упасть фишки в данный момент, выставляет флаг willFallChip в Game::Square
	void PrecalcFall();

	// Прекратить/возобновить расчет падения фишек
	void Pause();
	void Unpause();
}