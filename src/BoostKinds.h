#ifndef ONCE_BOOST_KINDS
#define ONCE_BOOST_KINDS

#include "GameFieldAddress.h"
#include "GameFieldController.h"
#include "Tutorial.h"

///////////////////// базовый клас буста ///////////////////////////////////////
class AbstractBoostKind  //вид буста
{
private:
	std::string _name;            //уникальное имя (для хранения и т.п.
	std::string _displayName;     //отображаемое имя

	bool _buyBeforeLevel;        //можно ли покупать перед началом уровня
	bool _buyDuringLevel;        //можно ли покупать во время игры
	bool _isInterfaceBoost;      //внедряется ли буст в интерфейс (типа например "лопата")

public:
	int _flashButtonNumber;      //номер кнопки в интерфейсе
	IRect _flashButtonRect;      //прямоугольник кнопки

public:
	AbstractBoostKind(std::string name, std::string displayName, bool buyBeforeLevel, bool buyDuringLevel, bool isInterfaceBoost):
		_name(name), _displayName(displayName), _buyBeforeLevel(buyBeforeLevel), _buyDuringLevel(buyDuringLevel), _isInterfaceBoost(isInterfaceBoost)
	{
	}

	~AbstractBoostKind(void)
	{
	}

	std::string getName();
	std::string getDisplayName();
	bool getBuyBeforeLevel();        //можно ли покупать перед началом уровня
	bool getBuyDuringLevel();        //можно ли покупать во время игры
	bool IsInterfaceBoost();        //интерфейсный ли буст
	void useBonusAndDecCount();            //использует бонус и уменьшает запас
	void DecCount();             //уменьшение запаса буста
	void IncCount();             //увеличение запаса (при отмене интерфейсного буста)
	
	//шаблоны функций интерфейсного бонуса
	virtual void Cancel() {};               //отмена интерфейсного бонуса
	virtual void DoAddresClick(Game::FieldAddress destination) {};    //обработка клика по клетке (для интерфейсных бонусов)
	virtual void MouseDown(Game::FieldAddress pos) {};    
	virtual void MouseUp(Game::FieldAddress pos) {};    
	virtual void MouseMove(Game::FieldAddress pos) {};
	virtual void MouseMove(const IPoint &mouse_pos) {};
	virtual void UseBoost() = 0;                  //выполняет действия буста (инициализация для интерфейсного)
	virtual void Draw() {};
	virtual void FinalizeInterfaceBoost();  //действия при завершении работы - уменьшить запас и все такое
	virtual void Run() = 0;                   //выполняет действия интерфейсного буста. вызывается по зеленой кнопке !! нужно перекрыть
	//для во время игры но не интерфейсного просто вызывает стандартное исполнение

	virtual void OnFieldMoving(bool value); //вызывается при движении камеры. по умолчанию - просто нельзя применять пока движется

	bool thisSquareHitByBoost(Game::FieldAddress fa);	//можно ли убрать эту фишку лопатой/линией/бомбой и т.п.
	void getScreenRectsFromAddressVector(AddressVector &addresses, std::vector<IRect> &rects);	//получает список экранных прямогуольников из координат, для подсветки

	void SetCanApply(int value); //включает или выключает доступность кнопки запуска буста
	virtual void AcceptMessage(const Message &message) {};
};

//////////////////////////////
//перечень бустов используемых в программе
class BoostList
{
public:
	typedef boost::shared_ptr<AbstractBoostKind> HardPtr; //ссылка на буст
	typedef boost::weak_ptr<AbstractBoostKind> WeakPtr; //ссылка на буст

private:
	std::vector<HardPtr> _boostList;
public:
	void addBoost(HardPtr newBoost);
	const std::vector<HardPtr> getBoostList();

	void StandardInit();

	WeakPtr FindByName(std::string name);

	void AddUsedBoost(std::string bonusName);  //добавить в список бонус используемый до уровня
	void UseBoostInstantly(std::string bonusName); //использовать бонус (вывызвается из lua во время игры)

	void RunCurrentBoost(BoostList::WeakPtr boost); //запуск интрефейсного буста
	void CancelCurrentBoost(BoostList::WeakPtr boost); //отмена интрефейсного буста

