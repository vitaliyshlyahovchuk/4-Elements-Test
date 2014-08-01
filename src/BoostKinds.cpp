#include "StdAfx.h"
#include "BoostKinds.h"
#include "GameInfo.h"
#include "GameField.h"
#include "ActCounter.h"
#include "Match3.h"
#include "Energy.h"
#include "FlyingThing.h"
#include "GameSquare.h"
#include "Match3Spirit.h"
#include "GameFieldControllers.h"
#include "FreeFront.h"
#include "Tutorial.h"
#include "MyApplication.h"
#include "GameLightningController.h"

// функции над матч-3

bool thisSquareHitByBoost(Game::FieldAddress fa)	//можно ли убрать эту фишку лопатой/линией/бомбой и т.п.()
{
	Game::Square *sq = GameSettings::gamefield[fa];
	Game::ChipColor chip = sq->GetChip();
			
	return (Game::isVisible(sq) && (sq->IsStone() || sq->GetWood()>0 || sq->GetWall()>0
				|| chip.IsSimpleChip()	|| chip.IsLicorice() || chip.IsThief() ));
}

//добавляет список поражаеиых бонусами клеток 
void addBonusAffected(AddressVector& _affectedCells)
{
	GameField::Get()->_bonusCascade.clear();

	std::map<Game::FieldAddress, bool> affectedByBonus;

	for (auto cell: _affectedCells)
	{
		AddressVector v;
		v.push_back(cell);
		GameField::Get()->CalcSubSequenceEffects(v, affectedByBonus);
	}

	for (auto i: affectedByBonus)
	{
		_affectedCells.push_back(i.first);
	}
}

void clearAffectedCells(AddressVector& _affectedCells)
{
	for (auto currentCell : _affectedCells) //перебираем клетки
	{
		//удаляем фишку и бонус
		Game::Square *sq = GameSettings::gamefield[currentCell];

		Game::ClearCell(sq, sq, Game::GetCenterPosition(currentCell), true, 0, true, ColorToMask(sq->GetChip().GetColor()), 50, false, true);
		sq->GetChip().GetHang().Clear();
	}
	
	//запускаем бонусы
	Game::AddController(new CombinedBonus(GameField::Get()->_bonusCascade, GameField::Get(), false, CombinedBonus::INSTANT));
	GameField::Get()->_bonusCascade.clear();

	//роняем
	Game::AddController(new BoostRunFallController(_affectedCells, 1.2f));
}

/////////////////// AbstractBoostKind ///////////////////////

bool AbstractBoostKind::getBuyBeforeLevel()       //можно ли покупать перед началом уровня
{
	return _buyBeforeLevel;
}

bool AbstractBoostKind::getBuyDuringLevel()        //можно ли покупать во время игры
{
	return _buyDuringLevel;
}

std::string AbstractBoostKind::getName()
{
	return _name;
}

bool AbstractBoostKind::IsInterfaceBoost()
{
	return _isInterfaceBoost;
}

void AbstractBoostKind::useBonusAndDecCount() //использует бонус и уменьшает запас
{
	Run();
	DecCount();
}

void AbstractBoostKind::IncCount() //увеличивает запас
{
	int currentCount = gameInfo.getLocalInt("Have"+_name, 0);
	gameInfo.setLocalInt("Have"+_name, currentCount + 1);
}

void AbstractBoostKind::DecCount() //использует бонус и уменьшает запас
{
	int currentCount = gameInfo.getLocalInt("Have"+_name, 0);
	MyAssert(currentCount > 0);
	if (currentCount > 0)
	{
		gameInfo.setLocalInt("Have"+_name, currentCount - 1);
		gameInfo.setLocalInt("used_boosts_count", gameInfo.getLocalInt("used_boosts_count") + 1);
	}
}

bool AbstractBoostKind::thisSquareHitByBoost(Game::FieldAddress fa)	//можно ли убрать эту фишку лопатой/линией/бомбой и т.п.()
{
	Game::Square *sq = GameSettings::gamefield[fa];
	Game::ChipColor chip = sq->GetChip();
			
	return (Game::isVisible(sq) && (sq->IsStone() || sq->GetWood()>0 || sq->GetWall()>0
				|| chip.IsSimpleChip()	|| chip.IsLicorice() || chip.IsThief() ) );
}

//получает список экранных прямогуольников из координат, для подсветки
void AbstractBoostKind::getScreenRectsFromAddressVector(AddressVector &addresses, std::vector<IRect> &rects)	
{
	for (Game::FieldAddress fa: addresses)
	{
		IPoint pt = GameSettings::ToScreenPos(fa.ToPoint() * GameSettings::SQUARE_SIDE);
		IRect rect(pt.x, pt.y, GameSettings::SQUARE_SIDE, GameSettings::SQUARE_SIDE);
		rect.Inflate(5);
		rects.push_back(rect);
	}
}

//действия при завершении работы - уменьшить запас и все такое
void AbstractBoostKind::FinalizeInterfaceBoost()
{
	GameField *gamefield = GameField::Get();
	//удаляем активный буст
	 gamefield ->_currentActiveBoost = BoostList::WeakPtr();
	 gameInfo.currentBoostTutorial = GameInfo::BoostTutorial();

	//чистим список фишек к уничтожению
	gamefield -> _chipSeqAffectedVisual.clear();

	//нужно обновить возможные ходы
	GameField::Get()->_maybeMoveGadget.NeedUpdate();

	//обновляем будущее энергии
	Gadgets::freeFrontDetector.Update();

	GameField::Get()->UseAffectedZonePulsation(false);
}

//включает или выключает доступность кнопки запуска буста
void AbstractBoostKind::SetCanApply(int value)
{
	Message messageCanApply("CanApplyBoost", value);
	Core::LuaCallVoidFunction("GameFunc", messageCanApply);
}

void AbstractBoostKind::OnFieldMoving(bool value)
{
	int intVal = !value;
	SetCanApply(intVal);
}


///////////////// SimpleBoostKind /////////////////////////

void SimpleBoostKind::UseBoost() //инициализация
{
	ShowAffected();
}

void SimpleBoostKind::Cancel() //инициализация
{
	HideAffected();
}

void SimpleBoostKind::Run() //выполняет действия буста
{
	HideAffected();

	Assert(_function != NULL);
	_function();
}

///////////////// BoostList ///////////////////////////////
void BoostList::addBoost(HardPtr newBoost)
{
	_boostList.push_back(newBoost);
}

const std::vector<BoostList::HardPtr> BoostList::getBoostList()
{
	return _boostList;
}

void UseBoostThreeMoreMoves();
void UseBoostSuperChip();
void UseBoostSpade();
void UseBoostReshuffle();
void UseBoostLittleBomb();
void UseBoostClearRandomChips();
void UseBoostClearColorBeforeGame();

