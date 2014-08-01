#include "StdAfx.h"
#include "FUUUTester.h"

#include "Game.h"
#include "GameField.h"
#include "EnergyReceivers.h"
#include "GameBonus.h"
#include "GameBonuses.h"
#include "Energy.h"
#include "GameInfo.h"

//получить текущее время в секундах
double getGlobalTime()
{
	double global_time=0;
#if defined ( ENGINE_TARGET_WIN32 )
	FILETIME ft;
	GetSystemTimeAsFileTime(&ft);
	global_time = (static_cast<__int64>(ft.dwHighDateTime) << 32 | ft.dwLowDateTime) / 10000000.0;
#elif defined ( ENGINE_TARGET_IPHONE ) || defined(ENGINE_TARGET_LINUX)
	struct timeval time;
	gettimeofday(&time, NULL);
	global_time = time.tv_sec + (double)time.tv_usec / 1000000.0;
#else
	Assert2(false);
#endif
	return global_time;
}


//////////////////////////////////////// клетка //////////////////////////////////
//пытаемся проверить что заказ подходит
void TryToMakeOrders(FUUUOrders &o1, FUUUCell &cell)
{
	for (std::list<FUUUOrder>::iterator it=o1.begin(); it != o1.end(); ++it)
	{
		FUUUOrder& currentOrder = *it;
		if (cell._chipType == fuuu_normal_chip && cell._color == currentOrder._orderObjective) //обычная фишка
		{
			currentOrder._madeCount++;
		} else if (cell._chipType == fuuu_licorice_chip && currentOrder._orderObjective == Game::Order::LICORICE) //лакрица
		{
			currentOrder._madeCount++;
		} else if (cell._chipType == fuuu_empty_chip && cell._cellType == fuuu_wall && cell._wallHeight == 1 && currentOrder._orderObjective == Game::Order::WALL) //стена(земля)
		{
			currentOrder._madeCount++;
		} else if (cell._cellType == fuuu_wood && cell._woodHeight == 1 && currentOrder._orderObjective == Game::Order::GROUND) //земля(лес)
		{
			currentOrder._madeCount++;
		}
	}
}

//может ли быть участником хода
bool FUUUCell::CanMoveHere()
{
	return (!_visited) && (_chipType == fuuu_normal_chip) && (_cellType != fuuu_block);
}

bool FUUUCell::CanEnergyMoveHere()
{
	return (!_visited) && (_cellType == fuuu_empty || _isReciever ) && (!_isEnergy);
}
//клетка блокирует падение
bool FUUUCell::BlockFalling()
{
	return !(_cellType == fuuu_empty || _cellType == fuuu_wall);
}
//можно ли сюда упасть
bool FUUUCell::CanFallHere()
{
	return (_cellType == fuuu_empty || _cellType == fuuu_wall) && (_chipType == fuuu_empty_chip);
}
//может ли эта фигня упасть
bool FUUUCell::ThisChipCanFall()
{
	return (_chipType == fuuu_normal_chip || _chipType == fuuu_licorice_chip) && (!_isIce);
}

bool FUUUCell::ProcessArrow(std::queue<FUUUBonus> &Bonuses, FUUUOrders& o1, int direction) //возвращает true если стрела тут заканчивается
{
	if (_affectedByBonus) return false; //если уже были тут, выходим

	_affectedByBonus = true; //ставим флажок что были. таким образом за ход только 1 раз попадаем бонусом в клетку

	if (_bonus.HasArrowBonus()) //если тут есть бонус, добавляем его в очередь на обработку
	{
		FUUUBonus newBonus;
		newBonus = _bonus;
		//проверяем а вдруг надо повернуть стрелку
		if ((newBonus._arrowDirs & direction) !=0) //направления совпадают
		{
			BYTE new_dir = newBonus._arrowDirs;
			new_dir = 	(new_dir << 2) | (new_dir >> (6));
			newBonus._arrowDirs = new_dir;
		}
		Bonuses.push(newBonus);
		_bonus.Clear();
	}
	
	//бонус уже запустили (вдруг клетка разрушилась без запуска бонуса)
	//теперь если не разрушена то рушим
	if (_destroyed)
	{
		return false;
	}

	_destroyed = true;

	if (_chipType==fuuu_licorice_chip) //сперва обрабатываем лакриц. они принимают весь урон на себя
	{
		TryToMakeOrders(o1,*this);
		_chipType=fuuu_empty_chip;
		return true;
	}
	if (_isIce) //убираем лед если есть. дальше идет но с этой клеткой все
	{
		_isIce = false;
		return false;
	}
	if (_chipType == fuuu_normal_chip) //по любому убирается обычная фишка
	{
		TryToMakeOrders(o1,*this);
		_chipType = fuuu_empty_chip;
	}
	if (_cellType == fuuu_wall) {//рушатся стены
		BreakWall(o1);
	} else if (_cellType == fuuu_wood) {//рушатся леса
		BreakWood(o1);
	} else if (_cellType == fuuu_stone) { //рушим стену
		BreakStone();
	}
	return false;
}

void FUUUCell::BreakWall(FUUUOrders& o1)
{
	TryToMakeOrders(o1, *this);

	_wallHeight--;
	if (_wallHeight == 0) //значит стена исчезла
	{
		_cellType = fuuu_empty;
	}
}

void FUUUCell::BreakWood(FUUUOrders& o1)
{
	TryToMakeOrders(o1, *this);
	_woodHeight--;
	if (_woodHeight == 0) //значит лес исчез, вместо него стена. лес может стоять только на стене
	{
		_cellType = fuuu_wall;
	}
}

void FUUUCell::BreakStone() //считается что стена всегда стоит на земле высоты 1
{
	if (_wallHeight>0) {
		_cellType = fuuu_wall;
		_wallHeight = 1;
	}
}

//обработка "прикосновения" (матч рядом)
void FUUUCell::Touch(FUUUOrders& o1)
{
	if (_destroyed)
	{
		return;
	}

	if (_chipType == fuuu_licorice_chip) //прикосновения удаляют лакриц
	{
		TryToMakeOrders(o1, *this);
		_chipType = fuuu_empty_chip;
		_destroyed = true;
	}
	if (_cellType == fuuu_wood) //рушат лес
	{
		BreakWood(o1);
		_destroyed = true;
	}
}


