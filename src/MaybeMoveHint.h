#ifndef MAYBE_MOVE_HINT_H
#define MAYBE_MOVE_HINT_H

#include "GameFieldAddress.h"

struct ChipForChange
{
	int color;
	IPoint pos;
	int color2;
	IPoint pos2;
	ChipForChange()
		: color(-1)
		, color2(-1)
	{}
};

//namespace Gadgets
//{
//	//Соединить стрелкой две клетки (для подсказки)	
//	void DrawOneArrow(const IPoint &p0, const IPoint &p1, const float &alphaFactor, const float &factor, const float &r = 6.f);
//	//Нарисовать подсказку на одну из последовательностей
//	void DrawArrowSequence(float alphaFactor, float vTime, const AddressVector &seq);
//	void DrawArrowSequence(float alphaFactor, float vTime, const std::vector<IPoint> &seq);
//
//}//namespace Gadgets

struct MaybeCell
{

	bool exist;
	bool dirs[8];
	size_t dirs_count;
	int checked;
	bool ice;
	
	//IPoint pos;
	MaybeCell()
		: exist(false)
		, ice(false)
		, checked(-1)
		, dirs_count(0)
	{
		for(size_t i = 0; i < 8; i++)
		{
			dirs[i] = false;
		}
	};

};

struct Group
{
	IPoint checked; //Проверка группы залачивается определенной фишкой с кординатами checked

	size_t count; //количество фишек в группе

	Group()
		: count(0)
		, checked(-1, -1)
	{
	
	}
	bool IsChecked() const
	{ 
		return checked.x >= 0;
	};

	void SetChecked(const IPoint &pos)
	{
		if(checked.x < 0)
		{
			checked = pos;
		}else if(checked == pos)
		{
			checked = IPoint(-1, -1);
		}
	};
};

class MaybeMoveHint
{
	//Разрешены диагональные цепочки
	bool _allowDiagonal;
	//Необходимо пересчитать ситуацию на экране
	bool _needUpdate;
	
	//Ячейки для ускорения вычисления. После вычисления не используется.
	//Считается что на экран не помещается более 20х20 фишек.
	MaybeCell _maybeMoveCell[20][20];

	//Список фишек готовых для замены
	std::vector<ChipForChange> _chipForChange;
private:

	// Каждый цвет обрабатывается отдельно. Составляется карта, группы
	bool FindForColor(int current_color);

	//Если нет ходов то составляем список того чего можно перекрасить
	bool FindChipsForRecolor();
	
	bool _chainsExist;
public:
	MaybeMoveHint();

	void Draw();

	//Дебажная отрисовка - зажмите SHIFT
	void DrawDebug();

	void Clear();
	
	// Ищем возможные ходы, заполняя набор _maybeMovesFull
	bool Find();	

	//Есть ли ходы? Для инициации решафла.
	bool IsEmpty() const;

	//Необходимо пересчитать поле
	void NeedUpdate();

	//Что будем менять?
	ChipForChange GetChipForChange();
};

#endif //MAYBE_MOVE_HINT_H