#include "StdAfx.h"
#include "MovingMonster.h"
#include "GameField.h"
#include "EditorUtils.h"
#include "Match3Gadgets.h"
#include "Energy.h"
#include "GameFieldControllers.h"
#include "Tutorial.h"
#include "MyApplication.h"
#include "EnergyReceivers.h"

static const int monsterFieldBlock = -100;
static const int monsterFieldEmpty = -1;
static const int monsterFieldCanJump = -2;


Thief::Thief(IPoint index)
	: _index(index)
	, _currentAnimation(0)
	, _bornTime(0.f)
	, _awake(false)
	, _pulse(false)
	, _isKilled(false)
	, _canUpdate(false)
	, _timer(0.f)
	, _mirror(false)
	, _localOffset(0.f, 0.f)
	, _moves(0)
	, _turned(false)
	, _countNear(0)
{
	AddAnimation("thief_stay");
}

void Thief::RunAnimation(std::string name, float pause, FPoint offset)
{
	_anim_name = name;
	Assert(Game::ANIM_RESOURCES.find(_anim_name) != Game::ANIM_RESOURCES.end());

	Game::AnimRes &res = Game::ANIM_RESOURCES[_anim_name];

	_currentAnimation = 0;
	_currentAnimation = new FlashAnimation(res.lib, res.source);
	_currentAnimation->GetMovieClip()->setPlaybackOperation(GameLua::getPlayOnceOperation());
	_timer = -pause;
	if(!res.can_turn)
{
		Assert(res.mirror == _turned);
		_mirror = res.mirror;
	}else{
		_mirror = (res.mirror != _turned);
	}
	FPoint in_offset0 = res.inner_offset;
	FPoint in_offset1 = res.inner_offset_after;
	if(_mirror)
	{
		in_offset0.x = 80.f - in_offset0.x;
		in_offset1.x = 80.f - in_offset1.x;
	}
	_offset0 = offset/GameSettings::SQUARE_SCALE + in_offset0;
	_offset1 = offset/GameSettings::SQUARE_SCALE + in_offset1;
	_currentAnimation->SetHotSpot(-_offset0.x, -_offset0.y);
}

void Thief::AddAnimation(std::string name_, bool immediate, float pause_, FPoint offset_start_)
{
	if(immediate)
	{
		_queueAnim.clear();
		_currentAnimation = 0;
	}
	AnimInfo info;
	info.name = name_;
	info.offset = offset_start_;
	info.pause = pause_;
	_queueAnim.push_back(info);
	Update(0.f);
}

Thief::~Thief()
{
}

void Thief::Update(float dt)
{
	if(!_canUpdate)
	{
		return;
	}
	if(_currentAnimation && !_currentAnimation->IsLastFrame())
	{
		if(_timer < 0.f) {
			//Пауза для проигрыша текущей установленной анимации
			_timer += dt;
		} else {
			_timer = math::clamp(0.f, 1.f, _timer + dt/0.5f);
			FPoint offset = math::lerp(_offset0, _offset1, math::clamp(0.f, 1.f, (_timer- 0.2f)/0.6f));
			_currentAnimation->SetHotSpot(-offset.x, -offset.y);
			_currentAnimation->Update(dt);
		}
	}else{
		if(_anim_name == "thief_turnback") {
			_turned = false; //Вор полностью повернулся
		} else if(_anim_name == "thief_turn") {
			_turned = true; //Вор полностью повернулся
		}
		if(_anim_name.substr(0, 10) == "thief_jump")
		{
			_countNear = 0;
		}
		if(_anim_name == "thief_hide")
		{
			_timer = 0.f;
			_isKilled = true;
			_currentAnimation = NULL;
			_queueAnim.clear();
		}
		else if(!_queueAnim.empty())
		{
			AnimInfo &info(_queueAnim.front());
			Game::AnimRes &res = Game::ANIM_RESOURCES[info.name];
			if(!res.can_turn && res.mirror != _turned)
			{
				//Нужно проиграть анимацию поворотаf 
				if(_turned){
					RunAnimation("thief_turnback", 0.f, info.offset );
				}else{
					RunAnimation("thief_turn", 0.f, info.offset);
				}
			}
			else
			{
				RunAnimation(info.name, info.pause, info.offset);
				_queueAnim.pop_front();
			}
		}
		else
		{
			RunAnimation("thief_stay", math::random(0.f, 5.f));
		}
	}
}

void Thief::Draw(FPoint pos)
{
	if(_currentAnimation)
	{
		Render::PushMatrix pushMatrix;
		Render::device.MatrixTranslate(pos);
		Game::MatrixSquareScale();
		_currentAnimation->Draw(0.f, 0.f, _mirror);
	}
}

IPoint Thief::Cell() const
{
	return _index;
}

int Thief::GetMoves() const
{
	return _moves;
}

void Thief::ChangeMoves(int diff)
{
	_moves += diff;
}