/////////////////////// ход //////////////////////////////////////////////////

//содержит ли ход эту клетку
bool FUUUMove::ContainsAddress(Game::FieldAddress fa)
{
	for (MoveCoordsList::iterator it=_coords.begin(); it != _coords.end(); ++it)
	{
		Game::FieldAddress tryFa=*it;
		if (fa == tryFa)
		{
			return true;
		}
	}
	return false;
}

//содержит ли ячейку с энергией (используя поле f1)
bool FUUUMove::ContainsEnergyCell(FUUUField &f1) 
{
	for (MoveCoordsList::iterator it=_coords.begin(); it != _coords.end(); ++it)
	{
		Game::FieldAddress tryFa=*it;
		FUUUCell &tryCell = f1[tryFa];
		if (tryCell._isEnergy)
		{
			return true;
		}
	}
	return false;
}


//////////////////// тестер ////////////////////////////////////////////////
FUUUTester::FUUUTester(void)
{
}


FUUUTester::~FUUUTester(void)
{
}

//заполнение списка соседних клеток включая диагонали
void FillDirections8(Game::FieldAddress *fa, std::list<Game::FieldAddress> &directions)
{
	directions.clear();
	directions.push_back(*fa + fa->DOWN);
	directions.push_back(*fa + fa->UP);
	directions.push_back(*fa + fa->LEFT);
	directions.push_back(*fa + fa->RIGHT);
	directions.push_back(*fa + fa->LEFT_DOWN);
	directions.push_back(*fa + fa->LEFT_UP);
	directions.push_back(*fa + fa->RIGHT_DOWN);
	directions.push_back(*fa + fa->RIGHT_UP);
}
//не включая диагонали
void FillDirections4(Game::FieldAddress *fa, std::list<Game::FieldAddress> &directions)
{
	directions.clear();
	directions.push_back(*fa + fa->DOWN);
	directions.push_back(*fa + fa->UP);
	directions.push_back(*fa + fa->LEFT);
	directions.push_back(*fa + fa->RIGHT);
}

//подсчет расстояния от клетки до получателей
int FUUUTester::CountDistanceFromRecievers(Game::FieldAddress fa)
{
	int d = 1000;
	for (MoveCoordsList::iterator it = _recievers.begin(); it != _recievers.end(); ++it)
	{
		Game::FieldAddress fa1 = *it;
		int new_d = math::max( abs(fa.GetRow() - fa1.GetRow()), abs(fa.GetCol() - fa1.GetCol()) );
		if (new_d < d)
		{
			d = new_d;
		}
	}
	return d;
}

//находит возможный ход с этой клетки (строит связную компоненту)
void FUUUTester::FindMoveFromHere(FUUUField &f1, Game::FieldAddress fa, FUUUMove &move)
{
	move._coords.clear();
	move._bonusCoords.clear();
	move._distFromRecieves = CountDistanceFromRecievers(fa); //инициализируем расстояние до приемников
	FUUUCell &cell = f1[fa];
	//если можно начать ход
	if (cell.CanMoveHere())
	{
		std::queue <Game::FieldAddress> queue; //очередь не отработанных клеток
		
		queue.push(fa); //добавляем
		cell._visited = true;
		move._coords.insert(fa);
		if (cell._bonus._arrowBonusCount > 0)   //добавляем координаты клеток с бонусом
		{
			move._bonusCoords.insert(fa);
		}

		int colorMoving = cell._color; //цвет фишек текущего хода
		//итеративный алгоритм поиска в ширину
		while (!queue.empty())
		{
			Game::FieldAddress currentCell=queue.front(); //клетка из которой ходим
			queue.pop(); //убираем ее
			std::list<Game::FieldAddress> directions; //соседи
			FillDirections8(&currentCell, directions);  //заполняем список
			
			//перебираем соседей
			for (std::list<Game::FieldAddress>::iterator it=directions.begin(); it != directions.end(); ++it)
			{
				//берем соседа
				Game::FieldAddress tryFa = *it;
				FUUUCell &tryCell = f1[tryFa];
				
				//если можно туда ходить и она того же цвета
				if (tryCell.CanMoveHere() && tryCell._color==colorMoving)
				{
					queue.push(tryFa); //добавляем
					tryCell._visited = true;

					move._coords.insert(tryFa); //добавляем к ходу

					int newDistance = CountDistanceFromRecievers(tryFa); //обновляем расстояние от хода до получателей
					if (newDistance < move._distFromRecieves)
					{
						move._distFromRecieves = newDistance;
					}

					if (tryCell._bonus._arrowBonusCount > 0)   //добавляем координаты клеток с бонусом
					{
						move._bonusCoords.insert(tryFa);
					}
				}
			}
		}
	}
}

//считаем бонусы на ходе
void FUUUTester::CountBonuses(FUUUField &f1, FUUUMove& move)
{
	move._arrowBonusesCount = 0;
	move._arrowBonusesRadius = 0;

	for (MoveCoordsList::iterator it=move._coords.begin(); it != move._coords.end(); ++it)
	{
		Game::FieldAddress fa=*it;
		FUUUCell &cell = f1[fa];

		if (cell._bonus.HasArrowBonus())
		{
			move._arrowBonusesCount++;
			move._arrowBonusesRadius = math::max(move._arrowBonusesRadius, cell._bonus._arrowRadius); //важно только если бонус 1
			move._arrowBonusesDirs = cell._bonus._arrowDirs; //важно только если бонус 1
		}
	}
}

void FUUUTester::SetNotVisited(FUUUField &f1)
{
	for (int col=_fieldLeft; col<=_fieldRight; col++)
		for (int row=_fieldBottom; row<=_fieldTop; row++)
		{
			Game::FieldAddress fa(col, row);
			f1[fa]._visited = false;
		}

}

void FUUUTester::SetNotAffected(FUUUField &f1)
{
	for (int col=_fieldLeft; col<=_fieldRight; col++)
		for (int row=_fieldBottom; row<=_fieldTop; row++)
		{
			Game::FieldAddress fa(col, row);
			f1[fa]._affectedByBonus = false;
		}

}