//Стандартные бусты
//Внимание: при добавлении/удалении бусты необходимо исправить
//1. Здесь
//2. levelStart.lua - prepareBoostPanel - покупка/использование бонусов при старте; или match3_panels.lua prepareBoostPanel - бусты на уровне
//3. editor.lua - ApplyBoostSettings, SaveBoostSettings - настройка использования бонуса в уровнях
//4. в Edior_GUI.xml - чекбоксы для настройки (слой BoostConfig)
//5. GameSettings.xml - цены
//6. Картинку в booster_pictures
//7. Название и описание в тексты
//8. столбик в 4EF уровни чтобы забиралось скриптом в настройки
//9. исправить скрипт fillAllowBoosts.py чтобы правильно контролировал число бустов
//10. для бустов "до игры" добавить в FlyingBoosters.xml
//11. туториалы в BoostTutorials.xml (Нужно! написать функцию туториала в tutorial.lua)
void BoostList::StandardInit()
{
	//До игры
	addBoost(boost::make_shared<SimpleBoostKind>("BoostThreeMoreMoves","+3 фишки",true,false,&UseBoostThreeMoreMoves));
	addBoost(boost::make_shared<SimpleBoostKind>("BoostSuperChip","Супер-фишка",true,false,&UseBoostSuperChip));
	addBoost(boost::make_shared<SimpleBoostKind>("BoostLittleBomb","Маленький взрыв",true,false,&UseBoostLittleBomb));
	addBoost(boost::make_shared<SimpleBoostKind>("BoostClearColorBeforeGame","Удаление фишек одного цвета",true,false, &UseBoostClearColorBeforeGame));

	//Во время игры
	addBoost(boost::make_shared<BoostSpade>("BoostSpade","Лопата",false,true));
	addBoost(boost::make_shared<BoostExchange>("BoostExchange","Обмен",false,true));
	addBoost(boost::make_shared<SimpleBoostKind>("BoostReshuffle","Перемешивание",false,true,&UseBoostReshuffle));
	addBoost(boost::make_shared<BoostClearLine>("BoostClearHorizontalLine","Удалить ряд",false,true));
	addBoost(boost::make_shared<BoostCross>("BoostCross","Крест",false,true));
	addBoost(boost::make_shared<BoostBigBomb>("BoostBigBomb","Большая бомба",false,true));
	addBoost(boost::make_shared<BoostClearColor>("BoostClearColorInGame","Удаление фишек одного цвета",false,true));
	addBoost(boost::make_shared<SimpleBoostKind>("BoostClearRandomChips","Удаление случайных фишек",false,true,&UseBoostClearRandomChips));
	addBoost(boost::make_shared<BoostGrowEnergyAround>("BoostGrowEnergyAround","Увеличить область энергии",false,true));
	addBoost(boost::make_shared<BoostFreeSequence>("BoostFreeSequence","Любая цепочка",false,true));
}

BoostList::WeakPtr BoostList::FindByName(std::string name)
{
	for(std::vector<HardPtr>::iterator itr = _boostList.begin(); itr != _boostList.end(); ++itr)
	{
		HardPtr currentBoost = *itr;
		if (currentBoost.get()->getName() == name)
		{
			return WeakPtr (currentBoost);
		}
	}
	return WeakPtr();
}

void BoostList::AddUsedBoost(std::string bonusName)  //добавить в список бонус используемый до уровня
{
	BoostList::WeakPtr foundBoostWeak = FindByName(bonusName); //ищем
	if (!foundBoostWeak.expired())
	{
		GameField::Get()->_boostBeforeStart.push_back(foundBoostWeak); //если наши добавляем
	} else {
		Assert2(false,"Unknow boost "+bonusName);
	}

}

bool BoostList::NeedConfirmationForBoost(std::string bonusName)  //требуется ли подтверждение использования буста
{
	BoostList::WeakPtr foundBoostWeak = FindByName(bonusName); //ищем
	if (BoostList::HardPtr boost = foundBoostWeak.lock())
	{
		return ! boost->IsInterfaceBoost(); //интерфейсным не требуется подтверждения, их и так можно отменить
	} else {
		Assert2(false,"Unknow boost "+bonusName);
	}
	return false;
}

bool BoostList::CanStartBoostNow(std::string bonusName)//можно ли запустить этот буст сейчас (например нельзя пока камера едет)
{
	return GameField::Get()->IsStandby(); //пока что считаем что можно запускать только если поле успокоилось
}


void BoostList::UseBoostInstantly(std::string bonusName) //использовать бонус (вывызвается из lua во время игры)
{
	BoostList::WeakPtr foundBoostWeak = FindByName(bonusName); //ищем
	if (!foundBoostWeak.expired()) //если нашли
	{
		BoostList::HardPtr currentBoost = foundBoostWeak.lock();
//		currentBoost -> _flashButtonNumber = buttonNumber;	//запоминаем номер кнопки
//		currentBoost -> _flashButtonRect = buttonRect;
		GameField::Get()->_currentActiveBoost = BoostList::WeakPtr(currentBoost);	//устанавливаем активный буст

		currentBoost->UseBoost(); //используем (фактически для интерфейсных бустов это инициализация)

		Tutorial::luaTutorial.AcceptMessage( Message("OnBoostSelect") );
	} else {
		Assert2(false,"Unknow boost "+bonusName);
	}
}

void BoostList::CancelCurrentBoost(BoostList::WeakPtr boost) //отмена интрефейсного буста
{
	if (BoostList::HardPtr currentActiveBoost = boost.lock())
	{
		currentActiveBoost->Cancel();
	}
}

void BoostList::RunCurrentBoost(BoostList::WeakPtr boost) //выполнение интрефейсного буста
{
	if (BoostList::HardPtr currentActiveBoost = boost.lock())
	{
		currentActiveBoost->Run();      //запускаем
		currentActiveBoost->DecCount(); //уменьшаем запас
	}
}

Game::FieldAddress BoostList::EmptyAddres = Game::FieldAddress(-1,-1);


//////////////////// Основные функции простых бустов ////////////////////////////////////////////

void DoAddThreeMoves()
{
	Match3GUI::ActCounter::ChangeCounter(gameInfo.getConstInt("BoostThreeMoreMovesActuallyAdds", 3));
}

void UseBoostThreeMoreMoves() //+3 хода
{
	//Game::AddController(new ShowBoostMovesFlyingText(GameField::Get(), &DoAddThreeMoves));
	DoAddThreeMoves();
}

void UseBoostSuperChip() //суперфишка (с поставленным бонусом)
{
	//ищем все фишки
	AddressVector chips;

	bool isSuperChipTutorial = gameInfo.currentBoostTutorial.name == "BoostSuperChip";

	for(int x = 0; x < Game::activeRect.Width();x++)
	{
		for(int y = 0; y < Game::activeRect.Height();y++)
		{
			Game::FieldAddress fa = Game::FieldAddress(Game::activeRect.x + x, Game::activeRect.y + y);
			Game::Square *sq = GameSettings::gamefield[fa];

			if (Game::CanHangBonusOnSquare(sq, false))
			{
				chips.push_back(sq->address);
			}
		}
	}

	//куда ставить суперфишки
	AddressVector superChips;

	if (isSuperChipTutorial && !gameInfo.currentBoostTutorial.points.empty()) {
		//проедопределённые в туториале
		std::vector<IPoint> points = gameInfo.currentBoostTutorial.points;
		for (IPoint point : points) {
			superChips.push_back(Game::FieldAddress(point));
		}
	} else {
		int needSuperChips = gameInfo.getConstInt("BoostSuperChipCount", 2);
		if (needSuperChips > static_cast<int>(chips.size())) {
			needSuperChips = chips.size();
		}
		//выбираем случайно скоко надо
		for ( ; needSuperChips > 0; --needSuperChips)
		{
			size_t n = math::random(0u, chips.size()-1);
			superChips.push_back(chips[n]);
			chips.erase(chips.begin() + n);
		}
	}

	//выставляем
	for (auto fa:superChips)
	{
		AddressVector seq;
		seq.push_back( Game::FieldAddress() );
		seq.push_back( Game::FieldAddress() );

		Game::Hang hang;
		hang.MakeArrow(11, math::random(0,1) ? 
			 (Game::Hang::ARROW_R|Game::Hang::ARROW_L) : (Game::Hang::ARROW_U|Game::Hang::ARROW_D));

		//летит примерно из правого верхнего угла
		//IPoint from = GameSettings::FIELD_SCREEN_CONST.RightTop() - IPoint(math::random(100, 300), math::random(100, 300));
		IPoint from = IPoint(MyApplication::GAME_WIDTH/2, 200) - IPoint(80, 50) + IPoint(math::random(0, 160), math::random(0, 100));

		Game::AddController(new Game::Spirit(
				from, IPoint(-1, -1),
				std::string("screen"), hang, seq, fa, 0.f, -1)); 	
	}

	//if (!chips.empty())
	//{
	//	//если нашли, приделываем бонус
	//	size_t n = math::random(0u, chips.size()-1);
	//	Game::FieldAddress fa = chips[n];

	//	AddressVector seq;
	//	seq.push_back( Game::FieldAddress() );
	//	seq.push_back( Game::FieldAddress() );

	//	Game::Hang hang;
	//	hang.MakeArrow(11, math::random(0,1) ? 
	//		 (Game::Hang::ARROW_R|Game::Hang::ARROW_L) : (Game::Hang::ARROW_U|Game::Hang::ARROW_D));

	//	Game::AddController(new Game::Spirit(
	//		GameSettings::FIELD_SCREEN_CONST.RightTop() - IPoint(100,100), //летит примерно из правого верхнего угла
	//		std::string("screen"), hang, seq, fa)); 

	//	//GameSettings::gamefield[fa]->GetChip().GetHang().MakeArrow(11, math::random(0,1) ? 
	//	//	 (Game::Hang::ARROW_R|Game::Hang::ARROW_L) : (Game::Hang::ARROW_U|Game::Hang::ARROW_D));
	//}
}

