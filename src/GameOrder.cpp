#include "stdafx.h"
#include "GameOrder.h"
#include "Game.h"
#include "GameField.h"

namespace Game
{

static std::vector<Order::WeakPtr> all_orders;

std::vector<Order::WeakPtr>& GetAllOrders() //используется для моделирования ходов в FUUU*
{
	return all_orders;
}


Render::Texture *ordersTexture = NULL;;
bool Order::OnScreen() const
{
	return Game::activeRect.Contains(_cell);
}

Order::HardPtr Order::Load(rapidxml::xml_node<> *elem)
{
	std::string type = Xml::GetStringAttribute(elem, "type");
	HardPtr order;
	if( type == "kill_cell" ) {
		order = boost::make_shared<DestroyChipsOrder>(elem);
	} else if( type == "fill_energy" ) {
		order = boost::make_shared<FillEnergyOrder>(elem);
	} else if( type == "empty" ) {
		order = boost::make_shared<EmptyOrder>();
	} else {
		Assert2(false, "Unsupported order type: " + type);
	}

	bool delayed = Xml::GetBoolAttributeOrDef(elem, "delayed", false);
	order->Activate(!delayed);

	TrackOrder(order);

	return order;
}

void Order::Save(Xml::TiXmlElement *elem)
{
	elem->SetAttribute("delayed", utils::lexical_cast(!_active));
}

Order::Order()
	: _active(true)
	, _timeAppearIcon(1.f)
	, _timeAppearText(1.f)
{
}

void Order::Update(float dt)
{
	if(_timeAppearIcon < 0)
	{
		_timeAppearIcon += dt;
	}else if(_timeAppearIcon < 1){
		_timeAppearIcon += dt*6.f;
		if(_timeAppearIcon > 1)
		{
			_timeAppearIcon = 1.f;
		}
	}

	if(_timeAppearText < 0)
	{
		_timeAppearText += dt;
	}else if(_timeAppearText < 1){
		_timeAppearText += dt*3.f;
		if(_timeAppearText > 1)
		{
			_timeAppearText = 1.f;
		}
	}
	
}

bool Order::IsActive() const
{
	return _active;
}

void Order::Activate(bool enable)
{
	_active = enable;
}

void Order::TrackOrder(Order::HardPtr order)
{
	if( order ) {
		all_orders.push_back( Order::WeakPtr(order) );
	}
}

void EmptyOrder::Save(Xml::TiXmlElement *elem)
{
	Order::Save(elem);
	elem->SetAttribute("type", "empty");
}

DestroyChipsOrder::DestroyChipsOrder(rapidxml::xml_node<> *elem)
	: _count_selected(0)
	, _count_killed(0)
	, _drawShowCount(0.f)
{
	_count = Xml::GetIntAttribute(elem, "count");
	_drawShowCount = _count;
	_reset_after_move = Xml::GetBoolAttributeOrDef(elem, "one_move", true);
	_cell_type = Objective( Xml::GetIntAttribute(elem, "cell") );
	InitTextures();
}

DestroyChipsOrder::DestroyChipsOrder(Objective objective, int count, bool _reset_after_move)
	: _count(count)
	, _count_selected(0)
	, _count_killed(0)
	, _cell_type(objective)
	, _reset_after_move(_reset_after_move)
	, _drawShowCount(count)
{
	InitTextures();
}

void DestroyChipsOrder::InitAppear(const float pause_icon, const float pause_text)
{
	_timeAppearIcon = -pause_icon;
	_timeAppearText = -pause_text;
}

void DestroyChipsOrder::InitTextures()
{
	Game::ordersTexture = Core::resourceManager.Get<Render::Texture>("LockOrders");
	if(_cell_type < WALL){
		_rect_num = _cell_type - 1;
		if(_reset_after_move)
		{
			_rect_num += 8;
		}
	}else if(_cell_type == WALL){
		_rect_num = 16;
	}else if(_cell_type == GROUND){
		_rect_num = 17;
	}else if(_cell_type == LICORICE){
		_rect_num = 19;
	}else if(_cell_type == MUSOR){
		_rect_num = 20;
	}else if(_cell_type == TIME_BOMB){
		_rect_num = 21;
	}else if(_cell_type == ENERGY){
		_rect_num = 22;
	}else if(_cell_type == MOVING_MONSTER){
		_rect_num = 25;
	}else if(_cell_type == BEAR){
		_rect_num = 7;
	}else if(_cell_type == DIAMOND){
		_rect_num = 26;
	}
	_rect_num = math::clamp(0, 32, _rect_num);
}

bool DestroyChipsOrder::IsReadyToBreak() const
{
	return _drawShowCount <= 0.0f;
}

bool DestroyChipsOrder::ObjectiveCompleted() const
{
	return (_count_killed >= _count);
}

bool DestroyChipsOrder::Completed() const
{
	return IsReadyToBreak() && ObjectiveCompleted();
}

void GetOrderIconRect(int type, FRect &rect, FRect &frect)
{
	const float cell_w = 64.f;
	const float cell_h = 64.f;
	rect = FRect(0.f, cell_w, 0.f, cell_w).MovedTo(FPoint(-0.5f*cell_w, -0.5f*cell_h));
	float u_size = cell_w/512.f;
	float v_size = cell_h/256.f;
	frect = FRect(0.f, u_size, 0.f,  v_size).MovedTo(FPoint(u_size*float(type % 8), v_size*float(3 - (type)/8)));
}

void DestroyChipsOrder::Update(float dt)
{
	Order::Update(dt);
	int showCount = _reset_after_move ? std::max(_count - _count_selected, 0) : (_count - _count_killed);
	if( ObjectiveCompleted() )
		showCount = 0;

	if( _drawShowCount > showCount)
	{
		_drawShowCount -= dt * 8.0f;
		if(_drawShowCount < 0.0f){
			_drawShowCount = 0.0f;
		}
	} else {
		_drawShowCount = showCount + 0.f;
	}
}


void DestroyChipsOrder::Draw(IPoint pos, bool vertical)
{
	if(_timeAppearText > 0)
	{
		//Текст
		Render::BeginAlphaMul(math::clamp(0.f, 1.f, _timeAppearText));
		if(_drawShowCount > 0.0f && !Completed())
		{
			IPoint textOffset = vertical ? IPoint(40, 14) : IPoint(54, 24);
			Render::FreeType::BindFont("OrderOnLock");
			Render::PrintString(pos + textOffset, utils::lexical_cast(math::ceil(_drawShowCount)), 1.f, CenterAlign, CenterAlign);
		}
		Render::EndAlphaMul();
	}
	if(_timeAppearIcon > 0)
	{
		FPoint iconOffset = vertical ? FPoint(40.0f, 53.0f) : FPoint(24.0f, 24.0f);
		//Иконка
		Render::BeginAlphaMul(math::clamp(0.f, 1.f, _timeAppearIcon));
		FRect rect, frect;
		GetOrderIconRect(_rect_num, rect, frect);
		rect.MoveBy(FPoint(0, 30.f)*(1.f - _timeAppearIcon) + pos + iconOffset);
		Game::ordersTexture->Bind();
		Render::DrawRect(rect, frect);
		Render::EndAlphaMul();
	}
}

void DestroyChipsOrder::Save(Xml::TiXmlElement *elem)
{
	//по алфавиту !
	elem->SetAttribute("cell", _cell_type);
	elem->SetAttribute("count", _count);
	Order::Save(elem); //delayed
	elem->SetAttribute("one_move", _reset_after_move ? "1" : "0");
	elem->SetAttribute("type", "kill_cell");
}

void DestroyChipsOrder::KillCell(Objective type, Game::FieldAddress address)
{
	if( _cell_type == type ) {
		if (_count_killed < _count)
		{
			++_count_killed;
			MM::manager.PlaySample("ReceiverOrderUpdate");
		}
//		_count_killed = std::min(_count_killed + 1, _count);
	}
}

void DestroyChipsOrder::OnEndMove()
{
	if( !ObjectiveCompleted() )
	{
		if(_reset_after_move) {
			_count_killed = 0;
		}
	}
}

void DestroyChipsOrder::SelectSequence(Objective type, int count)
{
	if(_cell_type == type) {
		_count_selected = count;
	}
}

std::string DestroyChipsOrder::GetTooltip() const
{
	if(_count_selected >= 2 && _reset_after_move) {
		return utils::lexical_cast(_count_selected) + "/" + utils::lexical_cast(_count);
	} else {
		return std::string();
	}
}

std::string DestroyChipsOrder::GetObjectiveType() const
{
	switch(_cell_type) {
		case WALL: return "walls";
		case GROUND: return "ground";
		case LICORICE: return "licorice";
		case MUSOR: return "musor";
		case TIME_BOMB: return "time_bomb";
		case MOVING_MONSTER: return "pirate";
		case BEAR: return "bear";
		case DIAMOND: return "diamond";
		default:
			if(_cell_type < WALL) {
				return _reset_after_move ? "sequence" : "chips";
			} else {
				Assert(false);
			}
	};
	return "empty";
}

FillEnergyOrder::FillEnergyOrder(rapidxml::xml_node<> *elem)
{
	rapidxml::xml_node<> *cell = elem->first_node("Cell");
	while( cell )
	{
		_area.insert( Game::FieldAddress(cell) );
		cell = cell->next_sibling("Cell");
	}
}

FillEnergyOrder::FillEnergyOrder(const std::set<Game::FieldAddress>& area)
	: _area(area)
{
}
void FillEnergyOrder::InitAppear(const float pause_icon, const float pause_text)
{

}

const std::set<Game::FieldAddress>& FillEnergyOrder::GetArea() const
{
	return _area;
}

void FillEnergyOrder::SetArea(const std::set<Game::FieldAddress> &area)
{
	_area = area;
}

void FillEnergyOrder::AddCell(Game::FieldAddress cell)
{
	_area.insert(cell);
}

void FillEnergyOrder::DeleteCell(Game::FieldAddress cell)
{
	_area.erase(cell);
}

void FillEnergyOrder::Draw(IPoint pos, bool vertical)
{
	if(!Completed())
	{
		Render::FreeType::BindFont("debug");
		Render::PrintString(pos + IPoint(GameSettings::SQUARE_SIDE*3/4, GameSettings::SQUARE_SIDE*3/4), utils::lexical_cast(_area.size()), 1.f, CenterAlign, CenterAlign);
	}
}

void FillEnergyOrder::KillCell(Objective type, Game::FieldAddress address)
{
	if( type == ENERGY )
	{
		if(_area.erase(address))
			_filled.insert(address);
	}
}

bool FillEnergyOrder::ContainsCell(Game::FieldAddress fa) const
{
	return _area.count(fa) || _filled.count(fa);
}

bool FillEnergyOrder::Completed() const
{
	return _area.empty();
}

bool FillEnergyOrder::IsReadyToBreak() const
{
	return false;
}

void FillEnergyOrder::Save(Xml::TiXmlElement *elem)
{
	Order::Save(elem);

	elem->SetAttribute("type", "fill_energy");
	for(std::set<Game::FieldAddress>::iterator itr = _area.begin(); itr != _area.end(); ++itr)
	{
		Xml::TiXmlElement *cell = elem->InsertEndChild(Xml::TiXmlElement("Cell"))->ToElement();
		itr->SaveToXml(cell);
	}
}

namespace Orders
{

void KillCell(Order::Objective type, Game::FieldAddress address)
{
	for(std::vector<Order::WeakPtr>::iterator itr = all_orders.begin(); itr != all_orders.end(); )
	{
		Order::HardPtr order = (*itr).lock();
		if( order ) {
			if(order->OnScreen() && order->IsActive())
				order->KillCell(type, address);
			++itr;
		} else {
			itr = all_orders.erase(itr);
		}
	}
}

void OnEndMove()
{
	for(std::vector<Order::WeakPtr>::iterator itr = all_orders.begin(); itr != all_orders.end(); )
	{
		Order::HardPtr order = (*itr).lock();
		if( order ) {
			order->OnEndMove();
			++itr;
		} else {
			itr = all_orders.erase(itr);
		}
	}
}

void SelectSequence(Order::Objective type, int count)
{
	for(std::vector<Order::WeakPtr>::iterator itr = all_orders.begin(); itr != all_orders.end(); )
	{
		Order::HardPtr order = (*itr).lock();
		if( order ) {
			if(order->OnScreen() && order->IsActive())
				order->SelectSequence(type, count);
			++itr;
		} else {
			itr = all_orders.erase(itr);
		}
	}
}

std::string GetTooltip()
{
	std::string tooltip;
	std::vector<Order::WeakPtr> active_orders;
	for(std::vector<Order::WeakPtr>::iterator itr = all_orders.begin(); itr != all_orders.end(); )
	{
		Order::HardPtr order = (*itr).lock();
		if( order ) {
			if(order->OnScreen() && order->IsActive() && !order->GetTooltip().empty())
			{
				Game::DestroyChipsOrder *order_destroy = (Game::DestroyChipsOrder*)order.get();
				//Тултип показываем не сразу. Возможно есть более приоритетный. Фишки из текущей выделяемой цепочки почетней.
				if(order_destroy && order_destroy->GetChipColor() == GameField::Get()->_chipSeqColor)
				{
					return order->GetTooltip();
				}
				active_orders.push_back(order);				
			}
			++itr;
		} else {
			itr = all_orders.erase(itr);
		}
	}
	if(!active_orders.empty() && active_orders.front().lock())
	{
		tooltip = active_orders.front().lock()->GetTooltip();
	}
	return tooltip;
}


bool CellIsOrdered(Game::FieldAddress cell)
{
	for(std::vector<Order::WeakPtr>::iterator itr = all_orders.begin(); itr != all_orders.end(); )
	{
		Order::HardPtr order = (*itr).lock();
		if( order ) {
			if(order->GetType() == Game::Order::FILL_ENERGY)
			{
				Game::FillEnergyOrder *ord = (Game::FillEnergyOrder*)order.get();
				if( ord->ContainsCell(cell) )
					return true;
			}
			++itr;
		} else {
			itr = all_orders.erase(itr);
		}
	}
	return false;
}

Game::FieldAddress ChipIsOrdered(int color, int sequenceLength)
{
	for(std::vector<Order::WeakPtr>::iterator itr = all_orders.begin(); itr != all_orders.end(); )
	{
		Order::HardPtr order = (*itr).lock();
		if( order ) {
			if(order->GetType() == Game::Order::KILL_CELL && order->OnScreen() && order->IsActive() && !order->Completed())
			{
				Game::DestroyChipsOrder *ord = (Game::DestroyChipsOrder*)order.get();
				if( ord->GetChipColor() == color && !ord->NeedResetAfterMove())
					return Game::FieldAddress(ord->GetAddress());
			}
			++itr;
		} else {
			itr = all_orders.erase(itr);
		}
	}
	return Game::FieldAddress();
}

DestroyChipsOrder* GetDiamondsOrderOnScreen()
{
	for(std::vector<Order::WeakPtr>::iterator itr = all_orders.begin(); itr != all_orders.end(); )
	{
		Order::HardPtr order = (*itr).lock();
		if( order ) {
			if(order->GetType() == Game::Order::KILL_CELL && order->OnScreen() && order->IsActive() && !order->Completed())
			{
				DestroyChipsOrder *ord = (DestroyChipsOrder*)order.get();
				if( ord->GetChipColor() == Order::DIAMOND )
					return ord;
			}
			++itr;
		} else {
			itr = all_orders.erase(itr);
		}
	}
	return nullptr;
}

} // end of namespace

} // end of namespace