void Thief::SetPulse(IPoint dir)
{
	_queueAnim.clear();
	int dir_id = 0;
	if(dir.x == 1 && dir.y == 0)
	{
		dir_id = 0;
	}else if(dir.x == 0 && dir.y == 1)
	{
		dir_id = 1;
	}else if(dir.x == -1 && dir.y == 0)
	{
		dir_id = 2;
	}else if(dir.x == 0 && dir.y == -1)
	{
		dir_id = 3;
	}else{
		Assert(false);
	}
	if(_countNear == 0)
	{
		AddAnimation("thief_near_" + utils::lexical_cast(dir_id), true, math::random(0.2f, 0.4f));
	}else{
		AddAnimation("thief_near_" + utils::lexical_cast(dir_id), false, math::random(0.f, 5.f));
	}
	_countNear++;
}

bool Thief::IsStand() const
{
	return _anim_name == "thief_stay" && _queueAnim.empty();
}

void Thief::SmartClearQueue()
{
	_queueAnim.clear();
}

void Thief::SetIndex(IPoint new_index)
{
	_index = new_index;
}

void Thief::Kill()
{
	SmartClearQueue();
	AddAnimation("thief_hide", true);
}

bool Thief::IsKilled() const
{
	return _isKilled;
}

bool Thief::IsTurned() const
{
	return _turned;
}

std::string Thief::GetAnimationName() const
{
	return _anim_name;
}

void Thief::Appear()
{
	_queueAnim.clear();
	RunAnimation("thief_appear");
	_canUpdate = true;
	if(EditorUtils::editor && _currentAnimation)
	{
		_currentAnimation->Update(4.f);
	}
}

bool Thief::IsAppear() const
{
	return _canUpdate;
}


///////////////////// список монстров //////////////////////////
MovingMonsters::MovingMonsters(void)
	:_haveMonsters(false)
	, _needMoveMonsters(0)
	, _canWakeMonsters(false)
{
}

MovingMonsters::~MovingMonsters(void)
{
}


//распространяет энергию куда может
void MovingMonsters::GrowEnergy()
{
	//перебираем клетки
	for (int col=_fullFieldLeft; col<=_fullFieldRight; col++)
		for (int row=_fullFieldBottom; row<=_fullFieldTop; row++)
		{
			Game::FieldAddress fa(col, row);

			int energy = _energy[fa];
			// если есть энегрия и тут еще не были
			if (energy == 1)
			{
				//распространяем энергию
				std::queue <Game::FieldAddress> queue; //очередь не отработанных клеток
				queue.push(fa); //добавляем
				_energy[fa] = 2;
		
				//итеративный алгоритм поиска в ширину
				while (!queue.empty())
				{
					Game::FieldAddress currentCell=queue.front(); //клетка из которой ходим
					queue.pop(); //убираем ее
					AddressVector directions; //соседи
					currentCell.FillDirections8(directions);  //заполняем список
			
					//перебираем соседей
					for (auto tryFa: directions)
//					for (std::list<Game::FieldAddress>::iterator it=directions.begin(); it != directions.end(); ++it)
					{
						//берем соседа
//						Game::FieldAddress tryFa = *it;
						int tryEnergy = _energy[tryFa];
				
						//если может притечь энегрия
						if (tryEnergy == 0)
						{
							queue.push(tryFa); //добавляем
							_energy[tryFa] = 2;
						}
					}
				}
			}
		}
}

void MovingMonsters::LoadEnergy()
{
	//заполняем блоками
	_energy.Init(GameSettings::FIELD_MAX_WIDTH+2, GameSettings::FIELD_MAX_HEIGHT+2, monsterFieldBlock, 1, 1); 
	//перебираем клетки активной области
	for (int row = _fullFieldBottom; row <= _fullFieldTop; row++)
	{
		for (int col = _fullFieldLeft; col <= _fullFieldRight; col++)
		{
			Game::FieldAddress fa(col, row);
			Game::Square *sq = GameSettings::gamefield[fa];
			Game::ChipColor& chip=sq->GetChip ();

			if (Energy::field.FullOfEnergy(fa)) //уже есть энергия
			{
				_energy[fa] = 1;
			} 
			else if (sq->CanEnergy()) //может притечь
			{
				_energy[fa] = 0;
			}
		}
	}
}

