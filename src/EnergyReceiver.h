#ifndef ONCE_ENERGY_RECEIVER
#define ONCE_ENERGY_RECEIVER

#include "GameOrder.h"
#include "GameSquare.h"
#include "SnapGadgetElements.h"

struct WallState
{
	int wall;
	bool energy_wall;

	WallState() : wall(0), energy_wall(false) {}

	void Read(Game::Square *sq);
	void Apply(Game::Square *sq);
};

class EnergyReceiver
{
private:
	float _timerForRunChipFall; //Таймер для запуска осыпания фишек после активации приемника
	//Клетка на поле
	IPoint _index;
	Game::Square *_cell;

	Render::Texture *_textureBaseDown;
	Render::StreamingAnimationPtr _crystalAnim;

	//Гуляющий по полю приемник энергии (сейчас рисуется немного зеленым)
	bool _can_walk;

	//Привязанные точки привязки для блуждающих приемников
	std::vector<SnapGadgetBaseElement::SHARED_PTR> _shapElements;

	WallState _saved_wall;
	std::string _uid;

	bool _hideByThief;
	float _hideByThiefTime;
	
	Game::Order::HardPtr _order;

	ParticleEffectPtr _partUp;


	EffectsContainer _effContDown, _effContUp, _effectCellCont, _effContUpStatic;

	//float _timerLight;
	float _localTime;
	float _fly_offset_y;
	bool _firstShowed, _lockLiveEffectShowed; //Показываем эффект первого появления алтаря на экране после этого с задержкой эффект появления заказа на замке(если присутствует)
	float _timerForAppearOrder;
	float _timeAppear; //Таймер для появления ресивера из дыры


	enum State
	{
		ER_INIT,
		ER_APPEAR,
		ER_STAND,
		ER_ACTIVATED_BREAK,
		ER_ACTIVATED_BREAK_FINISHFLY,
	}_state;

	//int _frame1, _frame2;
	//float _crystalTime;
	float _crystalTimeScale;

private:
	void Init(int row, int col, bool can_walk); // constructor
public:
	//static const FPoint CRYSTAL_POS_ANIM_ON_SQUARE;
public:
	EnergyReceiver();
	EnergyReceiver(int center_x, int center_y, bool walking);
	~EnergyReceiver();

	IPoint GetIndex() const;
	void SetIndex(IPoint pos);

	bool IsActualForCell() const;
	bool IsActivatedByEnergy() const;
	bool IsFlying() const;
	bool IsFinished() const;

	void ActivateByEnergy();

	IPoint GetCenterPos() const;

	void LoadLevel(rapidxml::xml_node<>* root);
	void SaveLevel(Xml::TiXmlElement *root);

	void InitLevel();
	void ReleaseSquares();

	void WalkTo(Game::Square *sq);
	bool IsWalking() const;
	void SetWalking(bool enable);

	void DrawDown();
	void DrawBase();
	void DrawUp();

	void Update(float dt);

	bool SetOrder(Game::Order::HardPtr order);
	Game::Order::HardPtr GetOrder();
	bool IsOrdered() const;

	void InitEffects();
	void ReleaseEffects();

	bool AcceptMessage(const Message &message);
	void HideByThief(float pause);
	bool IsCellDraw();

	typedef boost::shared_ptr<EnergyReceiver> HardPtr;
public: //Editor
	void Editor_DrawIndicators(bool selected);
	void Editor_Move(IPoint delta);	
	Game::Order::HardPtr* Editor_GetOrderPtr();	

};

#endif