void FUUUTester::SetNotDestroyed(FUUUField &f1)
{
	for (int col=_fieldLeft; col<=_fieldRight; col++)
		for (int row=_fieldBottom; row<=_fieldTop; row++)
		{
			Game::FieldAddress fa(col, row);
			f1[fa]._destroyed = false;
		}

}

//распространяет энергию куда может
void FUUUTester::GrowEnergy(FUUUField &f1)
{
	//сбрасываем признак visited
	SetNotVisited(f1);
	
	//перебираем клетки
	for (int col=_fieldLeft; col<=_fieldRight; col++)
		for (int row=_fieldBottom; row<=_fieldTop; row++)
		{
			Game::FieldAddress fa(col, row);
			FUUUCell &cell = f1[fa];
			// если есть энегрия и тут еще не были
			if (cell._isEnergy && !cell._visited)
			{
				//распространяем энергию
				std::queue <Game::FieldAddress> queue; //очередь не отработанных клеток
				queue.push(fa); //добавляем
				cell._visited = true;
		
				//итеративный алгоритм поиска в ширину
				while (!queue.empty())
				{
					Game::FieldAddress currentCell=queue.front(); //клетка из которой ходим
					queue.pop(); //убираем ее
					std::list<Game::FieldAddress> directions; //соседи
					FillDirections8(&currentCell, directions);  //заполняем список
			
					//перебираем соседей
					for (std::list<Game::FieldAddress>::iterator it=directions.begin(); it != directions.end(); ++it)
					{
						//берем соседа
						Game::FieldAddress tryFa = *it;
						FUUUCell &tryCell = f1[tryFa];
				
						//если может притечь энегрия
						if (tryCell.CanEnergyMoveHere())
						{
							queue.push(tryFa); //добавляем
							tryCell._visited = true;
							tryCell._isEnergy = true;
						}
					}
				}
			}
		}
}

//роняет фишку или что там из cellFrom в cellTo
void FallCells(FUUUCell &cellFrom, FUUUCell &cellTo, Game::FieldAddress faTo)
{
	cellTo._chipType = cellFrom._chipType;
	cellTo._color = cellFrom._color;
	cellTo._bonus._arrowBonusCount = cellFrom._bonus._arrowBonusCount;
	cellTo._bonus._arrowRadius = cellFrom._bonus._arrowRadius;
	cellTo._bonus._arrowDirs = cellFrom._bonus._arrowDirs;
	cellTo._bonus._fromCell = faTo;

	cellFrom._chipType = fuuu_empty_chip;
	cellFrom._color = -1;
	cellFrom._bonus._arrowBonusCount = 0;
	cellFrom._bonus._arrowRadius = 0;
	cellFrom._bonus._arrowDirs = 0;
}

//производит вертикальное падение фишек
void FUUUTester::VerticalFallChips(FUUUField &f1)
{
	//перебираем ячейки
	for (int row=_fieldBottom; row<=_fieldTop; row++)
		for (int col=_fieldLeft; col<=_fieldRight; col++)
		{
			Game::FieldAddress fa(col, row);
			FUUUCell &cell = f1[fa];
			
			//находим пустую
			if (cell.CanFallHere())
			{
				//ищем для нее фишку сверху
				Game::FieldAddress faFall = fa.Up();
				while ((faFall.GetRow() <= _fieldTop) && (! f1[faFall].ThisChipCanFall()) && (!f1[faFall].BlockFalling())  )
				{
					faFall = faFall.Up();
				}

				//если нашли, роняем
				if (f1[faFall].ThisChipCanFall())
				{
					FUUUCell &cellFrom = f1[faFall];
					FallCells(cellFrom,cell, fa);
				}
			}
		}
}
//производит диагональное падение фишек
bool FUUUTester::DiagonalFallChips(FUUUField &f1)
{
	bool result = false;
	//перебираем ячейки
	for (int row=_fieldBottom; row<=_fieldTop; row++)
		for (int col=_fieldLeft; col<=_fieldRight; col++)
		{
			Game::FieldAddress fa(col, row);
			FUUUCell &cell = f1[fa];
			
			//находим пустую
			if (cell.CanFallHere())
			{
				//ищем вершину "колодца"
				Game::FieldAddress faFall = fa.Up();
				while ((faFall.GetRow() <= _fieldTop) && f1[faFall].CanFallHere())
				{
					faFall = faFall.Up();
				}
				//если может свалиться которая над колодцем (такое возможно), роняем и ее
				if (f1[faFall].ThisChipCanFall())
				{
					FUUUCell &cellFrom = f1[faFall];
					FallCells(cellFrom,cell, fa);
				}
				else //иначе ищем диагональные фишки
				{
					if (f1[faFall]._cellType == fuuu_block 
						&& f1[faFall.Left()]._cellType == fuuu_block 
						&& f1[faFall.Right()]._cellType == fuuu_block)
//					if (faFall.GetRow() > _fieldTop)
					{
						break; //дошли до верха поля - ронять по диагонали не будем потому что и так свалилось бы
					}
					else 
					{
						//ищем фишку которая свалится
						bool haveFound = false;
						while (faFall.GetRow()>fa.GetRow() && !haveFound) //пока не нашли и не опустились ниже ячейки куда падаем
						{
							//ищем соседей
							Game::FieldAddress faLeft = faFall.Left();
							Game::FieldAddress faRight = faFall.Right();
							//переставляем случайным образом
							if(math::random(0,1) > 0)
								std::swap(faLeft, faRight);
							//берем ячейки
							FUUUCell &cellLeft = f1[faLeft];
							FUUUCell &cellRight = f1[faRight];
							//роняем которую можем
							if (cellLeft.ThisChipCanFall())
							{
								FallCells(cellLeft, cell, fa);
								haveFound = true;
								return true;
							} else if (cellRight.ThisChipCanFall()) {
								FallCells(cellRight, cell, fa);
								haveFound = true;
								return true;
							}
							faFall=faFall.Down();
						}
					}
				}
			}

		}
	return result;
}