//загрузка текущего состояния игры. также прибавляет монстрикам запас ходов
void MovingMonsters::LoadGameState()
{
	//чистим массив
	_movesToRecievers.Init(GameSettings::FIELD_MAX_WIDTH+2, GameSettings::FIELD_MAX_HEIGHT+2, monsterFieldBlock, 1, 1); 
	
	_monstersPositions.clear();
	_recieversPositions.clear();

	_haveRecieversOnScreen = false;
	//перебираем клетки активной области
	for (int row = _fieldBottom; row <= _fieldTop; row++)
	{
		for (int col = _fieldLeft; col <= _fieldRight; col++)
		{
			Game::FieldAddress fa(col, row);
			Game::Square *sq = GameSettings::gamefield[fa];
			Game::ChipColor& chip = sq->GetChip();
			Thief::HardPtr thief = chip.GetThief();
			if(thief) 
			{
				//там монстр. 
				bool justWake = false;
				if (Game::activeRect.Contains(fa.ToPoint()) && _canWakeMonsters) //если что, будим
				{
					if (!thief->IsAppear())
					{
						thief->Appear();
						justWake = true;
					}
				}
				//если не спит, добавляем. спящие будут рассматриваться как блоки
				if (thief->IsAppear())
				{
					_monstersPositions.push_back(thief);
					_movesToRecievers[fa] = monsterFieldEmpty; //ходить "по монстру" можно

					if (justWake) { //только проснувшимся даем 1 ход
						thief->ChangeMoves(1);
					} else {
						thief->ChangeMoves(_needMoveMonsters);
					}
				}
			} 
			else if (Gadgets::receivers.IsReceiverCell(fa)) //приемник
			{
				_movesToRecievers[fa] = 0; //расстояние 0
				_recieversPositions.push_back(fa);
				_haveRecieversOnScreen = true;
			} 
			else if ( sq->IsGoodForMovingMonster() ) //фишка (не во льду)
			{
				_movesToRecievers[fa] = monsterFieldEmpty; //можно ходить
			} 
			else if ( sq->IsHardStand() || sq->IsIce() || sq->GetChip().GetLock()  ) //что то устойчивое
			{
				//прыжки пока отменили
				//_movesToRecievers[fa] = monsterFieldCanJump; //можно прыгать
			}

			//////все остальное считается барьером
		}
	}

	_needMoveMonsters = 0; //сбрасываем флажок
}


//ищем ход по массиву
void MovingMonsters::FindMove(Game::FieldAddress fa, Game::FieldAddress dir)
{
	fa = fa + dir; //двигаемся
	while (_movesToRecievers[fa] == monsterFieldCanJump)
	{
		fa = fa + dir; //прыгаем пока можем
	}
	if (_movesToRecievers[fa] >=0 && _movesToRecievers[fa] <= _minDistance 
		&& !GameSettings::gamefield[fa]->GetChip().IsThief()) //один монстрик не может задавить другого и пока что ждет
	{  //если по этому пути имеет смысл ходить
		if (_movesToRecievers[fa] < _minDistance) //если он лучше текущего 
		{
			_minDistance = _movesToRecievers[fa]; //запоминаем новый лучший 
			_possibleMoves.clear(); //чистим список возможных ходов
		}
		_possibleMoves.push_back(fa); //добавляем возможный ход
	}

}

int MovingMonsters::MoveMonster(Game::FieldAddress monsterFa, Game::FieldAddress chipFa)
{
	//меняемся
	Game::Square *from = GameSettings::gamefield[monsterFa];
	Game::Square *to = GameSettings::gamefield[chipFa];

	Assert(from->GetChip().IsThief());	
	Game::Square::SwapChips(from, to);
	Assert(to->GetChip().IsThief());


	//уменьшаем запас хода
	to->GetChip().GetThief()->ChangeMoves(-1);

	//запускаем анимацию передвижения
	IPoint dir(to->address.ToPoint() - from->address.ToPoint());
	Thief::HardPtr thief = to->GetChip().GetThief();
	thief->SmartClearQueue();
	float pause_chip = 0.7f;
	float pause = 0.5f;
	if(dir.x == 1 && dir.y == 0)
	{
		if(!thief->IsTurned())
		{
			pause_chip += 0.2f;
			pause = 0.3f;
		}
		thief->AddAnimation("thief_jump_0", true, pause, FPoint(-GameSettings::SQUARE_SIDEF, 0.f));
	}else if(dir.x == 0 && dir.y == 1)
	{
		thief->AddAnimation("thief_jump_1", true, pause, FPoint(0.f, -GameSettings::SQUARE_SIDEF));
	}else if(dir.x == -1 && dir.y == 0)
	{
		if(thief->IsTurned())
		{
			pause_chip += 0.2f;
			pause = 0.3f;
		}
		thief->AddAnimation("thief_jump_2", true, pause, FPoint(GameSettings::SQUARE_SIDEF, 0.f));
	}else if(dir.x == 0 && dir.y == -1)
	{
		thief->AddAnimation("thief_jump_3", true, pause, FPoint(0.f, GameSettings::SQUARE_SIDEF));
	}else{
		Assert(false);
	}

	//ставим флажок
	_somethingMoved = true;

	to->GetChip().ClearOffset();
	from->GetChip().ClearOffset();

	FPoint empty(0,0);

	// !!!сделать анимацию смены мест
	FPoint toPos =  from->GetCellPos() - to->GetCellPos();  
	FPoint fromPos =to->GetCellPos() - from->GetCellPos();

	//if (toPos != empty) 
	//{
	//	to->GetChip().SetPos(toPos);
	//	to->GetChip().RunMove(0, 2.0f * GameSettings::SQUARE_SIDEF, 0);
	//}

	if (fromPos != empty) //Двигаем фишку
	{
		from->GetChip().SetPos(fromPos);
		from->GetChip().RunMove(pause_chip, 5.0f * GameSettings::SQUARE_SIDEF, 0);
	}

	MM::manager.PlaySample("MovingMonsterMove");

	return to->GetChip().GetThief()->GetMoves();
}

