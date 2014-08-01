#ifndef ONCE_FUUU_TESTER
#define ONCE_FUUU_TESTER

#include "Array2D.h"
#include <queue>

enum FUUU_ChipType
{
	fuuu_normal_chip = 1,
	fuuu_empty_chip = 2,
	fuuu_licorice_chip = 3
};

enum FUUU_CellType
{
	fuuu_block = 1, //границы поля
	fuuu_empty = 2,
	fuuu_wall = 3,
	fuuu_wood = 4,
	fuuu_stone = 6,
	fuuu_order = 7
};

class FUUUBonus
{
public:
	int  _arrowBonusCount; //число бонусов типа стрелка
	int _arrowRadius;    //радиус стрелки
	int _arrowDirs;      //направление
	Game::FieldAddress _fromCell; //откуда стреляет (не из текущей ячейки только если бонус активируемый бонусом)

	FUUUBonus() : _arrowBonusCount(0)
	{
	}

	bool HasArrowBonus()
	{
		return _arrowBonusCount>0;
	}

	void Clear()
	{
		_arrowBonusCount = 0;
	}

};

class FUUUOrder
{
public:
	int _orderObjective; //цель
	int _targetCount;                      //целевое количество
	int _madeCount;                        //сделанное количество
	bool _clearAfterMove;                  //чистить после хода
	Game::FieldAddress _address; //клетка где он расположен
};

typedef std::list<FUUUOrder> FUUUOrders;

class FUUUCell
{
public:
	FUUU_CellType _cellType;    //тип ячейки
	int _wallHeight;  //высота стены
	int _woodHeight;  //высота леса

	FUUU_ChipType _chipType;    //тип фишки
	int _color;       //цвет фишки
	bool _isIce;      //фишка во льду
	bool _isEnergyWall; //стена убираемая только энергией

	FUUUBonus _bonus; //бонусы

	bool _isEnergy;   //есть энергия
	bool _isReciever; //есть получатель
	int _componentNo; //номер связной компоненты (пока не используется)
	bool _visited;    //посещена (в рамках перебора ходов)
	bool _affectedByBonus; //к ячейке применен бонус (в рамках текущего хода)
	bool _destroyed;	//была ли клетка разрушена в этом ходу (матчем, бонусом, прикосновением - может быть только 1 раз)

	FUUUCell() : _chipType(fuuu_empty_chip), _color(-1), _cellType(fuuu_block), _isEnergy(false), _isReciever(false), 
		_componentNo(0), _visited(false), _isIce(false), _isEnergyWall(false), _bonus(), _destroyed()
	{ 
	}

	//можно ли сюда ходить (в рамках перебора ходов)
	bool CanMoveHere();
	//может ли попасть сюда энергия (в рамках распространения энергии)
	bool CanEnergyMoveHere();
	//можно ли упасть сюда
	bool CanFallHere();
	//может ли выпасть фишка отсюда
	bool ThisChipCanFall();
	//клетка блокирует падение
	bool BlockFalling();
	//обработка попадания ARROW. возвращает true если останавливает стрелку. модифицирует Bonuses если появился новый
	bool ProcessArrow(std::queue<FUUUBonus> &Bonuses, FUUUOrders& o1, int direction);
	//ломание стены при матче, бонусе и т.п.
	void BreakWall(FUUUOrders& o1);
	//ломание леса
	void BreakWood(FUUUOrders& o1);
	//ломание плит
	void BreakStone();
	//"прикосновение" (вызывается при матче рядом)
	void Touch(FUUUOrders& o1);
};

typedef Array2D<FUUUCell> FUUUField ;
typedef std::set<Game::FieldAddress> MoveCoordsList;

class FUUUMove
{
public:
	MoveCoordsList _coords; //координаты хода
	MoveCoordsList _bonusCoords; //координаты клеток где есть бонусы
	Game::FieldAddress _startBonus; //откуда начинается бонус
	int _arrowBonusesCount;  //количество бонусов ARROW
	int _arrowBonusesRadius; //радиус бонусов
	int _arrowBonusesDirs;   //направления (если один бонус)
	int _distFromRecieves;  //расстояние до приемника (для более правильной последовательности перебора)

	bool ContainsAddress(Game::FieldAddress fa); //содержит ли ход этот адрес
	bool ContainsEnergyCell(FUUUField &f1); //содержит ли ячейку с энергией (используя поле f1)
};