void UseBoostLittleBomb() //Маленький взрыв
{
	GameField::Get()->_useBoostLittleBomb = true;
}

void UseBoostReshuffle() //перемешивание
{
	GameField::Get()->ReshuffleField();
}

void MeteorDrawer()
{
	FRect rect;
	FRect frect;
	Game::GetUpElementRects(Game::UP_STONE_0, rect, frect);
	Game::upElementsTexture->Draw(rect, frect);
}

void MeteorFinisher(void *pt)
{
	Game::FieldAddress *pt_to_fa = static_cast<Game::FieldAddress *> (pt);

	AddressVector chips;
	AddressVector chipsBonus;

	chips.push_back(*pt_to_fa);
	chipsBonus = chips;

	//добавляем бонусы
	//chipsBonus тут на самом деле не нужен, важно что заполняется _bonusCascade
	addBonusAffected(chipsBonus);

	//удаляем только исходную фишку. остальное удалится само
	clearAffectedCells(chips);

	//обновляем будущее энергии
	Gadgets::freeFrontDetector.Update();

	delete pt_to_fa;
}

void UseBoostClearRandomChips() //удаление случайных фишек
{
	AddressVector chips;

	//находим все фишки
	for(int x = 0; x < Game::activeRect.Width();x++)
	{
		for(int y = 0; y < Game::activeRect.Height();y++)
		{
			Game::FieldAddress fa = Game::FieldAddress(Game::activeRect.x + x, Game::activeRect.y + y);
			if (thisSquareHitByBoost(fa)) 
			{
				chips.push_back(fa);
			}
		}
	}

	//сколько надо грохнуть
	size_t clearCount = gameInfo.getConstInt("BoostClearRandomChipsCount", 10);
	if (clearCount > chips.size()) //ограничиваем
	{
		clearCount = chips.size();
	}

	AddressVector chipsToKill;
	//заполняем список кого грохаем
	for (; clearCount>0; --clearCount)
	{
		size_t n = chips.size();
		size_t i = math::random(0u, n-1);

		chipsToKill.push_back(chips[i]);
		chips.erase(chips.begin() + i);
	}

	//запускаем метеоры. по окончании вызовется MeteorFinisher и грохнется само
	for (auto fa: chipsToKill)
	{
		//куда лететь
		FPoint ptTo = GameSettings::ToScreenPos(fa.ToPoint() * GameSettings::SQUARE_SIDE);

		//откуда
		FPoint ptFrom;
		ptFrom.y = GameSettings::FIELD_SCREEN_CONST.height - math::random(0,100);
		ptFrom.x = ptTo.x + math::random(0,100);

		Game::FieldAddress *pt_to_fa = new Game::FieldAddress(fa);

		//запускаем
		FlyingThing *f = new FlyingThing(ptFrom, ptTo, 1, &MeteorDrawer, &MeteorFinisher, pt_to_fa);
		Game::AddController(f);

	}


}

void UseBoostClearColorBeforeGame() // удаление фишек одного цвета перед игрой
{
	// проверим все фишки
	AddressVector _allowCells;

	for(int x = 0; x < GameSettings::gamefield.Width(); x++)
	{
		for(int y = 0; y < GameSettings::gamefield.Height(); y++)
		{
			Game::FieldAddress fa = Game::FieldAddress(x, y);
			Game::Square *sq = GameSettings::gamefield[fa];
			Game::ChipColor chip = sq->GetChip();
			
			if (Game::isVisible(sq) && chip.IsSimpleChip()	&& !sq->IsIce())
			{
				_allowCells.push_back(fa);
			}
		}
	}

	ColorMask colors = 0; // цвета, которые будем перекрашивать
	ColorMask remainingColors = 0; // оставшиеся цвета
	std::vector<int> colorsToController;
	int colorscount = 0;
	while (colorscount < 2) {
		int randomColor = Gadgets::levelColors.GetRandom();
		if (!ColorInMask(colors, randomColor)) {
			colors |= ColorToMask(randomColor);
			colorscount++;
		}
	}

	// найдём все фишки которые будем перекрашивать
	AddressVector chipsToRecolorer; // фишки для перекраски (для контроллера)
	for (auto fa:_allowCells)
	{
		Game::Square *fa_sq = GameSettings::gamefield[fa];
		int color = fa_sq->GetChip().GetColor();
		if (ColorInMask(colors, color)) {
			chipsToRecolorer.push_back(fa);
		} else {
			if (!ColorInMask(remainingColors, color)) {
				remainingColors |= ColorToMask(color);
				colorsToController.push_back(color);
				
			}
		}
	}

	// отправляем фишки на перекраску
	ChipsRecolorController *recolorer = new ChipsRecolorController(chipsToRecolorer, colorsToController, 0.3f);
	Game::AddController(recolorer);
}

////// буст рост энергии. умеет показывать куда он вырастет //////////////////////////////////////////

void GetEnergyGrowCells(std::set<Game::FieldAddress> &grow)
{
	AddressVector energy;

	//обновляем будущее энергии на всякий пожарный
	Gadgets::freeFrontDetector.Update();

	//находим всю энергию
	for(int x = 0; x < Game::activeRect.Width();x++)
	{
		for(int y = 0; y < Game::activeRect.Height();y++)
		{
			Game::FieldAddress fa = Game::FieldAddress(Game::activeRect.x + x, Game::activeRect.y + y);
			Game::Square *sq = GameSettings::gamefield[fa];
			if (sq && Game::isVisible(sq) && sq->IsEnergyChecked(true)) 
			{
				energy.push_back(fa);
			}
		}
	}

	grow.clear();
	//находим соседей
	for (auto fa:energy)
	{
		AddressVector directions;
		fa.FillDirections4(directions);
		//перебираем 4 соседей
		for (auto faTry:directions)
		{
			Game::Square *sq = GameSettings::gamefield[faTry];
			if (Game::isVisible(sq) && (!sq->IsHardStand() && !sq->IsIce() && sq->GetWall() > 0))
			{
				grow.insert(faTry);
			}
		}
	}
}

void BoostGrowEnergyAround::ShowAffected()            //показать область поражения (если требуется).
{
	std::set<Game::FieldAddress> grow;
	GetEnergyGrowCells(grow);

	GameField *gamefield = GameField::Get();
	gamefield -> _chipSeqAffectedVisual.clear();
	for (auto fa: grow)
	{
		gamefield -> _chipSeqAffectedVisual.insert(std::make_pair(fa, true));
	}
}

void BoostGrowEnergyAround::HideAffected()            //убрать область поражения (если требуется).
{
	GameField *gamefield = GameField::Get();
	gamefield -> _chipSeqAffectedVisual.clear();
}

void BoostGrowEnergyAround::Run() //рост энергии
{
	std::set<Game::FieldAddress> grow;
	GetEnergyGrowCells(grow);

	//убираем землю
	for (auto fa:grow)
	{
		Game::Square *sq = GameSettings::gamefield[fa];

		Game::ClearCell(sq, sq, Game::GetCenterPosition(fa), true, 0, true, ColorToMask(sq->GetChip().GetColor()), 50, true);		
	}

	//обновляем будущее энергии
	Gadgets::freeFrontDetector.Update();
}


/////////////////////////////////// контроллер клика //////////////////////////////////////////////

SquareClickController::SquareClickController(AbstractBoostKind *boost)
	: GameFieldController("SquareClick", 2.0f, GameField::Get())
	, _boost(boost)
{

}