//смотрим не можем ли сюда сходить
void MovingMonsters::UpdateDistance(Game::FieldAddress fa, Game::FieldAddress dir)
{
	fa = fa + dir; //двигаемся
	while (_movesToRecievers[fa] == monsterFieldCanJump)
	{
		fa = fa + dir; //прыгаем пока можем
	}
	if (_movesToRecievers[fa] == monsterFieldEmpty) //если там куда припрыгали пусто то заполняем
	{
		_movesToRecievers[fa] = _currentDistance + 1;
		_somethingChanged = true;
	}
}

void MovingMonsters::FillDistances()
{
	_currentDistance = 0;
	_somethingChanged = true;
	while (_somethingChanged) //нашлись ли еще ходы
	{
		_somethingChanged = false; //сбрасываем флажок
		//перебираем клетки
		for (int row = _fieldBottom; row <= _fieldTop; row++)
		{
			for (int col = _fieldLeft; col <= _fieldRight; col++)
			{
				Game::FieldAddress fa(col, row);
				if (_movesToRecievers[fa] == _currentDistance) //из этой надо двигаться дальше
				{
					//специально проверяем приемник с заказом
					if (_currentDistance == 0) //расстояние 0 значит приемник прямо тут
					{
						EnergyReceiver *reciever = Gadgets::receivers.GetReceiverOnSquare(fa);
						if (reciever && reciever->IsOrdered())
						{   //если есть заказ то придти к приемнику один фиг нельзя
							continue;
						}
					}

					UpdateDistance(fa, fa.UP);
					UpdateDistance(fa, fa.LEFT);
					UpdateDistance(fa, fa.RIGHT);
					UpdateDistance(fa, fa.DOWN);
				}
			}
		}
		++_currentDistance;
	}
}

int Distance(Game::FieldAddress fa1, Game::FieldAddress fa2)
{
	return math::abs(fa1.GetCol() - fa2.GetCol()) + math::abs(fa1.GetRow() - fa2.GetRow());
}

//проверяем могут ли заполниться энергией в этом ходу
bool MovingMonsters::checkThisReceiverCanFill(AddressVector _receivers) const
{
	for (auto fa : _receivers)
	{
		if (_energy[fa] <= 0)
		{
			return false;
		}
	}
	return true;
}

void MovingMonsters::MonsterEat(Game::FieldAddress monsterFA, Game::FieldAddress recieverFA) //монстрик скушал получатель
{
	FPoint empty(0,0);

	// пока анимация съедения такая же как для обычного хода
	Game::Square *from = GameSettings::gamefield[monsterFA];
	Game::Square *to = GameSettings::gamefield[recieverFA];

	Assert(from->GetChip().IsThief());

	int dir_id = 0;
	IPoint dir = recieverFA.ToPoint() - monsterFA.ToPoint();
	if(dir.x == 1 && dir.y == 0)
	{
		dir_id = 0;
	}else if(dir.x == 0 && dir.y == 1)
	{
		dir_id = 1;
	}else if(dir.x == -1 && dir.y == 0)
	{
		dir_id = 2;
	}else if(dir.x == 0 && dir.y == -1)
	{
		dir_id = 3;
	}else{
		Assert(false);
	}
	from->GetChip().GetThief()->AddAnimation("thief_steal_" + utils::lexical_cast(dir_id), true, 1.f); //Анимация кражи приемника
	Gadgets::receivers.GetReceiver(recieverFA.ToPoint())->HideByThief(1.1f);

	Core::LuaCallVoidFunction("showTutorialHightlightForThiefEat", monsterFA.ToPoint(), recieverFA.ToPoint());
	MM::manager.PlaySample("MovingMonsterEat");

	GameField::Get()->_endLevel.SetLoseLifeReason(LevelEnd::MONSTER_EAT_RECEIVER); 
}

