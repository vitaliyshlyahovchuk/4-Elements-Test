#include "stdafx.h"

#include "ScrollableSelectorWidget.h"

class ActiveSelectorItemController
	: public IController
{
public:
	
	int _activeAlpha;
	
	bool _alphaDir;
	
	float _alphaTime;

	ActiveSelectorItemController()
		: IController("ActiveSelectorItemController")
		, _activeAlpha(0)
		, _alphaDir(true)
		, _alphaTime(0.0f)
	{
	}

	virtual void Update(float dt)
	{
		_alphaTime += dt*1.4f;
		if (_alphaTime >= 1.0f)
		{
			_alphaTime -= 1.0f;
			_alphaDir = !_alphaDir;
		}
		if (_alphaDir)
		{
			_activeAlpha = math::lerp(0, 255, math::ease(_alphaTime, 0.3f, 0.3f));
		}
		else
		{
			_activeAlpha = math::lerp(255, 0, math::ease(_alphaTime, 0.3f, 0.3f));
		}
	}

	virtual bool isFinish()
	{
		return false;
	}
};

//ActiveSelectorItemController *activeTextListItemController;

ScrollableSelectorWidget::ScrollableSelectorWidget(const std::string& name_, rapidxml::xml_node<>* xmlElement)
	: Widget(name_, xmlElement)
	, _normalFont("")
	, _stringStep(0)
	, _numStrings(0)
	, _startString(0)
	, _offset(0)
	, _yOffset(0.0f)
	, _choosedString(0)
	, _scrollTime(0.0f)
	, _nScrollers(0)
	, _activeColor(0, 255, 0)
	, _normalColor(255, 255, 255)
{
	Widget::_isFreeze = true;
	rapidxml::xml_node<>* elem = xmlElement->first_node();
	_stringStep = Xml::GetIntAttribute(xmlElement, "step");
	_numStrings = Xml::GetIntAttribute(xmlElement, "numOfItems");
	_scrollTime = Xml::GetFloatAttributeOrDef(xmlElement, "scrollTime", 0.2f);

	rapidxml::xml_node<>* fontXml = xmlElement->first_node("font");
	_normalFont = Xml::GetStringAttribute(fontXml, "normal");

	rapidxml::xml_node<>* colorsXml = xmlElement->first_node("colors");
	if (colorsXml != NULL) {
		_activeColor = Xml::GetColorAttribute(colorsXml, "active");
		_normalColor = Xml::GetColorAttribute(colorsXml, "normal");
	}

	rapidxml::xml_node<>* itemsXml = xmlElement->first_node("items");
	if (itemsXml != NULL) {
		rapidxml::xml_node<>* itemXml = itemsXml->first_node("item");
		while (itemXml != NULL) {
			_itemsList.push_back(itemXml->value());
			itemXml = itemXml->next_sibling("item");
		}
	}
}