//падение фишек
void FUUUTester::FallChips(FUUUField &f1)
{
	//сначала вертикальное
	VerticalFallChips(f1);
	//потом диагональное и вертикальное по очереди пока процесс не кончится
	while (DiagonalFallChips(f1))
	{
		VerticalFallChips(f1);
	}
}

//имитирует ход
void FUUUTester::ImitateMove(FUUUField &f1, FUUUOrders &o1, FUUUMove &move)
{
	//сбрасываем флажок разрушений
	SetNotDestroyed(f1);
	//сбрасываем заказы
	for (std::list<FUUUOrder>::iterator it=o1.begin(); it != o1.end(); ++it)
	{
		FUUUOrder& currentOrder = *it;
		if (currentOrder._clearAfterMove)
		{
			currentOrder._madeCount = 0;
		}
	}

	//грохаем все фишки
	for (MoveCoordsList::iterator it=move._coords.begin(); it != move._coords.end(); ++it)
	{
		Game::FieldAddress fa=*it;
		FUUUCell &cell = f1[fa];

		//"прикосновения" к фишкам рядом
		std::list<Game::FieldAddress> directions; //соседи
		FillDirections4(&fa, directions);  //заполняем список
		for (std::list<Game::FieldAddress>::iterator it=directions.begin(); it != directions.end(); ++it)
		{
			//берем соседа
			Game::FieldAddress tryFa = *it;
			FUUUCell &cell = f1[tryFa];
			if (!move.ContainsAddress(tryFa)) //если он не среди участников хода
			{
				cell.Touch(o1); //запускаем прикосновение
			}
		}

		if (cell._isIce) //если лед то просто рушим, больше ничего не происходит
		{
			cell._isIce = false;
			break;
		}

		//отрабатываем заказы
		TryToMakeOrders(o1,cell);
		//убираем фишку
		cell._chipType=fuuu_empty_chip;
		//убираем бонусы
		cell._bonus.Clear();
		//ставим что разрушена чтобы 2 раз не попасть
		cell._destroyed = true;

		//уменьшаем индекс стены
		if (cell._cellType==fuuu_wall)
		{
			if (cell._isEnergyWall) { //если это стена которая разрушается только энергией
				if (move.ContainsEnergyCell(f1))  { //то сперва проверяем а есть ли она
					cell.BreakWall(o1);
				}
			} else {
				cell.BreakWall(o1);
			}
		}
	}
}

//запуск стрел с точки
void FUUUTester::ThrowArrowsFromPoint(FUUUField &f1, FUUUOrders &o1, Game::FieldAddress start, int directions, int radius, std::queue<FUUUBonus> &Bonuses)
{
	const int dx[] = {1,  1,  0, -1, -1, -1,  0,  1};
	const int dy[] = {0,  1,  1,  1,  0, -1, -1, -1};
	for(size_t i = 0; i < 8; i++)
	{
		if((directions & (1 << i)) > 0)
		{
			// просчитываем стрелу в одном из направлений
			for(int r = 0; r <= radius; r++ )
			{
				Game::FieldAddress fa = start.Shift(dx[i]*r, dy[i]*r);
				FUUUCell &cell = f1[fa];
				bool MustStop = cell.ProcessArrow(Bonuses, o1, 1 << i); //отрабатываем стрелку на ячейке

				if (MustStop) //если ячейка останавливает стрелку, то выходим
					break;
			}
		}
	}
}

//имитирует запуск бонуса
void FUUUTester::ImitateBonus(FUUUField &f1, FUUUOrders &o1, FUUUMove &move, Game::FieldAddress start)
{
	//помечаем что бонусы еще не применялись ни к чему
	SetNotAffected(f1);
	//очередь бонусов, которые запускают друг друга (если найдутся)
	std::queue<FUUUBonus> Bonuses;
	//добавляем исходный бонус
	FUUUBonus Bonus;
	Bonus._arrowBonusCount = move._arrowBonusesCount;
	Bonus._arrowDirs = move._arrowBonusesDirs;
	Bonus._arrowRadius = move._arrowBonusesRadius;
	Bonus._fromCell = start;
	Bonuses.push(Bonus);
	
	//основной цикл
	while (!Bonuses.empty()) //пока бонусы не кончились
	{
		FUUUBonus currentBonus = Bonuses.front();
		Bonuses.pop();
		Game::FieldAddress startCell = currentBonus._fromCell;

		//стрелки
		if (currentBonus._arrowBonusCount > 0) {
			int directions = currentBonus._arrowDirs;
			//комбинация стрелок
			if (currentBonus._arrowBonusCount == 2 || currentBonus._arrowBonusCount == 3) 
			{
				directions = 85; //если 2 и 3 то летят во все стороны = 1+4+16+64
			}

			//запускаем стрелы
			if (currentBonus._arrowBonusCount <= 2) //если <=2 то из одной точки
			{
				ThrowArrowsFromPoint(f1, o1, startCell, directions, currentBonus._arrowRadius, Bonuses);
			} else if (currentBonus._arrowBonusCount == 3) { //если 3 стрелы то из 5 точек, "крестиком"
				ThrowArrowsFromPoint(f1, o1, startCell                   , directions, currentBonus._arrowRadius, Bonuses);
				ThrowArrowsFromPoint(f1, o1, startCell + startCell.UP        , directions, currentBonus._arrowRadius-1, Bonuses);
				ThrowArrowsFromPoint(f1, o1, startCell + startCell.DOWN      , directions, currentBonus._arrowRadius-1, Bonuses);
				ThrowArrowsFromPoint(f1, o1, startCell + startCell.LEFT      , directions, currentBonus._arrowRadius-1, Bonuses);
				ThrowArrowsFromPoint(f1, o1, startCell + startCell.RIGHT     , directions, currentBonus._arrowRadius-1, Bonuses);
			} else { //4 или более точек - весь экран
				for (int col=_fieldLeft; col<=_fieldRight; col++)
					for (int row=_fieldBottom; row<=_fieldTop; row++)
					{
						Game::FieldAddress fa(col, row);
						FUUUCell &cell = f1[fa];
						cell.ProcessArrow(Bonuses, o1, 17); //имитируем горизонтальные стрелы
					}
			}
		}
	}

}