OrderConfigWidget::OrderConfigWidget(std::string name, rapidxml::xml_node<>* xmlElement)
	: GUI::Widget(name, xmlElement)
	, _chipColor(0)
{
	chipTex = Core::resourceManager.Get<Render::Texture>("Chips");
}

bool OrderConfigWidget::MouseDown(const IPoint &mouse_pos)
{
	return false;
}

void OrderConfigWidget::MouseDoubleClick(const IPoint &mouse_pos)
{
}

void OrderConfigWidget::MouseUp(const IPoint &mouse_pos)
{
}

void OrderConfigWidget::MouseMove(const IPoint &mouse_pos)
{
}

void OrderConfigWidget::MouseWheel(int delta)
{
	int d = (delta >= 0) ? 1 : -1;
	_chipColor = (_chipColor + delta + 20) % 20;
	while( Gadgets::levelColors.GetIndex(_chipColor) < 0)
		_chipColor = (_chipColor + d + 20) % 20;
}

void OrderConfigWidget::Draw()
{
	if( orderType == "sequence" || orderType == "chips" )
	{
		FRect uv = Game::GetChipRect(_chipColor, false, false, false);
		IRect rect = Game::ChipColor::DRAW_RECT.MovedTo(getPosition());

		chipTex->Draw(rect, uv);
	}
}

void OrderConfigWidget::Update(float dt)
{
}

void OrderConfigWidget::AcceptMessage(const Message &message)
{
	if( message.is("SetType") )
	{
		orderType = message.getData();
		if( orderType == "sequence" || orderType == "chips" )
		{
			while( Gadgets::levelColors.GetIndex(_chipColor) < 0)
			_chipColor = (_chipColor + 1) % 20;
		}
	}
	else if( message.is("SetColor") )
	{
		_chipColor = message.getIntegerParam();
	}
}

Message OrderConfigWidget::QueryState(const Message& message) const
{
	if( message.is("PlaceOrder") )
	{
		Message msg("PlaceOrder", orderType);
		if( orderType == "sequence" || orderType == "chips" )
		{
			msg.variables.setInt("color", _chipColor);
		}
		return msg;
	}
	else
	{
		return Message();
	}
}