void MovingMonsters::MoveRound1() //поиск ходов ведущих прямо к приемнику
{
	//считаем расстояния до всех приемников
	FillDistances();
	//сортируем монстриков по расстоянию. чтобы сперва ходили те кто ближе и освобождали место для других. стандартный sort технически слжно использовать
	if (_monstersPositions.size() > 1)
	{
		for (size_t i=0; i < _monstersPositions.size() - 2; ++i)
		{
			for (size_t j=i+1; j<_monstersPositions.size() - 1; ++j)
			{
				if (_movesToRecievers[_monstersPositions[i]->Cell()] > _movesToRecievers[_monstersPositions[j]->Cell()])
				{
					std::swap(_monstersPositions[i], _monstersPositions[j]);
				}
			}
		}
	}

	//двигался ли хоть кто то из монстриков - возможно он освободил место для другого
	_somethingMoved = true;

	while (_somethingMoved)
	{
		_somethingMoved = false;
		//расстояния готовы - начинаем ход
		for (std::vector<Thief::HardPtr>::iterator it = _monstersPositions.begin(); it != _monstersPositions.end();)
		{
			//для каждого монстра
			if((*it)->IsKilled() || (*it)->GetAnimationName() == "thief_hide")
			{
				++it;
				continue;
			}
			Game::FieldAddress monsterFa((*it)->Cell());
			_minDistance = _movesToRecievers[monsterFa]; //чтобы не ухудщить текущее положение
			_possibleMoves.clear();
			//ищем лучшие пути
			FindMove(monsterFa, monsterFa.UP);
			FindMove(monsterFa, monsterFa.LEFT);
			FindMove(monsterFa, monsterFa.RIGHT);
			FindMove(monsterFa, monsterFa.DOWN);
			//если пути есть - идем по случайному
			if (!_possibleMoves.empty())
			{
				if (_minDistance == 0) //если дошли - проигрываем
				{
					if (checkThisReceiverCanFill(_possibleMoves)) //если эти приемники вот вот заполнится
					{
						it = _monstersPositions.erase(it); //ходить не будет, но и ходы не пропускает
					} else { //иначе поражение
						MonsterEat(monsterFa, _possibleMoves.front());
						_monstersPositions.clear();
						break;
					}
				}
				else
				{
					size_t count = _possibleMoves.size();
					size_t n = math::random(0u, count-1);
					//с кем меняемся
					Game::FieldAddress chipFa = _possibleMoves[n];
					int leftMoves = MoveMonster(monsterFa, chipFa);

					if (leftMoves > 0) //если ходы еще остались
					{
						_someMonsterStillCanMove = true;
					}

					//if (_minDistance == 1)
					//{
					//	//пират на расстоянии 
					//}

					it = _monstersPositions.erase(it); //удаляем монстрика потому что он уже ходил
				}

			} else {
				//монстрик в тупике - пропускает ходы
				int leftMoves = (*it)->GetMoves();
				(*it)->ChangeMoves(-leftMoves);
				++it;
			}

		}
	}
}