//проверяет выполнение заказов
void CheckOrdersDone(FUUUField &f1, FUUUOrders &o1)
{
	//перебираем заказы
	for (std::list<FUUUOrder>::iterator it=o1.begin(); it != o1.end(); )
	{
		FUUUOrder &order = *it;
		if (order._madeCount >= order._targetCount) //если выполнен
		{
			FUUUCell &cell = f1[order._address]; //
			cell._cellType = fuuu_empty;
			it = o1.erase(it);
		}
		else 
		{
			++it;
		}
	}

}

//проверяет что все приемники энергии довольны
bool FUUUTester::AllEnergyRecieversFilled(FUUUField &f1)
{
	for (int col=_fieldLeft; col<=_fieldRight; col++)
		for (int row=_fieldBottom; row<=_fieldTop; row++)
		{
			Game::FieldAddress fa(col, row);
			FUUUCell &cell = f1[fa];
			if (cell._isReciever && !cell._isEnergy) //есть хоть один недовольный
				return false;
		}
	//значит все довольны
	return true;
}

//запускает рост энергии и проверяет получателей
void FUUUTester::GrowEnergyAndCheck(FUUUField &f1, FUUUOrders &o1, MoveList &m1)
{
	//распространяем энергию
	GrowEnergy(f1);
	//проверяем
	if (AllEnergyRecieversFilled(f1)) //если нашли то запоминаем сколько ходов надо
	{
		_movesToWin = _currentDepth;

		if (gameInfo.IsDevMode()) {
			// рисуем клетки только в дебаг-режиме
			_winningMoveList = m1;
		}
	} else { // добавляем поле в очередь для поиска если можем
		if (_currentDepth < _maxDepth )
		{
			//роняем фишки
			FallChips(f1);
			//добавляем
			FUUUField* currentField=new FUUUField ();
			*currentField=f1; //копируем туда текущее поле

			FUUUOrders* currentOrders = new std::list<FUUUOrder>;
			*currentOrders = o1;

			FUUUFieldInfo currentInfo;
			currentInfo._pField = currentField;
			currentInfo._pOrders = currentOrders;
			currentInfo._depth = _currentDepth ; //глубина поиска
			currentInfo._moves = m1;

			_fieldsForCheck.push(currentInfo);
		}
	}
}

//набор вершин в ходе
typedef std::set<BYTE> MoveSearchCoordsList;

struct MoveForSearch
{
	MoveSearchCoordsList coords;
	int last; //номер последней вершина хода, из которой пойдем дальше
	bool haveBonus; //есть ли в ходе бонус. если есть то имеет значение где его заканчивать
};

bool MoveForSearchSortFunction(const MoveForSearch &m1, const MoveForSearch &m2) //сортировка ходов для перебора (shortlex)
{
	if (m1.coords.size() == m2.coords.size())	{ //если фишек поровну сравниваем поштучно
		if (m1.coords == m2.coords)	{             //если фишки те же
			return m1.last < m2.last;            //сравниваем последний ход
		} else {
			return m1.coords < m2.coords;
		}
	} else	{
		return m1.coords.size() < m2.coords.size(); //иначе меньше та где фишек меньше
	}
}