bool SquareClickController::MouseDown(const IPoint &mouse_pos)
{   //запоминаем куда кликнули
	_addresClicked = GameSettings::GetMouseAddress(mouse_pos - Game::ChipColor::CHIP_TAP_OFFSET);
	_boost->MouseDown(_addresClicked); //говорим бусту
	return true;
}


bool SquareClickController::MouseUp(const IPoint &mouse_pos)
{
	Game::FieldAddress fa = GameSettings::GetMouseAddress(mouse_pos - Game::ChipColor::CHIP_TAP_OFFSET);
	_boost->MouseUp(fa); //говорим бусту
	if (fa == _addresClicked) //если клик далеко не уехал
	{
		//клик состоялся
		_boost->DoAddresClick(_addresClicked);
	}
	return true;
}

bool SquareClickController::MouseMove(const IPoint &mouse_pos)
{   
	Game::FieldAddress fa = GameSettings::GetMouseAddress(mouse_pos - Game::ChipColor::CHIP_TAP_OFFSET);
	_boost->MouseMove(fa);
	_boost->MouseMove(mouse_pos);
	return true;
}



///////////////////////////// Абстрактный буст типа "кликни и взорвется" //////////////////////////////////////////////////

void AbstractClickAndDeleteBoost::AddIfAllow(Game::FieldAddress fa, AddressVector &cells)
{
	if (std::find(_allowClick.begin(), _allowClick.end(), fa) != _allowClick.end())
	{
		cells.push_back(fa);
	}
}

void AbstractClickAndDeleteBoost::SelectRandomCurrentCell() 
{
	//выбираем случайную стартовую клетку
	if (!_allowClick.empty())
	{
		//центр
		int center_x = Game::activeRect.x + Game::activeRect.Width()  / 2 - 1;
		int center_y = Game::activeRect.y + Game::activeRect.Height() / 2 - 1;

		//список клеток из которых будем выбирать
		AddressVector  cells;

		//центральную клетку
		AddIfAllow(Game::FieldAddress(center_x, center_y), cells);

		//расстояние
		int distance = 1;
		//заполняем как минимум центральный квадрат 5*5, или дальше если в квадрате ничего нет
		while ((distance <= 2 || cells.empty()) && distance <10)
		{
			for (int i=0; i<distance*2; ++i)
			{
				AddIfAllow(Game::FieldAddress(center_x - distance  + i, center_y + distance    ), cells);
				AddIfAllow(Game::FieldAddress(center_x + distance  - i, center_y - distance    ), cells);
				AddIfAllow(Game::FieldAddress(center_x - distance     , center_y - distance + i), cells);
				AddIfAllow(Game::FieldAddress(center_x + distance     , center_y + distance - i), cells);
			}

			distance++;
		}
		//если хоть что то нашли, выбираем случайную и ставим как активную
		if (!cells.empty())
		{
			size_t i = math::random(0u, cells.size()-1);

			ProcessAddresSelect(cells[i]);
			//_currentCell = cells[i];
			//UpdateAffectedCells();
		}
	}
}

void AbstractClickAndDeleteBoost::UseBoost() //инициализация
{
	//чистим
	_currentCell = BoostList::EmptyAddres;
	UpdateAffectedCells();
	//заполняем список разрешенных
	FillAllowClick();

	//контроллер клика
	_squareClick = new SquareClickController(this);
	Game::AddController(_squareClick);

	// выберем случайную клетку если сейчас не туториал на этот буст
	if (gameInfo.currentBoostTutorial.name != getName()) {
		SelectRandomCurrentCell();
	}

	GameField::Get()->UseAffectedZonePulsation(true);
}

void AbstractClickAndDeleteBoost::CountFromCurrentCell() //вызывается после изменения _currentCell
{
	UpdateAffectedCells();
	GameField *gamefield = GameField::Get();
	gamefield -> _chipSeqAffectedVisual.clear();
	for (auto fa: _affectedCells)
	{
		gamefield -> _chipSeqAffectedVisual.insert(std::make_pair(fa, true));
	}

	//если сейчас туториал на этот буст кнопку активируем/деактивируем в коде туториала (tutorial.lua)
	if (gameInfo.currentBoostTutorial.name == getName()) return;

	//сообщаем lua что активный квадрат поменялся, чтобы поменять иконку
	Message message ("ShowCurrentBoostSquare");

	if (_currentCell == BoostList::EmptyAddres) { //если не выбрано, надо спрятать стрелочку
		message.getVariables().setInt("x",-1);
		message.getVariables().setInt("y",-1);

		SetCanApply(0);
	} else {
		IPoint pt = GameSettings::ToScreenPos(_currentCell.ToPoint() * GameSettings::SQUARE_SIDE);
		message.getVariables().setInt("x",pt.x);
		message.getVariables().setInt("y",pt.y);

		SetCanApply(1);
	}

	Core::LuaCallVoidFunction("GameFunc", message);
}

//щелчок по клетке - персчитываем зону поражения
void AbstractClickAndDeleteBoost::DoAddresClick(Game::FieldAddress destination)
{
	//пока поле не успокоится, не реагируем
	if (!GameField::Get()->IsStandby() && destination != BoostList::EmptyAddres)
	{
		return;
	}

	ProcessAddresSelect(destination);
}

void AbstractClickAndDeleteBoost::ProcessAddresSelect(Game::FieldAddress destination)
{
	if (destination != _currentCell && 
		(std::find(_allowClick.begin(), _allowClick.end(), destination) != _allowClick.end() || destination == BoostList::EmptyAddres)) //если изменилась пересчитываем
	{
		if (gameInfo.currentBoostTutorial.name == getName()) {
			// если сейчас туториал на данный буст разрешаем кликать только на подсвеченные
			Game::Square *sq = GameSettings::gamefield[destination];
			if (!sq->GetChip().IsChainHighlighted()) return;
		}
		_currentCell = destination;
		CountFromCurrentCell();
		Tutorial::luaTutorial.AcceptMessage( Message("OnChipSelect") );
	}
}


void AbstractClickAndDeleteBoost::Run()
{
	Tutorial::luaTutorial.AcceptMessage( Message("OnRunBoost") );
	//удаляем клетки
	DoBoostAction();
	//убираем контроллеры
	Game::KillControllers(_squareClick -> getName());
	//завершаем работу
	FinalizeInterfaceBoost();
}


void AbstractClickAndDeleteBoost::MouseDown(Game::FieldAddress pos)
{
//	MouseMove(pos);
}

//при движении с нажатием пересчитываем эффект
void AbstractClickAndDeleteBoost::MouseMove(Game::FieldAddress pos)
{

}

//при отжатии активируем буст
void AbstractClickAndDeleteBoost::MouseUp(Game::FieldAddress pos)
{

}

void AbstractClickAndDeleteBoost::AcceptMessage(const Message &message)
{

}

void AbstractClickAndDeleteBoost::Cancel()
{
	//убираем контроллеры
	Game::KillControllers(_squareClick -> getName());
	//чистим список бонусов
	GameField::Get()->_bonusCascade.clear();
	GameField::Get()->UseAffectedZonePulsation(false);
}

//прибавляет к _affectedCells еще клетки поражаемые бонусами
void AbstractClickAndDeleteBoost::AddBonusAffected()
{

	_affectedCells = _sequenceCells; //сохраняем исходную "цепочку"

	addBonusAffected(_affectedCells);
}

void AbstractClickAndDeleteBoost::ClearAfectedCells()  //стандартный механизм удаления клеток
{
	UpdateAffectedCells();

	clearAffectedCells(_sequenceCells);
}

void AbstractClickAndDeleteBoost::OnFieldMoving(bool value)
{
	if (value) //если двигается, отменяем выбранную клетку
	{
		ProcessAddresSelect(BoostList::EmptyAddres);
	} else {
		FillAllowClick(); //перезаполняем разрешенные клетки
		SelectRandomCurrentCell();// если перестало то выбираем случайную клетку
	}
}


/////////////////////////////////// Буст лопата ////////////////////////////////////////

