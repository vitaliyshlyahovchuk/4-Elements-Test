#ifndef ONCE_MOVING_MONSTER
#define ONCE_MOVING_MONSTER

#include "GameOrder.h"
#include "BaseEditorMaker.h"
#include "Array2D.h"

//монстр охотник за приемниками энергии. убивается матчем рядом
class Thief
	: public RefCounter
{
private:
	struct AnimInfo
	{
		std::string name;
		FPoint offset;
		float pause;
	};
	IPoint _index;
	FlashAnimation *_currentAnimation;

	bool _awake; //разбужен ли монстр (пока на монстр не появился на экране, он спит)
	int _moves; //сколько ходов накопил
	bool _pulse; //пульсирует (если близко к источнику)
	float _bornTime; //рождение монстрика (0-только начал, 1-полностью вырос)
	bool _isKilled;
	std::string _anim_name;
	bool _canUpdate;
	float _timer;
	bool _mirror;
	std::list<AnimInfo> _queueAnim;
	FPoint _localOffset;
	FPoint _offset0, _offset1;
	bool _turned;
	int _countNear;
public:
	void RunAnimation(std::string name, float pause = 0.f, FPoint offset = FPoint(0.f, 0.f));

	void AddAnimation(std::string name, bool immediate = false, float pause = 0.f, FPoint offset_start = FPoint(0.f, 0.f));
	static void Init(rapidxml::xml_node<> *node);
	Thief(IPoint index);
	~Thief();
	void Update(float dt);
	void Draw(FPoint pos);
	IPoint Cell() const;
	void Kill();
	bool IsKilled() const;
	bool IsTurned() const;
	std::string GetAnimationName() const;
	void Appear();
	bool IsAppear() const;
	bool IsStand() const;
	void SmartClearQueue();

	int GetMoves() const;
	void ChangeMoves(int diff);
	void SetPulse(IPoint dir);
	void SetIndex(IPoint new_index);

	typedef boost::intrusive_ptr<Thief> HardPtr;
};

class MovingMonsters
{
public:
	bool _haveMonsters; //есть ли вообще монстрики на уровне 
	int _needMoveMonsters; //сколько ходов могут сделать монстрики

	bool _canWakeMonsters; //можно ли будить. ставится в false при движении камеры и в true после хода
	                       //нужно чтобы не было такого что после хода камера двинулась и появившиеся монстрики сразу сходили
private:
	int _fieldLeft;
	int _fieldRight;
	int _fieldTop;
	int _fieldBottom;
	int _fullFieldLeft;
	int _fullFieldRight;
	int _fullFieldTop;
	int _fullFieldBottom;

	Array2D<int> _movesToRecievers; //основной массив с кратчайшими расстояниями
	Array2D<int> _energy; //где есть (и докуда дотечет) энергия   <0 - блок, 0 - может течь, 1 - энергия (не были), 2 - энергия (были)
	std::vector<Thief::HardPtr> _monstersPositions; //позиции монстриков
	std::list<Game::FieldAddress> _recieversPositions; //позиции приемников

	int _currentDistance; //текщее проверяемое расстояние
	bool _somethingChanged; //массив расстояния поменялся
	AddressVector _possibleMoves; //возможные ходы монстрика
	int _minDistance; //текущее минимальное расстояние до приемникуа
	bool _somethingMoved; //кто то из монстров двинулся

	bool _someMonsterStillCanMove; //у кого то из монстриков еще остались ходы

	bool _haveRecieversOnScreen; //на экране есть приемники. если нету монстрики ходят рандомно

	void Move(); //поиск ходов
	void MoveRandom(); //случайное перемещение
	void MoveRound1(); //поиск ходов ведущих прямо к приемнику
	void MoveRound2(); //поиск ходов чтобы придти как можно ближе к приемнику
	void FillDistances(); //заполнение расстояний до точек с "0" по волновому алгоритму
	void LoadGameState(); //начальное заполнение
	void LoadEnergy(); //загрузка энергии
	void GrowEnergy(); //рост энергии
	void UpdateDistance(Game::FieldAddress fa, Game::FieldAddress dir); //
	void FindMove(Game::FieldAddress fa, Game::FieldAddress dir);
	int MoveMonster(Game::FieldAddress fromFA, Game::FieldAddress toFA); //двигает монстрика, возвращает оставшееся число ходов
	void MonsterEat(Game::FieldAddress monsterFA, Game::FieldAddress recieverFA); //монстрик скушал получатель
	bool checkThisReceiverCanFill(AddressVector _recievers) const; //проверяем могут ли заполниться энергией в этом ходу
public:

	MovingMonsters(void);
	~MovingMonsters(void);
	
	void Update(float dt);
	//обновляет пульсацию монстриков рядом с приемником
	void UpdatePulse(float dt);

	bool IsEatingReceiver() const;

	static const int MOVING_MONSTER_BORN_TIME = 1; //время в секундах в течение которого рождается монстрик
};

namespace Gadgets
{

class MovingMonsterSources : public BaseEditorMaker
{
	typedef std::map<Game::FieldAddress, int> Sources;
	Sources::iterator _selected;
public:
	Sources _sources;

	void LoadLevel(rapidxml::xml_node<> *root);
	void SaveLevel(Xml::TiXmlElement *root);

	void Clear();
	bool Editor_LeftMouseDown(const IPoint &mouse_pos, Game::Square *sq);
	bool Editor_RightMouseDown(const IPoint &mouse_pos, Game::Square *sq);
	void DrawEdit();
	void MakeMonsters();
//	bool AcceptMessage(const Message &message);

//	ChipSource* GetSource(Game::FieldAddress fa);
};

extern MovingMonsterSources movingMonstersSources;

}

#endif