void MovingMonsters::MoveRound2() //поиск ходов чтобы придти как можно ближе к приемнику
{
	if (_monstersPositions.empty())
	{
		return;  //если все монстры двинулись - выходим
	}
	//теперь двигаем тех кто не может дойти до приемника
	//сперва меняем на блоки получателей и все рядом, чтобы не мешались
	for (int row = _fieldBottom; row <= _fieldTop; row++)
	{
		for (int col = _fieldLeft; col <= _fieldRight; col++)
		{
			Game::FieldAddress fa(col, row);
			if (_movesToRecievers[fa] >=0 )
			{
				_movesToRecievers[fa] = monsterFieldBlock;
			}
		}
	}

	//теперь остались только клетки изолированные от получателей
	//для каждого монстрика запустим поиск куда может дойти
	//и из этих клеток выберем ближайшую к потребителю
	_somethingMoved = true;
	while (_somethingMoved) //пока в теории могут двигаться
	{
		_somethingMoved = false;
		for (std::vector<Thief::HardPtr>::iterator it = _monstersPositions.begin(); it != _monstersPositions.end();)
		{
			//для каждого монстра	
			Game::FieldAddress monsterFa((*it)->Cell());

			//чистим массив от предыдущих попыток
			for (int row=_fieldBottom; row<=_fieldTop; row++)
				for (int col=_fieldLeft; col<=_fieldRight; col++)
				{
					Game::FieldAddress fa(col, row);
					if (_movesToRecievers[fa] >=0 )
					{
						_movesToRecievers[fa] = monsterFieldEmpty;
					}
				}


			_movesToRecievers[monsterFa] = 0;  //ставим исходную точку

			FillDistances(); //находим куда может пойти
			//находим на каком расстоянии сейчас, чтобы не ухудшить
			int bestDistance = 1000;
			for (std::list<Game::FieldAddress>::iterator it1 = _recieversPositions.begin(); it1 != _recieversPositions.end(); ++it1)
			{
				Game::FieldAddress recieverFa = *it1;
				int newDistance =  Distance(monsterFa, recieverFa);
				if (newDistance < bestDistance) 
				{
					bestDistance = newDistance;
				}
			}
			int bestMoves = 0;    //лучшее число ходов до ближней клетки (если 2 получателя в тупиках, пойдет к тому который ближе)
			std::vector<Game::FieldAddress> bestDestinations;
			bestDestinations.push_back(monsterFa);
			//Game::FieldAddress bestFa = monsterFa; //адрес ближайшей к получателю клетки, куда монстрик в итоге пойдет
			//перебираем найденные клетки и находим ближайшие к получателю
			for (int row = _fieldBottom; row <= _fieldTop; row++)
			{
				for (int col = _fieldLeft; col <= _fieldRight; col++)
				{
					Game::FieldAddress tryFa(col, row);
					Game::Square *sq = GameSettings::gamefield[tryFa];
					if(!Game::isVisible(sq) || !sq->GetChip().IsThief())
					{
						continue;
					}
					bool thereIsNoMonster = std::find(_monstersPositions.begin(), _monstersPositions.end(), sq->GetChip().GetThief()) == _monstersPositions.end();
					if (_movesToRecievers[tryFa] >0 && thereIsNoMonster) //может до сюда дойти  и там нет другого монстрика
					{
						//перебираем получателей
						for (std::list<Game::FieldAddress>::iterator it1 = _recieversPositions.begin(); it1 != _recieversPositions.end(); ++it1)
						{
							Game::FieldAddress recieverFa = *it1;
							int newDistance = Distance(tryFa, recieverFa);
							int newMoves = _movesToRecievers[tryFa];

							if ((newDistance < bestDistance) //если ближе до приемника
								|| (newDistance == bestDistance && newMoves <= bestMoves))  //или такое же расстояние но ближе идти
							{
								//это новая цель
								if (newDistance != bestDistance || newMoves != bestMoves)
								{
									bestDestinations.clear();
								}
								bestDistance = newDistance;
								bestMoves = newMoves;
								bestDestinations.push_back(tryFa);
							}
						}

					}
				}
			}
			
			if (bestMoves == 0) //если монстрику некуда ходить, значит он в тупике
			{
				//монстрик в тупике - пропускает ходы
				int leftMoves = (*it)->GetMoves();
				(*it)->ChangeMoves(-leftMoves);

				_movesToRecievers[monsterFa] = monsterFieldBlock; //ставим такм блок
				_monstersPositions.erase(it); //удаляем его
				_somethingMoved = true;  //и все по новой потому что надо пересчитать ходы
				break;
			}

			//теперь в bestDestinations список целей. выбираем случайную
			size_t count = bestDestinations.size();
			size_t n = math::random(0u, count-1);
			//с кем меняемся
			Game::FieldAddress bestFa = bestDestinations[n];

			if (bestFa != monsterFa) //если что то нашли то восстанавливаем путь и делаем 1-й шаг
			{
				Game::FieldAddress currentFa = bestFa;
				int currentDistance = _movesToRecievers[currentFa];
				while (currentDistance != 1)  
				{   //выбираем ячейку с числом на 1 меньше
					AddressVector directions; 
					currentFa.FillDirections4(directions); //куда можем пойти
					std::random_shuffle(directions.begin(), directions.end()); //чтобы ходил случайно

					bool found = false;
					for (auto tryFa: directions)
					{
						if (_movesToRecievers[tryFa] == currentDistance-1)	//идем в первую попавшуюся
						{
							currentFa = tryFa;
							found = true;
							break;
						}
					}

					if (!found) //если идти некуда, выходим, хотя не должно быть
					{
						break;
					}

					currentDistance = currentDistance -1;
				}
				//если нашли ячейку на расстоянии 1 хода и там нет монстрика
				if (currentDistance == 1 && !GameSettings::gamefield[currentFa]->GetChip().IsThief() )
				{
					int leftMoves = MoveMonster(monsterFa, currentFa);
					if (leftMoves > 0) //если ходы еще остались
					{
						_someMonsterStillCanMove = true;
					}
					it = _monstersPositions.erase(it);
					continue; //чтобы итератор не прибавился
				}
			}
			++it;
		}
	}
}

//движение монстров рандомно
void MovingMonsters::MoveRandom()
{
	for (std::vector<Thief::HardPtr>::iterator it = _monstersPositions.begin(); it != _monstersPositions.end();)
	{
		Game::FieldAddress monsterFA((*it)->Cell());
		AddressVector directions; 
		monsterFA.FillDirections4(directions); //куда можем пойти
		std::random_shuffle(directions.begin(), directions.end()); //чтобы ходил случайно

		Game::FieldAddress foundFA;

		bool found = false;
		for (auto tryFa: directions)
		{
			Game::Square *sq = GameSettings::gamefield[tryFa];
			if (sq->IsGoodForMovingMonster() && Game::activeRect.Contains(tryFa.ToPoint()))	//идем в первую попавшуюся
			{
				foundFA = tryFa;
				found = true;
				break;
			}
		}

		if (found)  //если нашли ход, делаем
		{
			int leftMoves = MoveMonster(monsterFA, foundFA);
			if (leftMoves > 0) //если ходы еще остались
			{
				_someMonsterStillCanMove = true;
			}
			it = _monstersPositions.erase(it);
		} else {   //если не нашли значит в тупике, сбрасываем ходы
			int leftMoves = (*it)->GetMoves();
			(*it)->ChangeMoves(-leftMoves);
			++it;
		}
	}
}