void BoostSpade::FillAllowClick()  //заполнить список разрешенных для клика ячеек !! нужно перекрыть
{
	_allowClick.clear();
	for(int x = 0; x < Game::activeRect.Width();x++)
	{
		for(int y = 0; y < Game::activeRect.Height();y++)
		{
			Game::FieldAddress fa = Game::FieldAddress(Game::activeRect.x + x, Game::activeRect.y + y);
			if (thisSquareHitByBoost(fa))
			{
				_allowClick.push_back(fa);
			}
		}
	}
}

void BoostSpade::UpdateAffectedCells()                   //обновляет список убиваемых фишек !! нужно перекрыть
{
	//чистим список
	_sequenceCells.clear();
	_affectedCells.clear();
	//если не выделено, выходим
	if (_currentCell == BoostList::EmptyAddres)
	{
		return;
	}

	//добавляем текущую
	_sequenceCells.push_back(_currentCell);

	//дополняем бонусы
	AddBonusAffected();
}



void BoostSpade::DoBoostAction()                   //выполняет действия буста 
{
	ClearAfectedCells();
}


///////////////////////////// буст удаление линии //////////////////////////////////////////////////

//щелчок по клетке - персчитываем зону поражения
void BoostClearLine::DoAddresClick(Game::FieldAddress destination)
{
	//пока поле не успокоится, не реагируем
	if (!GameField::Get()->IsStandby() && destination != BoostList::EmptyAddres)
	{
		return;
	}
	
	if (std::find(_allowClick.begin(), _allowClick.end(), destination) != _allowClick.end()   || destination == BoostList::EmptyAddres)
	{
		if (destination == _currentCell)   //клик по той же клетке - меняем направление
		{
			if (_currentCell != BoostList::EmptyAddres)
			{
				_isHorizontal = !_isHorizontal;
			}
		} else {   //клик по другой клетке - меняем текущую клетку
			_currentCell = destination;
		}
		CountFromCurrentCell();
	}
}

void BoostClearLine::FillAllowClick()  //заполнить список разрешенных для клика ячеек !! нужно перекрыть
{
	_allowClick.clear();
	for(int x = 0; x < Game::activeRect.Width();x++)
	{
		for(int y = 0; y < Game::activeRect.Height();y++)
		{
			Game::FieldAddress fa = Game::FieldAddress(Game::activeRect.x + x, Game::activeRect.y + y);
			Game::Square *sq = GameSettings::gamefield[fa];
			if (sq && !sq->IsFake()){
				_allowClick.push_back(fa);
			}
		}
	}
}

//обновляет список убиваемых фишек
void BoostClearLine::UpdateAffectedCells() 
{
	_sequenceCells.clear();
	_affectedCells.clear();

	if (_currentCell != BoostList::EmptyAddres && Game::activeRect.Contains(_currentCell.ToPoint()) )
	{
		Game::FieldAddress checkPoint; //начальная точка
		Game::FieldAddress moveBy; //приращение
		if (_isHorizontal) //слева направо
		{
			checkPoint.SetCol(Game::activeRect.x);
			checkPoint.SetRow(_currentCell.GetRow());
			moveBy = Game::FieldAddress::RIGHT;
		} else { //снизу вверх
			checkPoint.SetCol(_currentCell.GetCol());
			checkPoint.SetRow(Game::activeRect.y);
			moveBy = Game::FieldAddress::UP;
		}

		bool firstFakes = false;
		for ( ;Game::activeRect.Contains(checkPoint.ToPoint()); checkPoint += moveBy)
		{
			if (!firstFakes) { // фейковые сначала добавлять не будем
				Game::Square *sq = GameSettings::gamefield[checkPoint];
				if (sq && !sq->IsFake()) {
					_sequenceCells.push_back(checkPoint); //добавляем
					firstFakes = true;
				}
			} else {
				_sequenceCells.push_back(checkPoint); //добавляем
			}
		}

		//удалим фейковые с конца
		int count = 0;
		for(AddressVector::reverse_iterator itr = _sequenceCells.rbegin(); itr != _sequenceCells.rend(); ++itr ) {
			Game::Square *sq = GameSettings::gamefield[*itr];
			if (sq && sq->IsFake()) {
				count++;
			} else {
				break;
			}
		}
		for (int i = 0; i < count; i++) {
			_sequenceCells.pop_back();
		}

		//дополняем бонусы
		AddBonusAffected();
	}
}

void BoostClearLine::DoBoostAction()                   //выполняет действия буста 
{
	ClearAfectedCells();
}

////////////////////// Буст крест, убирает фишки по горизонтали и вертикали///////////////////////

void BoostCross::FillAllowClick()  //заполнить список разрешенных для клика ячеек !! нужно перекрыть
{
	fieldBound = IRect();
	for(int x = 0; x < Game::activeRect.Width();x++) {
		for(int y = 0; y < Game::activeRect.Height();y++) {
			Game::FieldAddress fa = Game::FieldAddress(Game::activeRect.x + x, Game::activeRect.y + y);
			Game::Square *sq = GameSettings::gamefield[fa];
			if (sq && !sq->IsFake()){
				if (fieldBound.x == 0) {
					fieldBound.x = fa.ToPoint().x;
				}
				if (fieldBound.y == 0) {
					fieldBound.y = fa.ToPoint().y;
				}
				if (fieldBound.width < fa.ToPoint().x) {
					fieldBound.width = fa.ToPoint().x;
				}
				if (fieldBound.height < fa.ToPoint().y) {
					fieldBound.height = fa.ToPoint().y;
				}
			}
		}
	}

	_allowClick.clear();
	for(int x = 0; x < Game::activeRect.Width();x++)
	{
		for(int y = 0; y < Game::activeRect.Height();y++)
		{
			Game::FieldAddress fa = Game::FieldAddress(Game::activeRect.x + x, Game::activeRect.y + y);
			Game::Square *sq = GameSettings::gamefield[fa];
			if (sq && fieldBound.Contains(fa.ToPoint())){
				_allowClick.push_back(fa);
			}
		}
	}
}

//обновляет список убиваемых фишек
void BoostCross::UpdateAffectedCells() 
{
	_sequenceCells.clear();
	_affectedCells.clear();

	if (_currentCell != BoostList::EmptyAddres && Game::activeRect.Contains(_currentCell.ToPoint()) )
	{
		Game::FieldAddress checkPoint; //начальная точка
		Game::FieldAddress moveBy; //приращение

		//слева направо
		checkPoint.SetCol(Game::activeRect.x);
		checkPoint.SetRow(_currentCell.GetRow());
		moveBy = Game::FieldAddress::RIGHT;
		for ( ;Game::activeRect.Contains(checkPoint.ToPoint()); checkPoint += moveBy)
		{
			Game::Square *sq = GameSettings::gamefield[checkPoint];
			if (sq && fieldBound.Contains(checkPoint.ToPoint())) {
				_sequenceCells.push_back(checkPoint); //добавляем
			}
		}

		//снизу вверх
		checkPoint.SetCol(_currentCell.GetCol());
		checkPoint.SetRow(Game::activeRect.y);
		moveBy = Game::FieldAddress::UP;
		for ( ;Game::activeRect.Contains(checkPoint.ToPoint()); checkPoint += moveBy)
		{
			Game::Square *sq = GameSettings::gamefield[checkPoint];
			if (sq && fieldBound.Contains(checkPoint.ToPoint())) {
				_sequenceCells.push_back(checkPoint); //добавляем
			}
		}

		//дополняем бонусы
		AddBonusAffected();
	}
}

void BoostCross::DoBoostAction()                   //выполняет действия буста 
{
	// создаём бонус струлу(крест)
	Game::Hang hang;
	hang.MakeArrow(11, Game::Hang::ARROW_4);
	Game::AddController(new CombinedBonus(_currentCell, hang, GameField::Get(), false));

	// запустим бонусы
	Game::AddController(new CombinedBonus(GameField::Get()->_bonusCascade, GameField::Get(), false, CombinedBonus::INSTANT));
	GameField::Get()->_bonusCascade.clear();

	// разлачиваем все блокировки клеток
	for(int x = Game::visibleRect.LeftBottom().x; x <= Game::visibleRect.RightTop().x; x++)
	{
		for(int y = Game::visibleRect.LeftBottom().y; y <= Game::visibleRect.RightTop().y; y++)
		{
			Game::Square *sq = GameSettings::gamefield[IPoint(x, y)];
			if(!Game::isBuffer(sq))
			{
				sq->SetBusyNear(0);
				sq->SetBusyCell(0);
			}
		}	
	}

	//ClearAfectedCells();
}