//строит все возможные ходы из "полного хода" (компоненты связности) fullMove в moves
void FUUUTester::BuildPossibleMoves(FUUUField &f1, FUUUMove &fullMove, MoveList &moves)
{
//	Log::Info("Start Build Moves "+utils::lexical_cast(getGlobalTime()));

	const int MAX_SIZE = 50;
	size_t size = fullMove._coords.size();
	if (size > MAX_SIZE) //если точек вообще много даже и пытаться не будем
		return;

	//делаем vector для быстрого доступа по индексу
	std::vector<Game::FieldAddress> coordsV(fullMove._coords.begin(), fullMove._coords.end());
	
	//табличка чтобы легко было узнать где бонус
	std::vector<bool> haveBonus(size, false);
	for (size_t i=0; i<size; i++ )
	{
		if (fullMove._bonusCoords.find(coordsV[i]) != fullMove._bonusCoords.end())
		{
			haveBonus[i] = true;
		}
	}

	int edgeCount = 0; //число ребер
	//строим матрицу смежности
	bool joint[MAX_SIZE][MAX_SIZE];
	for (size_t i=0; i<size; i++ )
		for (size_t j=0; j<size; j++ )
			joint[i][j] = false;

	for (size_t i=0; i+1<size; ++i)
	{
		Game::FieldAddress fa1 = coordsV[i];

		for (size_t j=i+1; j<size; j++ )
		{
			Game::FieldAddress fa2 = coordsV[j];
			if (abs(fa1.GetCol() - fa2.GetCol()) < 2 && abs(fa1.GetRow() - fa2.GetRow()) < 2)
			{
				joint[i][j] = true;
				joint[j][i] = true;
				++edgeCount;
			}
		}
	}

	size_t MAX_LEFT_MOVES = 10000; //ограничение количества ходов в очереди чтобы совсем не тормозило
	size_t TRUNCATE_SIZE = 400;   //по скоко грохать в начале очереди если придется

	size_t MAX_GOOD_MOVES = 10000; //ограничение числа ходов которые считаем
	size_t TRUNCATE_GOOD_MOVES = 100; //ограничение числа ходов которые считаем

	if (edgeCount >= 20 && edgeCount < 26)	{
		MAX_LEFT_MOVES = 400; 
		TRUNCATE_SIZE = 200;   
		MAX_GOOD_MOVES = 400; 
		TRUNCATE_GOOD_MOVES = 200; 
	} else if (edgeCount >= 26 && edgeCount < 32)	{
		MAX_LEFT_MOVES = 400; 
		TRUNCATE_SIZE = 200;   

		MAX_GOOD_MOVES = 300; 
		TRUNCATE_GOOD_MOVES = 150; 
	} else if (edgeCount >= 32)	{
		MAX_LEFT_MOVES = 400; 
		TRUNCATE_SIZE = 200;   

		MAX_GOOD_MOVES = 200; 
		TRUNCATE_GOOD_MOVES = 100; 
	}

	//int chipCount = fullMove._coords.size();
	//if ((chipCount > 8) && (chipCount <= 10)) 
	//{
	//	MAX_LEFT_MOVES = 400; 
	//	TRUNCATE_SIZE = 200;   

	//	MAX_GOOD_MOVES = 400; 
	//	TRUNCATE_GOOD_MOVES = 200; 
	//} else if (chipCount > 10  && chipCount <= 13)  {
	//	MAX_LEFT_MOVES = 400; 
	//	TRUNCATE_SIZE = 200;   

	//	MAX_GOOD_MOVES = 300; 
	//	TRUNCATE_GOOD_MOVES = 150; 
	//} else if (chipCount > 13)  {
	//	MAX_LEFT_MOVES = 400; 
	//	TRUNCATE_SIZE = 200;   

	//	MAX_GOOD_MOVES = 200; 
	//	TRUNCATE_GOOD_MOVES = 100; 
	//}

	//строим все подграфы
	size_t left_moves_count = 0; //количество ходов для перебора, используется для ограничения перебора вариантов
	std::list<MoveForSearch> leftMovesToDo; //список ходов для перебора
	std::vector<MoveForSearch> goodMoves; //список найденных ходов

	//заполняем стартовыми вершинами
	MoveCoordsList::iterator it1 = fullMove._coords.begin();
	for (size_t i=0; i < size; i++, ++it1) /////!! подумать может 1 или 2 последних можно не брать, хотя.....
	{
		MoveForSearch start;
		start.coords.insert(i);
		start.last = i;
		start.haveBonus = haveBonus[i];
		
		leftMovesToDo.push_back(start);
		++left_moves_count;
	}
	
	//отрабатываем очередь
	while (!leftMovesToDo.empty())
	{
		CheckTime(); //провеяем не хватит ли
		if (_timeExpired)
		{
			break;
		}

		//если вариантов лишка, отсекаем начало очереди
		//очередь leftMovesToDo всегда выглядит примерно так (по числу фишек в ходах) 33333344444444444444444
		// если очередь офигенно большая то начало очереди где 3 фишки убираем. вариантов и так лишка, авось прокатит
		if (left_moves_count > MAX_LEFT_MOVES)
		{
			//удаляем кусочек
			std::list<MoveForSearch>::iterator it_end = leftMovesToDo.begin();
			for (size_t i=0; i<TRUNCATE_SIZE; ++i)
				++it_end;
			leftMovesToDo.erase(leftMovesToDo.begin(), it_end);
			left_moves_count -= TRUNCATE_SIZE;
			//int moveSize = leftMovesToDo.front().coords.size(); //количество фишек в удаляемых ходах
			//int deletingCount = 0;
		}

		//чистим очередь отобранных ходов. нет смысла отрабатывать все что можно ибо перебор все равно не закончится
		if (goodMoves.size() > MAX_GOOD_MOVES)
		{
//			std::random_shuffle(goodMoves.begin(), goodMoves.end());
			goodMoves.erase(goodMoves.begin(), goodMoves.begin() + TRUNCATE_GOOD_MOVES);
//			std::sort(goodMoves.begin(), goodMoves.begin(), MoveForSearchSortFunction);
		}


		MoveForSearch startMove = leftMovesToDo.front(); //достаем 
		leftMovesToDo.pop_front();
		--left_moves_count;

		int i = startMove.last; //последняя вершина
		for (size_t j=0; j<size; j++) //берем все вершины
		{
			if (joint[i][j] && startMove.coords.find(j)==startMove.coords.end()) //если соединена и еще не ходили
			{
				MoveForSearch nextMove = startMove; //генерим следующий ход
				nextMove.coords.insert(j);
				nextMove.last = j;
				nextMove.haveBonus = nextMove.haveBonus || haveBonus[j];

				bool haveEqualMove = false; 
				
				//если есть ход ведущий к тем же результатам но с разной финальной точкой то перебирать его продолжения надо, но сам ход не сохраняем

				//ищем двоичным поиском правильное место
				std::vector<MoveForSearch>::iterator findMove = std::lower_bound(goodMoves.begin(), goodMoves.end(), nextMove, MoveForSearchSortFunction);

				//если там что то другое значит такого хода еще не было
				if (findMove ==  goodMoves.end() || MoveForSearchSortFunction(nextMove, *findMove))
				{
					if (nextMove.coords.size() < size ) //если еще есть что перебирать
					{
						leftMovesToDo.push_back(nextMove); //добавляем в очередь обработки
						++left_moves_count;
					}
					if (nextMove.coords.size() > 2)
					{
						goodMoves.insert(findMove, nextMove); //и в список всех ходов на правильное место, если больше 2 фишек и нету эквивалентного
//						goodMoves.push_back(nextMove); 
					}

				}

			}
		}

	}
	
	//формируем результат
	//в обратную сторону потому что у ходов которые удаляют больше клеток, больше шансов быть выигрывающими
	//при этом удаляем дублирующие ходы (одинаковые фишки и нет бонусов, т.е. где заканчивается неважно)
	
	//последний добавленный ход. сверяем с ним при добавлении потому что эквивалентные ходы идут подряд
	MoveForSearch lastSourceMove;

	moves.clear();
	for (std::vector<MoveForSearch>::reverse_iterator it = goodMoves.rbegin(); it != goodMoves.rend(); ++it)
	{
		MoveForSearch sourceMove = *it;
		FUUUMove destMove;

		bool new_move = false; //новый ли ход
		if ((lastSourceMove.coords != sourceMove.coords) || sourceMove.haveBonus)
		{
			new_move = true;
		}

		if (new_move)
		{
			destMove._coords.clear();
			for (const auto &i: sourceMove.coords)
			{
				destMove._coords.insert(coordsV[i]);
			}
			if (sourceMove.haveBonus )//если есть бонус
			{ //генерим данные
				destMove._startBonus = coordsV[sourceMove.last]; //начинается с последней клетки
				CountBonuses(f1, destMove); //считаем бонусы
			} else {
				destMove._arrowBonusesCount = 0; //иначе ставим что их нету
			}

			moves.push_back(destMove);

			lastSourceMove = sourceMove;
		}
	}
//	Log::Info("Finish Build Moves "+utils::lexical_cast(getGlobalTime()));
}