//движение монстров
void MovingMonsters::Move()
{
	_fullFieldLeft = GameField::Get()->_fieldLeft;         
	_fullFieldRight = GameField::Get()->_fieldRight;
	_fullFieldTop = GameField::Get()->_fieldTop;
	_fullFieldBottom = GameField::Get()->_fieldBottom;

	_fieldLeft = Game::activeRect.x;         //это размеры активной области
	_fieldRight = Game::activeRect.x + Game::activeRect.width - 1;
	_fieldTop = Game::activeRect.y + Game::activeRect.height - 1;
	_fieldBottom = Game::activeRect.y;

	//грузим и распространяем энергию
	LoadEnergy();
	GrowEnergy();

	_someMonsterStillCanMove = true;
	while (_someMonsterStillCanMove)
	{
		//грузим состояние игры
		LoadGameState();
		//проверяем что монстры вообще есть
		if (_monstersPositions.empty())
		{
			_movesToRecievers.Release();
			break;
		}
		_someMonsterStillCanMove = false;

		if (!_haveRecieversOnScreen)	//если приемников нету ходим рандомно
		{
			MoveRandom();
		} else {
			//////////////////////// этап 1//////////////////////////////////////////////
			MoveRound1();
			//////////////////////// этап 2//////////////////////////////////////////////
			MoveRound2();
		}
		//чистим массив
		_movesToRecievers.Release();
	}

	_energy.Release();

	GameField::Get()->_maybeMoveGadget.NeedUpdate(); //возможно, кончились ходы
}

//вызывается из главного Update если поле спокойно (StandBy). Флажок необходимости хода ставится в OnEndMove
void MovingMonsters::Update(float dt)
{
	if (!_haveMonsters)
	{
		return;
	}

	if (_needMoveMonsters > 0) //если надо ходить
	{
		Move(); //ходим
		Gadgets::movingMonstersSources.MakeMonsters(); //генерим новых
	}
}

//обновляет пульсацию монстриков рядом с приемником
void MovingMonsters::UpdatePulse(float dt)
{
	if (!_haveMonsters)
	{
		return;
	}

	_fieldLeft = Game::activeRect.x;         //это размеры активной области
	_fieldRight = Game::activeRect.x + Game::activeRect.width - 1;
	_fieldTop = Game::activeRect.y + Game::activeRect.height - 1;
	_fieldBottom = Game::activeRect.y;
	//_fieldLeft = GameField::Get()->_fieldLeft;         
	//_fieldRight = GameField::Get()->_fieldRight;
	//_fieldTop = GameField::Get()->_fieldTop;
	//_fieldBottom = GameField::Get()->_fieldBottom;

	for (int row =_fieldBottom; row <=_fieldTop; row++)
	{
		for (int col =_fieldLeft; col <=_fieldRight; col++)
		{
			Game::FieldAddress fa(col, row);
			Game::Square *sq = GameSettings::gamefield[fa];
			Game::ChipColor& chip=sq->GetChip ();
			
			if (chip.IsThief() && chip.GetThief()->IsStand()) 
			{
				AddressVector directions;
				fa.FillDirections4(directions);
				for (Game::FieldAddress faTest : directions)
				{
					if (Gadgets::receivers.IsReceiverCell(faTest))
					{
						chip.GetThief()->SetPulse(faTest.ToPoint() - fa.ToPoint());
						break;
					}
				}
			}
		}
	}
}

bool MovingMonsters::IsEatingReceiver() const
{
	for(const auto &thief : _monstersPositions)
	{
		const std::string &anim = thief->GetAnimationName();
		if( anim.substr(0,11) == "thief_steal" )
			return true;
	}
	return false;
}