void BoostCross::AcceptMessage(const Message &message)
{
	if (message.is("SelectRandomChipForCross")) {
		SelectRandomCurrentCell();
		CountFromCurrentCell();
	} else if (message.is("GetAffectedChips")) {
		Message msg = Message("TutorialCells");
		int count = (int)_affectedCells.size();
		msg.getVariables().setInt("count", count);
		for (int i = 0; i < count; i++) {
			msg.getVariables().setInt("x"+utils::lexical_cast(i+1), _affectedCells[i].GetCol());
			msg.getVariables().setInt("y"+utils::lexical_cast(i+1), _affectedCells[i].GetRow());
		}
		Core::LuaCallVoidFunction("fillTutorialCells", msg);
	}
}

/////////////////////////////////// Буст обмен фишек ////////////////////////////////////////
void BoostExchange::UseBoost() 
{
	_firstChipSelected = false;
	_secondChipSelected = false;
	//инициализируем список кого можно менять
	FillAllowExchange();

	//контроллер клика
	_squareClick = new SquareClickController(this);
	Game::AddController(_squareClick);
	//обновляем состояние чтобы спрятать зеленую кнопочку 
	UpdateLuaState();
}

void BoostExchange::FillAllowExchange()	//заполняет список кого можно менять
{
	_allowExchange.clear();
	for(int x = 0; x < Game::activeRect.Width();x++)
	{
		for(int y = 0; y < Game::activeRect.Height();y++)
		{
			Game::FieldAddress fa = Game::FieldAddress(Game::activeRect.x + x, Game::activeRect.y + y);
			Game::Square *sq = GameSettings::gamefield[fa];
			Game::ChipColor chip = sq->GetChip();
			
			if (Game::isVisible(sq) && chip.IsSimpleChip()	&& !sq->IsIce())
			{
				_allowExchange.push_back(fa);
			}
		}
	}
}


void BoostExchange::UpdateLuaState()
{
	//говорим включить или выключить зеленую кнопку
	SetCanApply((int)(_firstChipSelected && _secondChipSelected));

	//сообщаем lua что активный квадрат поменялся, чтобы поменять иконку
	IPoint pt;
	Message message ("ShowSquare");
	message.getVariables().setInt("n",1);

	if (_firstChipSelected)
	{
		pt = GameSettings::ToScreenPos(_firstChip.ToPoint() * GameSettings::SQUARE_SIDE);
		message.getVariables().setInt("x",pt.x);
		message.getVariables().setInt("y",pt.y);
	} else {
		message.getVariables().setInt("x", -1); //признак что нужно спрятать
		message.getVariables().setInt("y", -1);
	}

	Core::LuaCallVoidFunction("GameFunc", message);

	//то же для 2 квадратика
	message.getVariables().setInt("n",2);

	if (_secondChipSelected)
	{
		pt = GameSettings::ToScreenPos(_secondChip.ToPoint() * GameSettings::SQUARE_SIDE);
		message.getVariables().setInt("x",pt.x);
		message.getVariables().setInt("y",pt.y);
	} else {
		message.getVariables().setInt("x", -1); //признак что нужно спрятать
		message.getVariables().setInt("y", -1);
	}

	Core::LuaCallVoidFunction("GameFunc", message);
}

//обработка щелчка по клетке
void BoostExchange::DoAddresClick(Game::FieldAddress destination)
{
	//пока поле не успокоится, не реагируем
	if (!GameField::Get()->IsStandby() && destination != BoostList::EmptyAddres)
	{
		return;
	}

	AddressVector::iterator find = std::find(_allowExchange.begin(), _allowExchange.end(), destination); //проверяем разрешена ли клетка
	if (find == _allowExchange.end()) //кликнули где то не там
	{
		return;
	}

	//щелчок по выделенной отменяет ее
	if (_firstChipSelected && destination == _firstChip)
	{
		_firstChipSelected = _secondChipSelected; //вторая становится первой (если была)
		_firstChip = _secondChip;
		_secondChipSelected = false; // а саму вторую убираем
	} else if (_secondChipSelected && destination == _secondChip) {
		_secondChipSelected = false; //если вторая то просто отменяем
	
	//теперь просто щелчки
	} else if (!_firstChipSelected) {
		_firstChipSelected = true;
		_firstChip = destination;
	} else {
		_secondChipSelected = true; //если обе клетки выделены и еще где то тыкают
		_secondChip = destination;  //то переставляем вторую
	}

	//таким образом теперь _firstChip и _secondChip правильные. все обновляем
	UpdateLuaState();
}

void BoostExchange::Run()
{
	Assert(_firstChipSelected && _secondChipSelected);

	//меняем
	Game::Square *from = GameSettings::gamefield[_firstChip];
	Game::Square *to = GameSettings::gamefield[_secondChip];

	std::swap(from->GetChip(), to->GetChip());

	//анимируем
	FPoint toPos =  from->GetCellPos() - to->GetCellPos();  
	FPoint fromPos = /* from->GetChip().GetPos() +*/ to->GetCellPos() - from->GetCellPos();

	FPoint empty(0,0);

	if (toPos != empty) 
	{
		float speed = toPos.Length();
		to->GetChip().SetPos(toPos);
		to->GetChip().RunMove(0, 2.0f * speed, 10 * GameSettings::SQUARE_SIDEF);
	}

	if (fromPos != empty)
	{
		float speed = toPos.Length();
		from->GetChip().SetPos(fromPos);
		from->GetChip().RunMove(0, 2.0f * speed, 10 * GameSettings::SQUARE_SIDEF);
	}


	//убираем контроллеры
	Game::KillControllers(_squareClick -> getName());

	//завершаем
	FinalizeInterfaceBoost();
}

void BoostExchange::Draw() //рисуем рамку вокруг первой выбранной фишки
{
	//if (_firstChipSelected)
	//{
	//	IRect rect = Game::GetCellRect(_firstChip);
	//	rect.Inflate(-5);

	//	Render::device.SetTexturing(false);
	//	Render::BeginColor(Color::BLACK);
	//	Render::DrawFrame(rect);
	//	Render::EndColor();
	//	Render::device.SetTexturing(true);
	//}
}

void BoostExchange::Cancel()
{
	//убираем контроллеры
	Game::KillControllers(_squareClick -> getName());
}

void BoostExchange::OnFieldMoving(bool value)
{
	if (value) //если двигается, отменяем весь выбор
	{
		_firstChipSelected = false;
		_secondChipSelected = false;
		UpdateLuaState();
	} else {
		FillAllowExchange(); //перезаполняем разрешенные клетки
		//когда остановилось не трогаем, пусть пользователь руками выбирает че ему надо
	}
}

//////////////////////////////// Буст - большая бомба ////////////////////
void BoostBigBomb::FillAllowClick()  //заполнить список разрешенных для клика ячеек 
{
	_allowClick.clear();
	for(int x = 0; x < Game::activeRect.Width();x++)
	{
		for(int y = 0; y < Game::activeRect.Height();y++)
		{
			Game::FieldAddress fa = Game::FieldAddress(Game::activeRect.x + x, Game::activeRect.y + y);
			_allowClick.push_back(fa);
		}
	}
}

void BoostBigBomb::UpdateAffectedCells()                   //обновляет список убиваемых фишек 
{
	//копипаст из бонуса - бомбочки. рассчитывает поражаемые ячейки в соответствии с настройками
	_sequenceCells.clear();
	_affectedCells.clear();

	if (_currentCell != BoostList::EmptyAddres && Game::activeRect.Contains(_currentCell.ToPoint()) )
	{
		int radius = gameInfo.getConstInt("BoostBigBombRadius", 2);
		int directions = gameInfo.getConstInt("BoostBigBombDirections", 8);

		for (int i = -radius; i <= radius; i++)
		{
			for (int j = radius; j >= -radius; j--)
			{
				Game::FieldAddress fa = _currentCell + Game::FieldAddress(i, j);
				Game::Square *sq = GameSettings::gamefield[fa];			
				if(  (directions==8 || Game::CheckContainInRadius((float)radius, _currentCell.ToPoint(), fa.ToPoint(), sq)) 
					&& (Game::activeRect.Contains(fa.ToPoint())))
				{
					_sequenceCells.push_back(fa);
				}
			}
		}
		//дополняем бонусы
		AddBonusAffected();
	}
}