//сортировка компонент связности
bool ComponentSortFunction(const FUUUMove &m1, const FUUUMove &m2)
{
	if (m1._distFromRecieves == m2._distFromRecieves) //берем те которые ближе. из равноудаленных те которые больше
		return m1._coords.size() > m2._coords.size(); //именно > потому что они должны встать в начало списка
	else
		return m1._distFromRecieves < m2._distFromRecieves;
}

void FUUUTester::CountSimpleMove(FUUUFieldInfo fieldInfo)
{
	FUUUField &f0 = (*fieldInfo._pField); //делаем рабочую копию поля

	SetNotVisited(f0);

	MoveList connectedComponents; ////компоненты связности (для поиска ходов)
	//строим список компонент связности
	for (int col=_fieldLeft; col<=_fieldRight; col++)
		for (int row=_fieldBottom; row<=_fieldTop; row++)
		{
			Game::FieldAddress fa(col, row);
			FUUUMove component;
			FindMoveFromHere(f0, fa, component);

			if (component._coords.size() >= 3)
			{
				connectedComponents.push_back(component);
			}
		}

	//сортируем по растоянию
	std::sort(connectedComponents.begin(), connectedComponents.end(), ComponentSortFunction);

	//перебираем компонениы
	for (MoveList::iterator itComponents = connectedComponents.begin(); itComponents != connectedComponents.end(); ++itComponents)
	{
		FUUUMove &testMove = *itComponents;

		MoveList moves;

		if (_currentDepth ==1) { //подсчет возможных первых ходов
			_numberOfFirstMoves++;
		}
		
		//считаем все возможные ходы
		BuildPossibleMoves(f0, testMove, moves);

		/// на последнем ходе проверяем только максимальный ход (если нету бонусов)
		if (testMove._bonusCoords.empty() && _currentDepth == _maxDepth && !moves.empty())
		{
			FUUUMove firstMove = moves.front();	 //наверно так быстрее чем удалять
			moves.clear();
			moves.push_back(firstMove);
		}
		///
		
		//_timeExpired = true;
		if (_timeExpired) //провеяем время, при генерации ходов могло и кончиться
		{
			break;
		}
				
		//перебираем их
		for (MoveList::iterator it = moves.begin(); it != moves.end(); ++it)
		{
			if (_movesToWin!=0) //если нашли выигрыш, выходим
			{
				break;
			}
			FUUUMove &currentMove = *it;
			//делаем копию поля после хода
			FUUUField f1; 
			f1 = f0;

			FUUUOrders o1; //создаем копию заказов
			o1 = (*fieldInfo._pOrders);

			ImitateMove(f1, o1, currentMove); //имитируем ход
			if (currentMove._arrowBonusesCount > 0) //имитируем бонусы
			{
				ImitateBonus(f1, o1, currentMove, currentMove._startBonus);
			}

			CheckOrdersDone(f1, o1);  //проверяем заказы

			MoveList m1 = fieldInfo._moves; //список ходов которые приводят к данной "позиции"
			m1.push_back(currentMove); //добавляем текущий ход

			GrowEnergyAndCheck(f1, o1, m1);            //ростим энергию и проверяем

			f1.Release();
		}
	}
}

void FUUUTester::LoadData()
{
	Array2D<Game::Square*> (&gamefield) = GameSettings::gamefield;
	GameField * game = GameField::Get();

	//берем размеры
	
	_fieldLeft = Game::activeRect.x;         //это размеры активной области
	_fieldRight = Game::activeRect.x + Game::activeRect.width - 1;
	_fieldTop = Game::activeRect.y + Game::activeRect.height - 1;
	_fieldBottom = Game::activeRect.y;
	_fieldWidth = Game::activeRect.width;
	_fieldHeight = Game::activeRect.height;

	size_t RecieversOnScreen = 0;
	//заполняем массив клеток
	FUUUCell EmptyCell;
	_field.Init(_fieldRight+2, _fieldTop+2, EmptyCell, 1, 1);
	_recievers.clear();

	for (int col=_fieldLeft; col<=_fieldRight; col++)
		for (int row=_fieldBottom; row<=_fieldTop; row++)
		{
			Game::FieldAddress fa(col, row);
			Game::Square *sq = GameSettings::gamefield[fa];
			Game::ChipColor& chip=sq->GetChip ();
			FUUUCell &cell=_field[fa];

			//фишка
			int color = sq->GetChip().GetColor();
			if (color > 0) {
				cell._color = color;
				cell._chipType = fuuu_normal_chip;

				//бонус типа ARROW
				Game::ArrowBonus *arr = (Game::ArrowBonus*) sq->GetChip().GetHang().GetBonusByType(Game::HangBonus::ARROW);
				if (arr)
				{
					cell._bonus._arrowBonusCount = 1;
					cell._bonus._arrowRadius = arr->GetRadius();
					cell._bonus._arrowDirs = arr ->GetDirections();
					cell._bonus._fromCell = fa;
				}
			} else if (sq->GetChip().IsLicorice()) {//лакрица
				cell._chipType = fuuu_licorice_chip;
			}

			//клетка
			if (!sq->IsFake())
			{
				int wall=sq -> GetWall();
				int wood=sq -> GetWood();
				cell._wallHeight = wall; //когда исчезнет надстройка, будет такая вот стена

				if (sq->IsStone()) {
					cell._cellType = fuuu_stone;
				} else if (wood>0) {
					cell._cellType = fuuu_wood;
					cell._woodHeight = wood;
				} else if (wall>0) {
					cell._cellType = fuuu_wall;
				} else {
					cell._cellType = fuuu_empty;
				}
			}

			//лед
			if (sq->IsIce())
			{
				cell._isIce = true;
			}

			//земля убираемая только энергией
			if (sq->IsEnergyWall())
			{
				cell._isEnergyWall = true;
			}

			//энергия
			if (Energy::field.FullOfEnergy(fa))
			{
				cell._isEnergy = true;
			}

			//приемник
			EnergyReceiver * rec= Gadgets::receivers.GetReceiverOnSquare(fa);
			if (rec && ( !(rec->IsFinished()) ))
			{
				cell._isReciever = true;
				++RecieversOnScreen;

				_recievers.insert(fa); //добавляем приемник
			}
		}
	//если активных приемников больше чем приемников на экране, смысла считать нет
	_hasNotVisibleRecievers = ( ( Gadgets::receivers.TotalCount() - Gadgets::receivers.ActiveCount() > RecieversOnScreen));

	//заполняем заказы
	std::vector<Game::Order::WeakPtr> &all_orders = Game::GetAllOrders();

	_orders.clear();
	for(std::vector<Game::Order::WeakPtr>::iterator itr = all_orders.begin(); itr != all_orders.end(); ++itr)
	{
		Game::Order::HardPtr order = (*itr).lock();
		if( order ) {
			if(order->GetType() == Game::Order::KILL_CELL && order->OnScreen() && order->IsActive() && !order->Completed())
			{
				Game::DestroyChipsOrder *ord = (Game::DestroyChipsOrder*)order.get();

				FUUUOrder newOrder;
				newOrder._orderObjective = ord->GetChipColor();
				newOrder._address = Game::FieldAddress(ord->GetAddress());
				newOrder._targetCount = ord->GetCount();
				newOrder._madeCount = ord->GetCountKilled();
				newOrder._clearAfterMove = ord->NeedResetAfterMove();

				_orders.push_front(newOrder);
				//помечаем в ячейке что это заказ
				FUUUCell &cell=_field[newOrder._address];
				cell._cellType = fuuu_order;


			}
			order.reset();
		}
	}
}
//просто подсчет возможных ходов
int FUUUTester::CountPossibleMoves()
{
	//исходные данные
	LoadData();
	//создаем копию поля
	FUUUField f0; 
	f0 = _field;
	//считаем
	SetNotVisited(f0);
	_numberOfFirstMoves = 0;
	//перебираем возможные ходы
	for (int col=_fieldLeft; col<=_fieldRight; col++)
		for (int row=_fieldBottom; row<=_fieldTop; row++)
		{
			//находим ход
			Game::FieldAddress fa(col, row);
			FUUUMove testMove;
			FindMoveFromHere(f0, fa, testMove);

			//если там больше 3 фишек то ход возможен
			if (testMove._coords.size()>=3)
			{
				_numberOfFirstMoves++;
			}
		}
	f0.Release();
	return _numberOfFirstMoves;
}