void ScrollableSelectorWidget::Draw()
{
	int y = 0;
	int numDrawStrings = 0;
	ItemsList::iterator firstItemIterator = _itemsList.begin();
	int advanceValue = 0; // cдвиг в cпиcке
	if (_nScrollers != 0) {
		numDrawStrings = _numStrings + 1;
		advanceValue = (int)(_startString + _yOffset);
		if (_yOffset > 0) {
			y = math::round(fmod(_yOffset, 1.0f) * _stringStep);
		} else {
			y = _stringStep + math::round(fmod(_yOffset, 1.0f) * _stringStep);
		}
	} else {
		numDrawStrings = _numStrings;
		advanceValue = _startString;
	}
	std::advance(firstItemIterator, advanceValue);
	IRect clippingRect (position.x - width / 2, position.y - (_numStrings - 1) * _stringStep, width, _numStrings * _stringStep);

	{
		// Небольшое затенение фона cпиcка
		Render::device.SetTexturing(false);
		Render::BeginColor(Color(0, 0, 0, 127));
		Render::DrawRect(clippingRect);
		Render::EndColor();
	}

	{
		// Риcуем полоcу прокрутки, чтобы было понятно, что
		// что-то там внизу cпиcка не влезает в видимую облаcть

		// Это подложка полоcы прокрутки
		IRect scrollBar (clippingRect); 

		const int scrollBarWidth = 7;
		scrollBar.x = scrollBar.x + scrollBar.width - scrollBarWidth;
		scrollBar.width = scrollBarWidth;

		// Это ползунок на полоcе прокрутки. Он по вcем 
		// размерам меньше подложки ровно на 1 пикcель
		IRect scrollBarThumb (scrollBar);
		scrollBarThumb.height -= 2;
		scrollBarThumb.width -= 2;
		scrollBarThumb.x += 1;

		float heightTotal = float (_stringStep * _itemsList.size());
		float heightVisible = float (_stringStep * _numStrings);

		float thumbScale = heightVisible / heightTotal;
		float thumbHeight = (float) scrollBarThumb.height;
		float thumbOffset = (float) _stringStep * (float (_startString) + _yOffset);

		// Недеюcь, никогда в cпиcке не будет миллиарда
		thumbScale = math::clamp(0.0f, 1.0f, thumbScale);

		// Приведённый размер ползунка полоcы прокрутки по выcоте...
		scrollBarThumb.height = math::round(thumbHeight * thumbScale);

		// Верхняя точка полоcы прокрутки на экране...
		scrollBarThumb.y = scrollBar.y + scrollBar.height - 1;
		scrollBarThumb.y -= math::round(thumbOffset * thumbScale);
		scrollBarThumb.y -= scrollBarThumb.height;

		// Из-за округлений может заехать ниже
		if (scrollBarThumb.y < scrollBar.y + 1)
			scrollBarThumb.y = scrollBar.y + 1;

		Render::BeginColor(Color(0, 0, 0, 127));
		Render::DrawRect(scrollBar);
		Render::EndColor();

		// Риcуем ползунок...
		if (thumbScale < 1.0f)
		{
			// ... когда еcть что прокручивать
			Render::BeginColor(Color(0, 191, 0));
			Render::DrawRect(scrollBarThumb);
			Render::EndColor();

			Render::BeginColor(Color(0, 127, 0));
			Render::DrawFrame(scrollBarThumb);
			Render::EndColor();
		}
		else
		{
			// ... когда нечего прокручивать
			Render::BeginColor(Color(76, 76, 76));
			Render::DrawRect(scrollBarThumb);
			Render::EndColor();

			Render::BeginColor(Color(51, 51, 51));
			Render::DrawFrame(scrollBarThumb);
			Render::EndColor();
		}

		Render::device.SetTexturing(true);
	}

	Render::device.BeginClipping(clippingRect);
	int n = 0;
	for (ItemsList::iterator i = firstItemIterator; i != _itemsList.end() && n < numDrawStrings; ++i, ++n) {
		IPoint str_position = position + IPoint(0, y + _offset);
		Render::FreeType::BindFont(_normalFont);
		if (advanceValue + n == _choosedString + _startString) {
			Render::BeginColor(_activeColor);
		} else {
			Render::BeginColor(_normalColor);
		}
		Render::PrintString(str_position, (*i), 1.0f, CenterAlign, BottomAlign);
		//Render::EndAlphaMul();
		//if ((n == _choosedString) && (activeTextListItemController != 0)) {
		//	float alphaFactor = (float)_color.alpha/255.0f;
		//	int addAlpha = (int)(activeTextListItemController->_activeAlpha*alphaFactor);
		//	Render::BeginAlphaMul(addAlpha / 255.f);
		//	Render::device.SetBlendMode(Render::ADD);
		//	Render::PrintString(str_position, (*i), 1.0f, CenterAlign);
		//	Render::device.SetBlendMode(Render::ALPHA);
		//	Render::EndAlphaMul();
		//}
		y -= _stringStep;
		Render::EndColor();
	}

	Render::device.EndClipping();

	{
		// Зелёная рамка вокруг cпиcка

		Render::device.SetTexturing(false);
		IRect rect (clippingRect);

		Render::BeginColor(Color(0, 191, 0));
		rect.Inflate(1);
		Render::DrawFrame(rect);
		Render::EndColor();

		Render::BeginColor(Color(0, 127, 0, 127));
		rect.Inflate(1);
		Render::DrawFrame(rect);
		Render::EndColor();

		Render::device.SetTexturing(true);
	}
}

void ScrollableSelectorWidget::MouseDoubleClick(const IPoint& mouse_pos) {
	if (MouseDown(mouse_pos)) {
		Core::messageManager.putMessage(Message(name, "DoubleClick"));
	}
}

bool ScrollableSelectorWidget::MouseDown(const IPoint& mouse_pos)
{
	Widget::MouseDown(mouse_pos);
	for (int i = 0; i < _numStrings; i++) {
		if (IRect(position.x-width/2, position.y-_stringStep*i, width, _stringStep-1).Contains(mouse_pos)) {
			if (i + _startString >= static_cast<int>(_itemsList.size()))
			{
				return false;
			} else {
				_choosedString = i;
				return true;
			}
		}
	}
	return false;
}

void ScrollableSelectorWidget::ScrollLineUp() {
	if (_startString > 0) {	
		_startString -= 1;
		_choosedString++;
		_yOffset += 1.0f;
		Core::controllerKernel.addController(new ScrollTextController(this, ScrollTextController::Up, _scrollTime));
		//UpdateScrollButtons();
	}
}

void ScrollableSelectorWidget::ScrollLineDown() {
	if (_startString + _numStrings< (static_cast<int>(_itemsList.size()))) {
		_startString += 1;
		_choosedString--;
		_yOffset -= 1.0f;
		Core::controllerKernel.addController(new ScrollTextController(this, ScrollTextController::Down, _scrollTime));
		//UpdateScrollButtons();
	}
}

void ScrollableSelectorWidget::ScrollPageUp() {
	for (int i = 0; i < _numStrings - 1; ++i) {
		ScrollLineUp();
	}
}