void BoostBigBomb::DoBoostAction()                   //выполняет действия буста 
{
	ClearAfectedCells();

}

//////////////////////////////// Буст - удаление фишек одного цвета ////////////////////
void BoostClearColor::FillAllowClick() //заполнить список разрешенных для клика ячеек
{
	_allowClick.clear();
	for(int x = 0; x < Game::activeRect.Width();x++)
	{
		for(int y = 0; y < Game::activeRect.Height();y++)
		{
			Game::FieldAddress fa = Game::FieldAddress(Game::activeRect.x + x, Game::activeRect.y + y);
			Game::Square *sq = GameSettings::gamefield[fa];
			Game::ChipColor chip = sq->GetChip();
			
			if (Game::isVisible(sq) && chip.IsSimpleChip()	&& !sq->IsIce())
			{
				_allowClick.push_back(fa);
			}
		}
	}
};

void BoostClearColor::UpdateAffectedCells()  //обновляет список убиваемых фишек
{
	_sequenceCells.clear();
	_affectedCells.clear();

	if (_currentCell != BoostList::EmptyAddres && Game::activeRect.Contains(_currentCell.ToPoint()) )
	{
		Game::Square *sq = GameSettings::gamefield[_currentCell];
		//берем цвет
		int color = sq->GetChip().GetColor();
		//ищем все такие же на экране
		for(int x = 0; x < Game::activeRect.Width();x++)
		{
			for(int y = 0; y < Game::activeRect.Height();y++)
			{
				Game::FieldAddress fa = Game::FieldAddress(Game::activeRect.x + x, Game::activeRect.y + y);
				Game::Square *sq = GameSettings::gamefield[fa];
				Game::ChipColor chip = sq->GetChip();
			
				if (Game::isVisible(sq) && chip.IsSimpleChip()	&& !sq->IsIce() && chip.GetColor() == color)  
				{
					_sequenceCells.push_back(fa);
				}
			}
		}
		//дополняем бонусы
		AddBonusAffected();
	}
};                   

void BoostClearColor::DoBoostAction() //выполняет действия буста
{
	Game::Square *sq = GameSettings::gamefield[_currentCell];
	int color = sq->GetChip().GetColor();

	//Game::GameLightningController *controller = new Game::GameLightningController(_currentCell.ToPoint(), color, 0.0f);
	//Game::AddController(controller);

	// переставим выбранную клетку вперёд
	for (auto fa : _sequenceCells) {
		if (fa == _currentCell) {
			Game::FieldAddress tmp = _sequenceCells[0];
			_affectedCells[0] = fa;
			fa = tmp;
			break;
		}
	}
	Game::GameSplashLightningController *controller = new Game::GameSplashLightningController(1.8f, _sequenceCells, 0.2f);
	Game::AddController(controller);
};

void BoostClearColor::AcceptMessage(const Message &message)
{
	if (message.is("UpdateAffectedCells")) {
		CountFromCurrentCell();
	} else if (message.is("SelectChipsFromRandomColor")) {
		SelectRandomCurrentCell();
		CountFromCurrentCell();
	} else if (message.is("GetAffectedChips")) {
		Message msg = Message("TutorialCells");
		int count = (int)_affectedCells.size();
		msg.getVariables().setInt("count", count);
		for (int i = 0; i < count; i++) {
			msg.getVariables().setInt("x"+utils::lexical_cast(i+1), _affectedCells[i].GetCol());
			msg.getVariables().setInt("y"+utils::lexical_cast(i+1), _affectedCells[i].GetRow());
		}
		Core::LuaCallVoidFunction("fillTutorialCells", msg);
	}
};

/////////////////////////////////// Буст Любая цепочка ////////////////////////////////////////
void BoostFreeSequence::UseBoost() 
{
	ResetSeq();

	//инициализируем список кого можно выделять
	FillAllowSelect();

	//контроллер клика
	_squareClick = new SquareClickController(this);
	Game::AddController(_squareClick);
}

void BoostFreeSequence::Cancel()
{
	ResetSeq();
	GameField::Get()->ResizeChipSeq(0, false);

	//убираем контроллеры
	Game::KillControllers(_squareClick -> getName());
}

void BoostFreeSequence::FillAllowSelect()
{
	_allowSelect.clear();
	for(int x = 0; x < Game::activeRect.Width();x++)
	{
		for(int y = 0; y < Game::activeRect.Height();y++)
		{
			Game::FieldAddress fa = Game::FieldAddress(Game::activeRect.x + x, Game::activeRect.y + y);
			Game::Square *sq = GameSettings::gamefield[fa];
			Game::ChipColor chip = sq->GetChip();
			
			if (Game::isVisible(sq) && chip.IsSimpleChip()	&& !sq->IsIce())
			{
				_allowSelect.push_back(fa);
			}
		}
	}
}

void BoostFreeSequence::Run()
{
	Assert(_sequenceStarted);

	Tutorial::luaTutorial.AcceptMessage( Message("OnRunBoost") );

	//убираем контроллеры
	Game::KillControllers(_squareClick -> getName());

	//завершаем
	FinalizeInterfaceBoost();

	// проверим, если все фишки одного цвета вернём 1 буст
	bool same = true;
	Game::Square *sq = GameSettings::gamefield[_sequenceCells[0]];
	int color = sq->GetChip().GetColor();
	for (auto cell: _sequenceCells) {
		Game::Square *sq = GameSettings::gamefield[cell];
		if (sq->GetChip().GetColor() != color) {
			same = false;
			break;
		}
	}
	if (same) {
		gameInfo.setLocalInt("HaveBoostFreeSequence", gameInfo.getLocalInt("HaveBoostFreeSequence", 0) + 1);
	}

	GameField::Get()->DestroySequence();
	GameField::Get()->ClearChipSeq(true);
}

void BoostFreeSequence::MouseDown(Game::FieldAddress pos)
{
	//пока поле не успокоится, не реагируем
	if (!GameField::Get()->IsStandby() && pos != BoostList::EmptyAddres)
	{
		return;
	}

	//не реагируем если туториал ждёт запуска буста
	if (Tutorial::luaTutorial.GetWait() == 9) {
		return;
	}

	_allowSelecting = true;

	if (!_sequenceStarted) {
		AddToSeq(pos);
	} else if ( !_sequenceCells.empty() && pos == _sequenceCells.front() ) {
		ResetSeq();
		GameField::Get()->ResizeChipSeq(0, false);
		_allowSelecting = false;
	}
}

void BoostFreeSequence::MouseUp(Game::FieldAddress pos)
{
	_allowSelecting = false;
	if (_sequenceStarted && (int)_sequenceCells.size() == 5) {
		Tutorial::luaTutorial.AcceptMessage( Message("OnFreeSeq") );
	}
}

void BoostFreeSequence::MouseMove(const IPoint &mouse_pos)
{
	if( GameSettings::FIELD_SCREEN_CONST.Contains(mouse_pos) && _sequenceStarted && _allowSelecting) {

		Game::FieldAddress pos = GameSettings::GetMouseAddress(mouse_pos - Game::ChipColor::CHIP_TAP_OFFSET);

		//пока поле не успокоится, не реагируем
		if (!GameField::Get()->IsStandby() && pos != BoostList::EmptyAddres) {
			return;
		}

		IPoint field_mp = GameSettings::ToFieldPos(mouse_pos);
		IPoint chipPos = pos.Get() * GameSettings::SQUARE_SIDE + IPoint(GameSettings::SQUARE_SIDE/2, GameSettings::SQUARE_SIDE/2);
		//проверяется область попадания в клетку
		bool hit_true = abs(chipPos.x - field_mp.x + 0.f) + abs(chipPos.y - field_mp.y + 0.f) <  (0.66f * GameSettings::SQUARE_SIDEF);


		if (hit_true) {
			Game::FieldAddress lastInSeq = _sequenceCells.back();

			if (pos != lastInSeq) {
				// возможно мы вернулись и нужно уменьшить цепочку
				if ((int)_sequenceCells.size() > 1) {
					Game::FieldAddress secondLastInSeq = *(_sequenceCells.rbegin() + 1);
					if (secondLastInSeq == pos) {
						_sequenceCells.pop_back();
						GameField::Get()->ResizeChipSeq(_sequenceCells.size(), false);
						SetCanApply(0);
					} else {
						AddToSeq(pos);
					}
				} else {
					AddToSeq(pos);
				}
			}
		}

	}
}