typedef std::vector<FUUUMove> MoveList;

//класс хранящий информацию о поле как элементе перебора вариантов
class FUUUFieldInfo
{
public:
	FUUUField* _pField; //ссылка на поле
	FUUUOrders *_pOrders; //ссылка на заказы
	MoveList _moves; //перечень ходов которые сюда приводят
	int _depth;        //число ходов через которое это поле можно получить
};

class FUUUTester
{
public:
	FUUUTester(void);
	~FUUUTester(void);

	//количество разных возможных первых ходов
	int _numberOfFirstMoves;
	//ходов до победы
	int _movesToWin;
private:
	//отладочный счетчик
	int _ctr;
	//размеры
	int _fieldLeft;
	int _fieldRight;
	int _fieldTop;
	int _fieldBottom;
	int _fieldWidth;
	int _fieldHeight;
	//стартовое время и лимит времени
	double _startTime;
	double _timeLimitInSec;
	bool _timeExpired; //кончилось время на расчет
	//есть получатели не видимые на экране
	bool _hasNotVisibleRecievers;
	//максимальная глубина поиска
	int _maxDepth;
	//глубина поиска текущей попытки
	int _currentDepth;
	//поле куда загружается исходное состояние
	FUUUField _field;
	//список приемников (не меняется за ход)
	MoveCoordsList _recievers;
	//исходные заказы
	std::list<FUUUOrder> _orders;
	//очередь еще не просчитанных вариантов
	std::queue<FUUUFieldInfo> _fieldsForCheck;
	//выигрывающие ходы
	MoveList _winningMoveList;
public:
	//сколько осталось ходов до победы (ограничение глубины расчета, ограничение времени
	int CountMovesToFinish(int maxMoves, float timeLimitInSec);
	//сколько осталось ходов до победы. просто возвращает ранее посчитанное значение
	int GetMovesToFinish();
	//сколько есть возможных первых ходов
	int CountPossibleMoves();
	//рисует выигрывающие ходы
	void Draw();
	//очистка данных
	void Clear();

private:
	//загрузка данных из игры во внутренние структуры
	void LoadData();
	//подсчет расстояния от клетки до получателей
	int CountDistanceFromRecievers(Game::FieldAddress fa);
	//Подсчет бонусов в ходе
	void CountBonuses(FUUUField &f1, FUUUMove& move);
	//поиск хода из текущей ячейки
	void FindMoveFromHere(FUUUField &f1, Game::FieldAddress fa, FUUUMove &move);
	//растим энергию куда только можем
	void GrowEnergy(FUUUField &f1);
	//имитация хода
	void ImitateMove(FUUUField &f1, FUUUOrders &o1, FUUUMove &m);
	//пробуем один ход
	void CountSimpleMove(FUUUFieldInfo fieldInfo);
	//сброс флажка affected
	void SetNotAffected(FUUUField &f1);
	//сброс флажка visited
	void SetNotVisited(FUUUField &f1);
	//сброс флажка affected
	void SetNotDestroyed(FUUUField &f1);
	//все приемники энергии заполнены
	bool AllEnergyRecieversFilled(FUUUField &f1);
	//запускает рост энергии и проверяет получателей
	void GrowEnergyAndCheck(FUUUField &f1, FUUUOrders &o1, MoveList &m1);
	//имитирует запуск бонусов с данной клетки
	void ImitateBonus(FUUUField &f1, FUUUOrders &o1, FUUUMove &move, Game::FieldAddress start);
	//имитирует запуск стрел с данной клетки
	void ThrowArrowsFromPoint(FUUUField &f1, FUUUOrders &o1, Game::FieldAddress start, int directions, int radius, std::queue<FUUUBonus> &Bonuses);
	//производит падение фишек
	void FallChips(FUUUField &f1);
	//производит вертикальное падение фишек
	void VerticalFallChips(FUUUField &f1);
	//производит диагональное падение фишек, возвращает true если было
	bool DiagonalFallChips(FUUUField &f1);
	//список возможных ходов
	void BuildPossibleMoves(FUUUField &f1, FUUUMove &fullMove, MoveList &moves);
	//проверить не кончилось ли время
	void CheckTime();
};

#endif