	bool NeedConfirmationForBoost(std::string bonusName);  //требуется ли подтверждение использования буста
	bool CanStartBoostNow(std::string bonusName);  //можно ли запустить этот буст сейчас (например нельзя пока камера едет)

	static Game::FieldAddress EmptyAddres; //пустая клетка
};

typedef void (*UseBonusFunction)(); //функция буста 

////////////////////////////////////////////////////////////////////////////////////
// обычный (не интерфейсный) класс буста. получает в конструкторе указатель на основную функцию  буста
class SimpleBoostKind:public AbstractBoostKind
{
private: 
	UseBonusFunction _function;

public:
	SimpleBoostKind(std::string name, std::string displayName, bool buyBeforeLevel, bool buyDuringLevel, 
		UseBonusFunction function):AbstractBoostKind(name, displayName, buyBeforeLevel, buyDuringLevel, false)
	{
		_function = function;
	}

	virtual void UseBoost() override;                  //выполняет подготовку буста (если требуется) пока только показывает область поражения

	virtual void Run();                   //выполняет действия интерфейсного буста. (вызывается по зеленой кнопке) для простых бустов операция одна и та же

	virtual void Cancel() override;               //отмена интерфейсного бонуса
	virtual void ShowAffected() {};            //показать область поражения (если требуется).
	virtual void HideAffected() {};            //убрать область поражения (если требуется).
};

////// буст рост энергии. умеет показывать куда он вырастет

class BoostGrowEnergyAround:public SimpleBoostKind
{
public:
	BoostGrowEnergyAround(std::string name, std::string displayName, bool buyBeforeLevel, bool buyDuringLevel):
		SimpleBoostKind(name, displayName, buyBeforeLevel, buyDuringLevel, nullptr)
	{ }

	void ShowAffected() override;            //показать область поражения (если требуется).
	void HideAffected() override;            //убрать область поражения (если требуется).
	void Run() override;
};


//////////////////////////////////////////// интерфейсные бусты ///////////////////
////////// контроллер мышки для бустов ///////
class SquareClickController:public GameFieldController
{
private: 
	Game::FieldAddress _addresClicked;
	AbstractBoostKind *_boost;

public:
	SquareClickController(AbstractBoostKind *boost);

	bool MouseDown(const IPoint &mouse_pos) override;
	bool MouseMove(const IPoint &mouse_pos) override;
	bool MouseUp(const IPoint &mouse_pos) override;
	void Update(float dt) override {};
	bool isFinish() override {return false;};

};

//////////////////////////////// Абстрактный буст типа "кликни и взорвется" ////////////////////
// является базовым для: лопаты, вертикального/горизонтального ряда, бомбы, креста
// действия: ждет клика по клетке, отображает перечень потенциально уничтожаемых клеток
// (используя UpdateAffectedCells)
// можно ограничить набор клеток для клика (FillAllowClick)
// при клике запускает DoBoostAction где надо убрать все ненужное

class AbstractClickAndDeleteBoost:public AbstractBoostKind
{
protected: 
	SquareClickController *_squareClick; //контроллео кликер
	Game::FieldAddress _currentCell; //где кликают
	AddressVector _sequenceCells; //исходные клетки (которые уничтожаются собственно бустом)
	AddressVector _affectedCells; //клетки которые взорвутся от бонусов
	AddressVector _allowClick; //где разрешен клик

public:

	AbstractClickAndDeleteBoost(std::string name, std::string displayName, bool buyBeforeLevel, bool buyDuringLevel):
		AbstractBoostKind(name, displayName, buyBeforeLevel, buyDuringLevel, true)
	{
	}

	virtual void UseBoost();                  //выполняет действия буста (подготовка)

	virtual void FillAllowClick() =0;  //заполнить список разрешенных для клика ячеек !! нужно перекрыть
	virtual void UpdateAffectedCells() = 0;                   //обновляет список убиваемых фишек !! нужно перекрыть
	virtual void DoBoostAction() = 0;                   //выполняет удаление клеток !! нужно перекрыть

	void AddBonusAffected();	//прибавляет к _affectedCells еще клетки поражаемые бонусами
	                            //side-effect: заполняет gamefield->_bonuscascade, надо чистить (в Cancel например)
	void CountFromCurrentCell(); //вызывается после изменения _currentCell. пересчитывает зону поражения и показывает на экране
	void ClearAfectedCells();  //стандартный механизм удаления клеток