void BoostFreeSequence::AddToSeq(Game::FieldAddress destination)
{
	//проверяем разрешена ли клетка
	AddressVector::iterator find_allow = std::find(_allowSelect.begin(), _allowSelect.end(), destination);
	if (find_allow == _allowSelect.end()) //кликнули где то не там
	{
		return;
	}

	//проверим нет ли этой клетки уже в цепочке
	AddressVector::iterator find_selected = std::find(_sequenceCells.begin(), _sequenceCells.end(), destination);
	if (find_selected != _sequenceCells.end())
	{
		return;
	}

	if (_sequenceStarted) {
		//проверим чтобы клетка была соседней с последней в цепочке
		Game::FieldAddress lastInSeq = _sequenceCells.back();
		AddressVector posNeighbors;
		destination.FillDirections8(posNeighbors);
		AddressVector::iterator find_neighbor = std::find(posNeighbors.begin(), posNeighbors.end(), lastInSeq);
		if (find_neighbor == posNeighbors.end()) {
			return;
		}
		if ((int)_sequenceCells.size() > 4) return;
	}

	Game::Square *sq = GameSettings::gamefield[destination];

	if (sq && Game::isVisible(sq)) {
		if (gameInfo.currentBoostTutorial.name == getName()) {
			// если сейчас туториал на данный буст разрешаем выделять только на подсвеченные
			if (!sq->GetChip().IsChainHighlighted()) return;
		}

		_sequenceCells.push_back(destination);
		_sequenceStarted = true;
		
		if (_sequenceCells.size() > 1) {
			GameField::Get()->SequenceStart(sq ,10);
			Match3::Unpause();
			sq->GetChip().Select(_sequenceCells.size(), sq->address.ToPoint(), true, _sequenceCells[_sequenceCells.size()-2].ToPoint());
		} else {
			GameField::Get()->SequenceStart(sq ,10);
			Match3::Unpause();
		}

		if ((int)_sequenceCells.size() >= 5) SetCanApply(1);
	}
}

void BoostFreeSequence::ResetSeq()
{
	_sequenceStarted = false;
	_sequenceCells.clear();
	SetCanApply(0);
}


ChipsRecolorController::ChipsRecolorController(AddressVector cells, std::vector<int> colors, float pause)
	: GameFieldController("ChipsRecolorController", 1.f, GameField::Get())
	, _state(ChipsRecolorController::PAUSE)
	, _pause(pause)
	, _timer(0.f)
	, _colors(colors)
	, _cellsToRecolor(cells)
	, _effectPause(0.3f)
	, _paintsCount(0)
{
	Init();
}

void ChipsRecolorController::Init()
{
	// сортировка для запуска волной
	AddressVector tmpVector = _cellsToRecolor;
	_cellsToRecolor.clear();
	int tmpSize = tmpVector.size();
	int diagonalsCount = GameSettings::gamefield.Width();
	float delay = 0.f;
	float deltaDelay = 0.1f;
	for(int i = 0; i < diagonalsCount; i++) {
		int diagonalChipsCount = math::min(i+1, GameSettings::gamefield.Height());
		bool wasChipInDiag = false;
		for (int j = 0; j < diagonalChipsCount; j++) {
			IPoint checkChip = IPoint(i-j, j);
			//проверим нужно ли красить эту фишку
			for (int k = 0; k < tmpSize; k++) {
				if (tmpVector[k].GetCol() == checkChip.x && tmpVector[k].GetRow() == checkChip.y) {
					ChipRecolorInfo info;
					info.delay = delay;
					info.recolored = false;
					info.effectStarted = false;
					info.toColor = _colors[math::random(0, (int)_colors.size()-1)];
					_chipsRecolorInfo.push_back(info);
					_cellsToRecolor.push_back(tmpVector[k]);
					wasChipInDiag = true;
					tmpVector.erase(tmpVector.begin()+k);
					tmpSize--;
					break;
				}
			}
		}
		if (wasChipInDiag) delay = delay + deltaDelay;
	}
	
	/*
	float delay = 0.f;
	float deltaDelay = 1.0f / _cellsToRecolor.size();
	for (auto fa: _cellsToRecolor) {
		ChipRecolorInfo info;
		info.delay = delay;
		info.recolored = false;
		info.effectStarted = false;
		_chipsRecolorInfo.push_back(info);
		delay = delay + deltaDelay;
	}
	*/
}

void ChipsRecolorController::Update(float dt)
{
	if (_state == PAUSE) {
		_timer += dt;
		if (_timer > _pause) {
			_state = RECOLORING;
			_timer = 0.f;
		}
	} else if (_state == RECOLORING) {
		_timer += dt;
		for (int i = 0; i < (int)_chipsRecolorInfo.size(); i++ ) {
			if (!_chipsRecolorInfo[i].effectStarted && _chipsRecolorInfo[i].delay < _timer)
			{
				StartEffect(_cellsToRecolor[i], i);
				_chipsRecolorInfo[i].effectStarted = true;
			}
			if (!_chipsRecolorInfo[i].recolored && _chipsRecolorInfo[i].delay + _effectPause < _timer)
			{
				ChangeColor(_cellsToRecolor[i], i);
				_chipsRecolorInfo[i].recolored = true;
				_paintsCount++;
			}
			if (_paintsCount >= (int)_chipsRecolorInfo.size()) {
				_state = FINISHED;
			}
		}
	}
}

void ChipsRecolorController::StartEffect(Game::FieldAddress chip, int indx)
{
	//запускаем эффект
	Game::Square *sq = GameSettings::gamefield[chip];
	IPoint pos = Game::GetCenterPosition(sq->address);
	if (Game::visibleRect.Contains(chip.ToPoint())) {
		ParticleEffectPtr eff = Game::AddEffect(GameField::Get()->_effCont, "Recolor"
			+ utils::lexical_cast(_chipsRecolorInfo[indx].toColor));
		eff->posX = float(pos.x);
		eff->posY = float(pos.y);
		eff->Reset();
	}
}

void ChipsRecolorController::ChangeColor(Game::FieldAddress chip, int indx)
{
	// меняем цвет
	Game::Square *sq = GameSettings::gamefield[chip];
	
	//если на фишке был бонус его нужно сохранить и снова поставить
	//так как SetColor делает полный Reset фишки
	if (sq->GetChip().HasHang()) {
		Game::Hang hang = sq->GetChip().GetHang();
		sq->GetChip().SetColor(_chipsRecolorInfo[indx].toColor);
		sq->GetChip().SetHang(hang);
	} else {
		sq->GetChip().SetColor(_chipsRecolorInfo[indx].toColor);
	}
}

bool ChipsRecolorController::isFinish()
{
	return (_state == FINISHED);
}

// контроллер запускающий падение фишек
BoostRunFallController::BoostRunFallController(AddressVector cells, float pause)
	: GameFieldController("BoostRunFallController", 1.f, GameField::Get())
	, _state(PAUSE)
	, _pause(pause)
	, _timer(0.f)
	, _cells(cells)
{
}

void BoostRunFallController::Update(float dt)
{
	if (_state == PAUSE) {
		_timer += dt;
		if (_timer >= _pause) {
			for (auto currentCell : _cells) {
				Match3::RunFallColumn(currentCell.GetCol());
			}
			_state = FINISHED;
		}
	}
}

bool BoostRunFallController::isFinish()
{
	return (_state == FINISHED);
}