////////////////////////////// MovingMonsterSources (источники монстриков) /////////////////////////////
namespace Gadgets
{

void MovingMonsterSources::LoadLevel(rapidxml::xml_node<> *root)
{
	rapidxml::xml_node<> *elem = root->first_node("MovingMonsterSources");
	if( elem ) {
		elem = elem->first_node("Source");
		while( elem )
		{
			Game::FieldAddress fa(elem);
			_sources.insert( std::make_pair(fa, 0) );
			elem = elem->next_sibling("Source");
		}
	}
}

void MovingMonsterSources::SaveLevel(Xml::TiXmlElement *root)
{
	Xml::TiXmlElement *srcElem = root->InsertEndChild(Xml::TiXmlElement("MovingMonsterSources"))->ToElement();
	for(Sources::iterator itr = _sources.begin(); itr != _sources.end(); ++itr)
	{
		Xml::TiXmlElement *elem = srcElem->InsertEndChild(Xml::TiXmlElement("Source"))->ToElement();
		itr->first.SaveToXml(elem);
	}
}

void MovingMonsterSources::DrawEdit()
{
	for(Sources::iterator itr = _sources.begin(); itr != _sources.end(); ++itr)
	{
		IRect draw(0, 0, GameSettings::SQUARE_SIDE, GameSettings::SQUARE_SIDE);
		IPoint pos = itr->first.ToPoint() * GameSettings::SQUARE_SIDE;
		Render::device.SetTexturing(false);
		Render::BeginColor( Color(128, 128, 128, 128) );
		Render::DrawRect( draw.MovedTo(pos) );
		Render::EndColor();
		Render::device.SetTexturing(true);

		Render::FreeType::BindFont("editor");
		Render::BeginColor(Color::RED);
		Render::PrintString( pos + GameSettings::CELL_HALF.Rounded(), "PIRATE", 1.0f, CenterAlign, CenterAlign, false);
		Render::EndColor();
	}
}
//лкм с контролом - установка исчтоника
bool MovingMonsterSources::Editor_LeftMouseDown(const IPoint &mouse_pos, Game::Square *sq)
{
	if( EditorUtils::activeEditBtn == EditorUtils::MovingMonsterEdit && Core::mainInput.IsControlKeyDown() )
	{
		Game::FieldAddress fa = GameSettings::GetMouseAddress(mouse_pos);
		_selected = _sources.find(fa);

		if(_selected == _sources.end())
		{
			_sources.insert( std::make_pair(fa, 0) );
		}

		return true;
	}
	return false;
}
//ркм - удаление исчтоника
bool MovingMonsterSources::Editor_RightMouseDown(const IPoint &mouse_pos, Game::Square *sq)
{
	if( EditorUtils::activeEditBtn == EditorUtils::MovingMonsterEdit)
	{
		Game::FieldAddress fa = GameSettings::GetMouseAddress(mouse_pos);
		_selected = _sources.find(fa);

		if(_selected != _sources.end())
		{
			_sources.erase(_selected);
			return true;
		}
	}
	return false;
}

void MovingMonsterSources::Clear()
{
	_sources.clear();
	_selected = _sources.end();
}

void MovingMonsterSources::MakeMonsters() //генерит новых монстриков
{
	if (_sources.empty()) //если нет источников выходим
		return;

	int monsterOnScreen = 0;
	//считаем сколько есть монстриков
	for(Game::Square *sq : GameSettings::squares)
	{
		if( sq->GetChip().IsThief() ) {
			++monsterOnScreen;
		}
	}

	//считаем сколько не хватает
	int needMoreMonsters = Gadgets::levelSettings.getInt("MovingMosterLimit") - monsterOnScreen;
	//если и так лишка, выходим
	if (needMoreMonsters <= 0 )
		return;

	//ищем источники на экране на которых можно сгенерить
	std::vector<Game::FieldAddress> sourcesOnScreen;
	for (Sources::iterator it = _sources.begin(); it != _sources.end(); ++it)
	{
		Game::FieldAddress fa = it->first;
		if ( Game::activeRect.Contains(fa.ToPoint()) ) //если на экране
		{
			Game::Square *sq = GameSettings::gamefield[fa];
			if (sq->IsGoodForMovingMonster()) //если может тут родиться
			{
				sourcesOnScreen.push_back(it->first); //добавляем
			}
		}
	}

	//в итоге есть sourcesOnScreen.size точек генерации и нужно needMoreMonsters монстриков
	//будем пытаться генерить по одному в случайной точке пока что нибудь не кончится
	while (needMoreMonsters > 0 && !sourcesOnScreen.empty())
	{
		size_t n = math::random(0u, sourcesOnScreen.size()-1);
		Game::FieldAddress fa = sourcesOnScreen[n]; //выбираем точку генерации и удаляем из списка
		sourcesOnScreen.erase(sourcesOnScreen.begin() + n);

		int rnd = math::random(0, 99);
		if (rnd <  Gadgets::levelSettings.getInt("MovingMosterPercent")) //если повезло, генерим
		{
			Game::Square *sq = GameSettings::gamefield[fa];

			//перевешиваем бонус если есть
			if (sq->GetChip().HasHang())
			{
				//находим куда бы повесить
				AddressVector possibleChips;
				Game::GetAddressesForHangBonus(possibleChips);

				//выбираем
				Game::Square *sqForBonus = GameSettings::gamefield[possibleChips[n]];
				
				//перенавешиваем
				std::swap(sqForBonus->GetChip().GetHang(), sq->GetChip().GetHang());
				////!!! TODO: анимамация перелета бонуса?
			}

			//запускаем анимацию съедения фишки
			Game::AddController(new ChipEatenRemover(sq, MovingMonsters::MOVING_MONSTER_BORN_TIME));

			sq->GetChip().SetThief(fa.ToPoint());

			needMoreMonsters--;
		}
	}
}

MovingMonsterSources movingMonstersSources;

}