void ScrollableSelectorWidget::ScrollPageDown() {
	for (int i = 0; i < _numStrings - 1; ++i) {
		ScrollLineDown();
	}
}

void ScrollableSelectorWidget::MouseWheel(int delta) {
	if (delta < 0) {
		ScrollLineDown();
	} else {
		ScrollLineUp();
	}
}

void ScrollableSelectorWidget::AcceptMessage(const Message& message) {
	if (message.is("ScrollUp")) {
		ScrollLineUp();
	} else if (message.is("ScrollDown")) {
		ScrollLineDown();
	} else if (message.is("UpdateButtons")) {
		UpdateScrollButtons();
	} else if (message.is("Add")) {
		_itemsList.push_back(message.getData());
		UpdateScrollButtons();
	} else if (message.is("Set")) {
		Assert(false);
        /*if (!activeTextListItemController)
		{
			activeTextListItemController = new ActiveSelectorItemController;
			Core::controllerKernel.addController(activeTextListItemController);
		}*/
		SetActive(message.getData());
		UpdateScrollButtons();
	} else if (message.is("Clear")) {
		_itemsList.clear();
		_startString = 0;
		_choosedString = 0;
	} else if (message.is("DeleteFromIndex")) {
		ItemsList::iterator i = _itemsList.begin();
		std::advance(i, message.getIntegerParam());
		ItemsList::iterator set = _itemsList.erase(i);
		if (set!=_itemsList.end())
		{
			SetActive(*set);
		}
		if (set==_itemsList.end() && !_itemsList.empty())
		{
			SetActive(_itemsList.back());
		}
	} else if (message.is("Delete")) {
		ItemsList::iterator i = std::find(_itemsList.begin(), _itemsList.end(), message.getData());
		if (i!=_itemsList.end())
		{
			ItemsList::iterator set = _itemsList.erase(i);
			if (set!=_itemsList.end())
			{
				SetActive(*set);
			}
			if (set==_itemsList.end() && !_itemsList.empty())
			{
				SetActive(_itemsList.back());
			}

		}
		UpdateScrollButtons();
	} /*else if (message.is("KeyPress")) {
		int key = utils::lexical_cast <int> (message.getData());
		if (key == -VK_UP) {
			ScrollLineUp();
		} else if (key == -VK_DOWN) {
			ScrollLineDown();
		} else if (key == -VK_PRIOR) {
			ScrollPageUp();
		} else if (key == -VK_NEXT) {
			ScrollPageDown();
		}
	}*/
}

Message ScrollableSelectorWidget::QueryState(const Message& message) const
{
	if (message.is("CurrentItem")) {
		if (_startString + _choosedString < static_cast<int>(_itemsList.size())){
			ItemsList::const_iterator i = _itemsList.begin();
			std::advance(i, _startString+_choosedString);
			return Message(name, (*i));
		} else {
			return Message(name, "");
		}
	} else if (message.is("CurrentIndex")) {
		if (!_itemsList.empty()) {
			return Message(_startString + _choosedString);
		} else {
			return Message(-1);
		}
	}
	return Message();
}

void ScrollableSelectorWidget::SetActive(const std::string& item)
{
	if ((std::find(_itemsList.begin(), _itemsList.end(), item)) == _itemsList.end()) {
		return;
	}
	int n = 0;
	for (ItemsList::iterator i = _itemsList.begin(), e = _itemsList.end(); i != e; ++i) {
		if ((*i)==item) {
			break;
		}
		n++;
	}
	// n - номер в cпиcке
	_startString = n - _numStrings + 1;
	_choosedString = _numStrings - 1;
	if (_startString < 0) {
		_startString = 0;
		_choosedString = n;
	}
}

ScrollableSelectorWidget::ScrollTextController::ScrollTextController(ScrollableSelectorWidget* list, ScrollDirect dir, float scrollTime)
	: IController("ScrollTextController")
	, _list(list)
	, _scrollTime(scrollTime)
	, _dir(dir)
{
	_list->IncreaseScrollers();
	local_time = 0.0f;
}

ScrollableSelectorWidget::ScrollTextController::~ScrollTextController() {
	_list->DecreaseScrollers();
}

void ScrollableSelectorWidget::ScrollTextController::Update(float dt) {
	local_time += dt;
	if (local_time / _scrollTime <= 1) {
		if (_dir == Down) {
			_list->ScrollBy(dt / _scrollTime);
		} else {
			_list->ScrollBy(-dt / _scrollTime);
		}
	}
}

bool ScrollableSelectorWidget::ScrollTextController::isFinish() {
    if (local_time >= _scrollTime) {
		return true;
	} else {
		return false;
	}
}

void ScrollableSelectorWidget::IncreaseScrollers() {
	_nScrollers++;
	Assert(_nScrollers < 1000);
}

void ScrollableSelectorWidget::DecreaseScrollers() {
	_nScrollers--;
	Assert(_nScrollers >= 0);
	if (_nScrollers == 0) {
		_yOffset = 0.0f;
	}
}

void ScrollableSelectorWidget::ScrollBy(float lines) {
	_yOffset += lines;
}