	void DoAddresClick(Game::FieldAddress destination) override;
	virtual void ProcessAddresSelect(Game::FieldAddress destination);	//обработка выбора текущей клетки
	void MouseMove(Game::FieldAddress pos) override;
	void MouseUp(Game::FieldAddress pos) override;
	void MouseDown(Game::FieldAddress pos) override;
	void Run() override;

	void OnFieldMoving(bool value) override; //вызывается при движении камеры. отменяет выделенную ячейку
	void SelectRandomCurrentCell();  //выбор случайной стартовой клетки. запускается при старте буста и при остановке камеры

	void Cancel() override;               //отмена

	void AddIfAllow(Game::FieldAddress fa, AddressVector &cells);
	void AcceptMessage(const Message &message) override;
};

/////////////////////////////////буст - лопата /////////////////////////
class BoostSpade:public AbstractClickAndDeleteBoost
{
private: 

public:
	BoostSpade(std::string name, std::string displayName, bool buyBeforeLevel, bool buyDuringLevel):
		AbstractClickAndDeleteBoost(name, displayName, buyBeforeLevel, buyDuringLevel)
	{
	}

	void FillAllowClick() override;  //заполнить список разрешенных для клика ячеек !! нужно перекрыть
	void UpdateAffectedCells() override;                   //обновляет список убиваемых фишек !! нужно перекрыть
	void DoBoostAction() override;                   //выполняет действия буста !! нужно перекрыть

};

//////////////////////////////// Буст - горизонтальный (вертикальный) ряд ////////////////////

class BoostClearLine:public AbstractClickAndDeleteBoost
{
private: 
	bool _isHorizontal;    // true - горизонтальный, false - вертикальный

public:

	BoostClearLine(std::string name, std::string displayName, bool buyBeforeLevel, bool buyDuringLevel):
		AbstractClickAndDeleteBoost(name, displayName, buyBeforeLevel, buyDuringLevel), 
		_isHorizontal(true)
	{
	}

	void DoAddresClick(Game::FieldAddress destination) override; //щелчок по той же клетке - меняем с вертикального на горизонтальное
	void FillAllowClick() override;  //заполнить список разрешенных для клика ячеек !! нужно перекрыть
	void UpdateAffectedCells() override;                   //обновляет список убиваемых фишек !! нужно перекрыть
	void DoBoostAction() override;                   //выполняет действия буста !! нужно перекрыть

};

//////////////////////////////// Буст - крест (убирает фишки по горизонтали и вертикали) ////////////////////

class BoostCross:public AbstractClickAndDeleteBoost
{
private:
	IRect fieldBound;

public:

	BoostCross(std::string name, std::string displayName, bool buyBeforeLevel, bool buyDuringLevel):
		AbstractClickAndDeleteBoost(name, displayName, buyBeforeLevel, buyDuringLevel)
		, fieldBound(IRect())
	{
	}

	void FillAllowClick() override;  //заполнить список разрешенных для клика ячеек !! нужно перекрыть
	void UpdateAffectedCells() override;                   //обновляет список убиваемых фишек !! нужно перекрыть
	void DoBoostAction() override;                   //выполняет действия буста !! нужно перекрыть
	void AcceptMessage(const Message &message) override;
};

/////////////////////////////////буст - обмен фишек /////////////////////////
class BoostExchange:public AbstractBoostKind
{
private: 
	AddressVector _allowExchange; //которые можно менять
	bool _firstChipSelected;	//выбрана ли первая фишка
	bool _secondChipSelected;	//выбрана ли первая фишка
	Game::FieldAddress _firstChip; //выбранная фишка, с которой меняемся
	Game::FieldAddress _secondChip; //выбранная фишка, с которой меняемся
	SquareClickController *_squareClick;

public:
	BoostExchange(std::string name, std::string displayName, bool buyBeforeLevel, bool buyDuringLevel):
		AbstractBoostKind(name, displayName, buyBeforeLevel, buyDuringLevel, true)
	{
	}

	virtual void UseBoost();                  //выполняет действия буста

