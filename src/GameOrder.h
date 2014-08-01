#ifndef __HANG_ORDER_H__
#define __HANG_ORDER_H__

#include "GameFieldAddress.h"

namespace Game
{

class Order
{
protected:
	IPoint _cell;
	bool _active; //Заказ в данный момент собирается
public:

	enum Objective
	{
		CHIP		= 0,
		WALL		= 40,
		GROUND		= 41,
		LICORICE	= 43,
		MUSOR       = 44,
		TIME_BOMB   = 45,
		ENERGY      = 46,
		MOVING_MONSTER	= 47,
		BEAR        = 48,
		DIAMOND     = 49
	};

	enum Type
	{
		EMPTY,
		KILL_CELL,
		FILL_ENERGY
	};
	
	float _timeAppearIcon, _timeAppearText;

	Order();
	virtual ~Order() {}

	virtual void InitAppear(const float pause_icon, const float pause_text) = 0;
	virtual void Update(float dt);

	bool IsActive() const;
	void Activate(bool enable);
	// выполнен ли заказ
	virtual bool Completed() const = 0;
	virtual bool IsReadyToBreak() const = 0;

	virtual void Draw(IPoint pos, bool vertical = true) {}

	virtual void Save(Xml::TiXmlElement *elem);

	// вызываем при убийстве фишки / земли / шоколада / прочей фигни
	virtual void KillCell(Objective type, Game::FieldAddress address) {}

	// вызываем в конце каждого хода
	virtual void OnEndMove() {}

	// вызываем в процессе выделения цепочки, сигнализируя что сейчас убьем некоторое
	// кол-во фишек или еще чего-нить
	virtual void SelectSequence(Objective type, int count) {}

	virtual std::string GetTooltip() const { return std::string(); }

	virtual Type GetType() const = 0;

	void SetAddress(IPoint pos) { _cell = pos; }
	IPoint GetAddress() const { return _cell; }
	bool OnScreen() const;

	typedef boost::shared_ptr<Order> HardPtr;
	typedef boost::weak_ptr<Order> WeakPtr;

	static HardPtr Load(rapidxml::xml_node<> *elem);
	static void TrackOrder(HardPtr order);
};

std::vector<Order::WeakPtr> &GetAllOrders();

class EmptyOrder : public Order
{
public:
	bool Completed() const { return true; }
	bool IsReadyToBreak() const {return true;}

	void Save(Xml::TiXmlElement *elem);
	
	void InitAppear(const float pause_icon, const float pause_text){};
	Type GetType() const { return EMPTY; }
};

// Заказ уничтожить определенное кол-во фишек заданного цвета за один или за несколько ходов
class DestroyChipsOrder : public Order
{
	Objective _cell_type;
	int _rect_num;
	int _count_selected;
	int _count_killed;
	int _count;
	float _drawShowCount;
	bool _reset_after_move;

	void InitTextures();
	bool ObjectiveCompleted() const;
public:
	DestroyChipsOrder(rapidxml::xml_node<> *elem);
	DestroyChipsOrder(Objective objective, int count, bool reset_after_move);
	void InitAppear(const float pause_icon, const float pause_text);

	bool Completed() const;
	bool IsReadyToBreak() const;

	void Draw(IPoint pos, bool vertical = true);

	void Save(Xml::TiXmlElement *elem);

	void KillCell(Objective type, Game::FieldAddress address);
	void OnEndMove();
	void SelectSequence(Objective type, int count);

	std::string GetTooltip() const;

	Type GetType() const { return KILL_CELL; }

	std::string GetObjectiveType() const;
	int GetChipColor() const { return (int)_cell_type; }
	int GetCount() const { return _count; }
	int GetCountKilled() const { return _count_killed; }
	bool NeedResetAfterMove() const { return _reset_after_move; }
	void Update(float dt);
};

// Заказ заполнить энергией определенную область
class FillEnergyOrder : public Order
{
	std::set<Game::FieldAddress> _area;
	std::set<Game::FieldAddress> _filled;
public:
	FillEnergyOrder(rapidxml::xml_node<> *elem);
	FillEnergyOrder(const std::set<Game::FieldAddress>& area);

	void InitAppear(const float pause_icon, const float pause_text);
	const std::set<Game::FieldAddress>& GetArea() const;
	void SetArea(const std::set<Game::FieldAddress> &area);
	void AddCell(Game::FieldAddress cell);
	void DeleteCell(Game::FieldAddress cell);

	void Draw(IPoint pos, bool vertical = true);
	void KillCell(Objective type, Game::FieldAddress address);
	bool ContainsCell(Game::FieldAddress fa) const;
	bool Completed() const;
	bool IsReadyToBreak() const;
	void Save(Xml::TiXmlElement *elem);

	Type GetType() const { return FILL_ENERGY; }
};

namespace Orders
{
void KillCell(Order::Objective type, Game::FieldAddress address);
void OnEndMove();
void SelectSequence(Order::Objective type, int count);
std::string GetTooltip();

// есть ли заказ на заполнение данной клетки энергией (для визуализации таких клеток)
bool CellIsOrdered(Game::FieldAddress cell);

// есть ли заказ (и где) на уничтожение фишек данного цвета (для эффекта полета фишек к замку)
Game::FieldAddress ChipIsOrdered(int color, int sequenceLength);

DestroyChipsOrder* GetDiamondsOrderOnScreen();
}

} // end of namespace


class OrderConfigWidget : public GUI::Widget
{
	std::string orderType;

	Render::Texture *chipTex;
	int _chipColor;
public:
	OrderConfigWidget(std::string name, rapidxml::xml_node<>* xmlElement);

	bool MouseDown(const IPoint &mouse_pos);
	void MouseDoubleClick(const IPoint &mouse_pos);
	void MouseUp(const IPoint &mouse_pos);
	void MouseMove(const IPoint &mouse_pos);
	void MouseWheel(int delta);

	void Draw();
	void Update(float dt);

	void AcceptMessage(const Message& message);
	Message QueryState(const Message& message) const;
};

#endif