void foo()
{
}

void FUUUTester::CheckTime()
{
	if (getGlobalTime() - _startTime >= _timeLimitInSec)
	{
		_timeExpired = true;
	}
}

int FUUUTester::CountMovesToFinish(int maxMoves, float timeLimitInSec)
{
//	AsyncWorkingQueue q;
//	q.Execute(foo, NULL);

	std::string result;

	//исходные данные
	LoadData();

	_movesToWin = 0;
	_winningMoveList.clear();
	_numberOfFirstMoves=0;

	_startTime = getGlobalTime();
	_timeLimitInSec = timeLimitInSec; 
	_timeExpired = false;

	if (!_hasNotVisibleRecievers) //если не все приемники на экране, смысла считать нет
	{
		_maxDepth = maxMoves;

		//создаем стартовый элемент очереди для поиска по дереву
		FUUUField* startField = new FUUUField ();
		*startField = _field; //копируем туда исходное поле
	
		FUUUOrders *startOrders= new std::list<FUUUOrder>;
		*startOrders = _orders; //копируем исходные заказы

		FUUUFieldInfo startInfo;
		startInfo._pField = startField;
		startInfo._pOrders = startOrders;
		startInfo._depth = 0; //глубина поиска

		//помещаем в очередь
		_fieldsForCheck.push(startInfo);
		//пока в очереди есть поля а ответ не найден
		while (!_fieldsForCheck.empty() && (_movesToWin==0))
		{
			//вынимаем элемент
			FUUUFieldInfo currentInfo = _fieldsForCheck.front();
			_fieldsForCheck.pop();
			//отрабатываем
			_currentDepth = currentInfo._depth + 1; //запускаем следующий виток перебора ходов
			CountSimpleMove(currentInfo);
			//очищаем
			currentInfo._pField->Release();
			delete currentInfo._pField;
			delete currentInfo._pOrders;
			//проверяем время, может уже хватит
			CheckTime();
			if (_timeExpired)
			{
				break;
			}
		}

		//чистим остатки очереди
		while (!_fieldsForCheck.empty())
		{
			FUUUFieldInfo currentInfo = _fieldsForCheck.front();
			_fieldsForCheck.pop();
			currentInfo._pField->Release();
			delete currentInfo._pField;
			delete currentInfo._pOrders;
		}
	}

	//очищаем исходное поле
	_field.Release();

	//возвращаем результат
	return GetMovesToFinish();
}

int FUUUTester::GetMovesToFinish()
{
	if (_movesToWin > 0)
	{
		return _movesToWin;
	} else {
		return 999999; //условное большое число
	}
}

void FUUUTester::Clear()
{
	_winningMoveList.clear();
}


//рисуем выигрывающую последовательность
void FUUUTester::Draw()
{
	if(!gameInfo.IsDevMode())
	{
		return;
	}
	if (_winningMoveList.empty()) 
		return; //если пустая - выходим

	Render::FreeType::BindFont("Score");
	Render::BeginColor(Color::BLACK);
	int n = 1; //номер хода
	for (MoveList::iterator it1 = _winningMoveList.begin(); it1 != _winningMoveList.end(); ++it1, ++n)
	{
		FUUUMove &curentMove = *it1;
		for (MoveCoordsList::iterator it2 = curentMove._coords.begin(); it2 != curentMove._coords.end(); ++it2)
		{
			Game::FieldAddress address = *it2;
			IPoint pos( Game::GetCenterPosition(address));
			pos.x = pos.x + (n-1)*10;
			std::string s = utils::lexical_cast(n);
			if (curentMove._arrowBonusesCount > 0 && curentMove._startBonus == address)
			{
				s = s + "*";
			}
			Render::PrintString(pos, s, 1.3f, CenterAlign, CenterAlign, false);
		}
	}
	Render::EndColor();
}