	void DoAddresClick(Game::FieldAddress destination);
	void Draw() override;
	void Cancel() override;               
	void Run() override;               
	void UpdateLuaState(); //обновляет квадратики и кнопочки в lua
	void FillAllowExchange();	//заполняет список кого можно менять
	void OnFieldMoving(bool value) override;
};


//////////////////////////////// Буст - большая бомба ////////////////////
class BoostBigBomb:public AbstractClickAndDeleteBoost
{
private: 

public:
	BoostBigBomb(std::string name, std::string displayName, bool buyBeforeLevel, bool buyDuringLevel):
		AbstractClickAndDeleteBoost(name, displayName, buyBeforeLevel, buyDuringLevel)
	{
	}

	void FillAllowClick() override;  //заполнить список разрешенных для клика ячеек !! нужно перекрыть
	void UpdateAffectedCells() override;                   //обновляет список убиваемых фишек !! нужно перекрыть
	void DoBoostAction() override;                   //выполняет действия буста !! нужно перекрыть

};

//////////////////////////////// Буст - удаление фишек одного цвета ////////////////////
class BoostClearColor:public AbstractClickAndDeleteBoost
{
private: 

public:
	BoostClearColor(std::string name, std::string displayName, bool buyBeforeLevel, bool buyDuringLevel):
		AbstractClickAndDeleteBoost(name, displayName, buyBeforeLevel, buyDuringLevel)
	{
	}

	void FillAllowClick() override;  //заполнить список разрешенных для клика ячеек !! нужно перекрыть
	void UpdateAffectedCells() override;                   //обновляет список убиваемых фишек !! нужно перекрыть
	void DoBoostAction() override;                   //выполняет действия буста !! нужно перекрыть
	void AcceptMessage(const Message &message) override;
};

class BoostFreeSequence: public AbstractBoostKind
{
private:
	SquareClickController *_squareClick;
	AddressVector _allowSelect;		// которые можно выделять
	AddressVector _sequenceCells;	// уже выделенные
	bool _sequenceStarted;			// начата ли цепочка
	bool _allowSelecting;

public:
	BoostFreeSequence(std::string name, std::string displayName, bool buyBeforeLevel, bool buyDuringLevel):
		AbstractBoostKind(name, displayName, buyBeforeLevel, buyDuringLevel, true)
		, _sequenceStarted(false)
		, _allowSelecting(false)
	{
	}

	virtual void UseBoost();                  //выполняет действия буста

	void MouseDown(Game::FieldAddress pos);
	void MouseUp(Game::FieldAddress pos);
	void MouseMove(const IPoint &mouse_pos);
	void Cancel() override;               
	void Run() override;               

	void AddToSeq(Game::FieldAddress destination);
	void ResetSeq();
	void FillAllowSelect();
};

// контроллер перекрашивающий фишки (используется бустом BoostClearColorBeforeGame)
class ChipsRecolorController: public GameFieldController
{
	enum { PAUSE, RECOLORING, FINISHED } _state;	// состояние
	float _pause;									// пауза перед стартом
	float _timer;									// таймер перекраски
	AddressVector _cellsToRecolor;					// фишки для перекраски
	std::vector<int> _colors;										// цвет перекраски
	float _effectPause;								// пауза после старта еффекта
	int _paintsCount;								// количество перекрашенных (в данный момент) фишек
	
	//информация о перекрашиваемой фишке
	struct ChipRecolorInfo {
		float delay;
		bool recolored;
		bool effectStarted;
		int toColor;
	};
	std::vector<ChipRecolorInfo> _chipsRecolorInfo;

public:
	ChipsRecolorController(AddressVector cells, std::vector<int> colors, float pause);

	void Init();
	void Update(float dt);
	void ChangeColor(Game::FieldAddress chip, int indx);
	void StartEffect(Game::FieldAddress chip, int indx);
	bool isFinish();
};

// контроллер запускающий падение фишек
class BoostRunFallController: public GameFieldController
{
private:
	float _pause;
	float _timer;
	AddressVector _cells;
	enum { PAUSE, FINISHED } _state;

public:
	BoostRunFallController(AddressVector cells, float pause);
	void Update(float dt);
	bool isFinish();
};

#endif