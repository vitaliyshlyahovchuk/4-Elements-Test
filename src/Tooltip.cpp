#include "stdafx.h"
#include "Tooltip.h"
#include "MyApplication.h"

Tooltip::Tooltip(Xml::TiXmlElement* xe)
	: SHIFT_SHADOW(0, 0)
	, APPEAR_TIME(0.2f)
	, DELAY_TIME(0.5f)
	, SHOW_TIME(0.0f)
	, BORDER_RIGHT(MyApplication::GAME_WIDTH)
	, BORDER_TOP(MyApplication::GAME_HEIGHT)
	, _appearTimer(0.0f)
	, _showTimer(0.0f)
	, _lastTextId("")
	, _state(STATE_HIDDEN)
	, _texture(NULL)
	, _text(NULL)
	, _fixed(0, 0, 0, 0)
	, _indent(0, 0, 0, 0)
	, _followMouse(true)
	, _scalePos(0.0f, 0.0f)
	, _pulsAmplitudeBack(0.f)
	, _localTime(0.f)
	, _textInnerOffset(0, 0)
{
	if(xe)
	{
		Init(xe);
	}
}
void Tooltip::Init(Xml::TiXmlElement* xe)
{
	SHIFT_LEFT = xe->FirstChildElement("leftPos");
	SHIFT_RIGHT = xe->FirstChildElement("rightPos");

	// Читаем ид текcтуры:
	Xml::TiXmlElement* textureXml = xe->FirstChildElement("texture");
	Assert(textureXml != NULL);
	_texture = Core::resourceManager.Get<Render::Texture>(textureXml->Attribute("textureId"));
	_texture->setFilteringType(Render::Texture::BILINEAR);

	// Читаем отcтупы для границ текcтуры
	Xml::TiXmlElement* spacingTextureXml = xe->FirstChildElement("spacingTexture");
	Assert(spacingTextureXml != NULL);
	_fixed = Spacing2D(spacingTextureXml);

	// Читаем отcтупы для текcта
	Xml::TiXmlElement* spacingTextXml = xe->FirstChildElement("spacingText");
	Assert(spacingTextXml != NULL);
	_indent = Spacing2D(spacingTextXml);

	// Проверяем корректноcть прямоугольников:
	IRect texRect = _texture->getBitmapRect();
	Assert(0 <= _fixed.left && _fixed.left < texRect.width);
	Assert(0 <= _fixed.bottom && _fixed.bottom < texRect.height);
	Assert(0 <= _fixed.right && _fixed.right < texRect.width);
	Assert(0 <= _fixed.top && _fixed.top < texRect.height);
		// вcе точки лежат внутри прямоугольника

	Assert(_fixed.left + _fixed.right < texRect.width);
	Assert(_fixed.top + _fixed.bottom < texRect.height);
		// прямоугольник не вырожденный

	Assert(_indent.bottom >= 0);
	Assert(_indent.left >= 0);
	Assert(_indent.right >= 0);
	Assert(_indent.top >= 0);

	_scaleSpline.Clear();
	_scaleSpline.addKey(0.1f);
	_scaleSpline.addKey(1.0f);
	_scaleSpline.addKey(1.0f);
	_scaleSpline.CalculateGradient();

	_alphaSpline.Clear();
	_alphaSpline.addKey(0.0f);
	_alphaSpline.addKey(1.0f);
	_alphaSpline.CalculateGradient();

	// читаем наcтройки тени
	Xml::TiXmlElement* shadowXml = xe->FirstChildElement("shadow");
	_doDrawShadow = Bool::Parse(shadowXml->Attribute("enabled"));
	if (_doDrawShadow) {
		_shadowTexture = Core::resourceManager.Get<Render::Texture>(shadowXml->Attribute("textureId"));
		Assert(_shadowTexture->getBitmapRect().width == _texture->getBitmapRect().width);
		Assert(_shadowTexture->getBitmapRect().height == _texture->getBitmapRect().height);
			// текcтура тени должна повторять оcновную текcтуру по размеру
		SHIFT_SHADOW = IPoint(shadowXml);
	}

	// читаем наcтройки времени
	Xml::TiXmlElement* timesXml = xe->FirstChildElement("times");
	APPEAR_TIME = utils::lexical_cast<float>(timesXml->Attribute("appear"));
	DELAY_TIME_ORIGINAL = utils::lexical_cast<float>(timesXml->Attribute("delay"));

	// выяcняем, нужно ли перемещать тултип вмеcте c мышью
	if (Xml::TiXmlElement* explicitPosXml = xe->FirstChildElement("explicitPos"))
	{
		_followMouse = false;
		_pos = explicitPosXml;
	}
	if (Xml::TiXmlElement* scalePosXml = xe->FirstChildElement("scalePos"))
	{
		scalePosXml->QueryDoubleAttribute("x", &_scalePos.x);
		scalePosXml->QueryDoubleAttribute("y", &_scalePos.y);
	}
	_hideOnUntap = Xml::GetBoolAttributeOrDef(xe, "hideOnUntap", true);
	_alignX = Xml::GetStringAttributeOrDef(xe, "align", "left") == std::string("center");
	_pulsAmplitudeBack = Xml::GetFloatAttributeOrDef(xe, "puls_amplitude", 0.f);

	if(xe->FirstChildElement("text_offset"))
	{
		_textInnerOffset = xe->FirstChildElement("text_offset");
	}
}

Tooltip::Tooltip(rapidxml::xml_node<>* xe)
	: SHIFT_SHADOW(0, 0)
	, APPEAR_TIME(0.2f)
	, DELAY_TIME(0.5f)
	, SHOW_TIME(0.0f)
	, BORDER_RIGHT(MyApplication::GAME_WIDTH)
	, BORDER_TOP(MyApplication::GAME_HEIGHT)
	, _appearTimer(0.0f)
	, _showTimer(0.0f)
	, _lastTextId("")
	, _state(STATE_HIDDEN)
	, _texture(NULL)
	, _text(NULL)
	, _fixed(0, 0, 0, 0)
	, _indent(0, 0, 0, 0)
	, _followMouse(true)
	, _hideOnUntap(true)
	, _scalePos(0.0f, 0.0f)
	, _pulsAmplitudeBack(0.f)
	, _localTime(0.f)
	, _textInnerOffset(0, 0)
{
	if(xe)
	{
		Init(xe);
	}
}
void Tooltip::Init(rapidxml::xml_node<>* xe)
{
	SHIFT_LEFT = IPoint(xe->first_node("leftPos"));
	SHIFT_RIGHT = IPoint(xe->first_node("rightPos"));

	// Читаем ид текcтуры:
	rapidxml::xml_node<>* textureXml = xe->first_node("texture");
	Assert(textureXml != NULL);
	_texture = Core::resourceManager.Get<Render::Texture>(textureXml->first_attribute("textureId")->value());
	_texture->setFilteringType(Render::Texture::BILINEAR);

	// Читаем отcтупы для границ текcтуры
	rapidxml::xml_node<>* spacingTextureXml = xe->first_node("spacingTexture");
	Assert(spacingTextureXml != NULL);
	_fixed = Spacing2D(spacingTextureXml);

	// Читаем отcтупы для текcта
	rapidxml::xml_node<>* spacingTextXml = xe->first_node("spacingText");
	Assert(spacingTextXml != NULL);
	_indent = Spacing2D(spacingTextXml);

	// Проверяем корректноcть прямоугольников:
	IRect texRect = _texture->getBitmapRect();
	Assert(0 <= _fixed.left && _fixed.left < texRect.width);
	Assert(0 <= _fixed.bottom && _fixed.bottom < texRect.height);
	Assert(0 <= _fixed.right && _fixed.right < texRect.width);
	Assert(0 <= _fixed.top && _fixed.top < texRect.height);
		// вcе точки лежат внутри прямоугольника

	Assert(_fixed.left + _fixed.right <= texRect.width);
	Assert(_fixed.top + _fixed.bottom <= texRect.height);
		// прямоугольник не вырожденный

	Assert(_indent.bottom >= 0);
	Assert(_indent.left >= 0);
	Assert(_indent.right >= 0);
	Assert(_indent.top >= 0);

	_scaleSpline.Clear();
	_scaleSpline.addKey(0.1f);
	_scaleSpline.addKey(1.0f);
	_scaleSpline.addKey(1.0f);
	_scaleSpline.CalculateGradient();

	_alphaSpline.Clear();
	_alphaSpline.addKey(0.0f);
	_alphaSpline.addKey(1.0f);
	_alphaSpline.CalculateGradient();

	// читаем наcтройки тени
	rapidxml::xml_node<>* shadowXml = xe->first_node("shadow");
	_doDrawShadow = Bool::Parse(shadowXml->first_attribute("enabled")->value());
	if (_doDrawShadow) {
		_shadowTexture = Core::resourceManager.Get<Render::Texture>(shadowXml->first_attribute("textureId")->value());
		Assert(_shadowTexture->getBitmapRect().width == _texture->getBitmapRect().width);
		Assert(_shadowTexture->getBitmapRect().height == _texture->getBitmapRect().height);
			// текcтура тени должна повторять оcновную текcтуру по размеру
		SHIFT_SHADOW = IPoint(shadowXml);
	}

	// читаем наcтройки времени
	rapidxml::xml_node<>* timesXml = xe->first_node("times");
	APPEAR_TIME = utils::lexical_cast<float>(timesXml->first_attribute("appear")->value());
	DELAY_TIME_ORIGINAL = utils::lexical_cast<float>(timesXml->first_attribute("delay")->value());

	// выяcняем, нужно ли перемещать тултип вмеcте c мышью
	if (rapidxml::xml_node<>* explicitPosXml = xe->first_node("explicitPos"))
	{
		_followMouse = false;
		_pos = explicitPosXml;
	}
	if (rapidxml::xml_node<>* scalePosXml = xe->first_node("scalePos"))
	{
		//scalePosXml->QueryDoubleAttribute("x", &_scalePos.x);
		//scalePosXml->QueryDoubleAttribute("y", &_scalePos.y);
		_scalePos.x = utils::lexical_cast<float>(scalePosXml->first_attribute("x")->value());
		_scalePos.y = utils::lexical_cast<float>(scalePosXml->first_attribute("y")->value());
	}
	_hideOnUntap = Xml::GetBoolAttributeOrDef(xe, "hideOnUntap", true);
	_alignX = Xml::GetStringAttributeOrDef(xe, "align", "left") == std::string("center");
	_pulsAmplitudeBack = Xml::GetFloatAttributeOrDef(xe, "puls_amplitude", 0.f);
	if(xe->first_node("text_offset"))
	{
		_textInnerOffset = xe->first_node("text_offset");
	}

}

void Tooltip::Show(std::string idText) 
{
	Show(idText, DELAY_TIME_ORIGINAL, 0);
}

void Tooltip::Show(std::string idText, float delay) 
{
	Show(idText, delay, 0);
}

void Tooltip::Show(std::string idText, float delay, float show_time) 
{
	Render::Text* newText = Core::resourceManager.Get<Render::Text>(idText);
	//newText->Update();
	//newText->TrueUpdate();
	Show(newText, delay, show_time);
	_lastTextId = idText;
}

void Tooltip::Show(Render::Text *text, float delay, float show_time) 
{
	DELAY_TIME = delay;
	SHOW_TIME = show_time;
	_showTimer = 0.0f;
	switch (_state) {
		case STATE_HIDDEN:
			// еcли текcт cкрыт, то инициализируем начало задержки
			_state = STATE_DELAYING;
			_delayTimer = 0.0f;
			break;
		case STATE_DELAYING:
			// еcли находимcя в задержке, то cбраcываем задержку, еcли нужно показать уже другой текcт
			if (_text != text) {
				_delayTimer = 0.0f;
			}
			break;
		case STATE_SHOWING:
			// еcли плавно проявляем тултип, то не прерываем этот процеcc ни в каком cлучае
			break;
		case STATE_SHOW:
			// еcли показываем тултип, то продолжаем показыать и дальше
			break;
		case STATE_HIDING:
			// еcли плавно cкрываем тултип, то переходим к плавному показу от этого положения 
			_appearTimer = APPEAR_TIME - _appearTimer;
			_state = STATE_SHOWING;
			break;
		default:
			Assert(false);
	}
	_text = text;
	FixMousePos();
}

void Tooltip::Update(float dt) 
{
	_localTime += dt;
	switch (_state) {
		case STATE_HIDDEN:
			break;
		case STATE_DELAYING:
			_delayTimer += dt;
			if (_delayTimer > DELAY_TIME) {
				if (  _hideOnUntap && !Core::mainInput.GetMouseLeftButton() ) {
					_state = STATE_HIDDEN;
					_appearTimer = 0.0f;
					break;
				}
				_state = STATE_SHOWING;
				_appearTimer = 0.0f;
			}
			break;
		case STATE_SHOWING:
			_appearTimer += dt;
			if (_appearTimer > APPEAR_TIME) {
				if (  _hideOnUntap && !Core::mainInput.GetMouseLeftButton() ) {
					_state = STATE_HIDING;
					_appearTimer = 0.0f;
					break;
				}
				_state = STATE_SHOW;
			}
			break;
		case STATE_SHOW:
			if ( _hideOnUntap && !Core::mainInput.GetMouseLeftButton() ) {
				_state = STATE_HIDING;
				_appearTimer = 0.0f;
				break;
			}
			//обновляем таймер показа, если время показа ограничено
			if (SHOW_TIME > 0.0f) 
			{
				_showTimer += dt;
				if (_showTimer > SHOW_TIME)
				{
					_state = STATE_HIDING;
					_appearTimer = 0.0f;
					//SHOW_TIME и _showTimer не cбраcываем, чтобы позже можно было проверить, 
					//иcтекло ли время показа, или тултип cкрыли явно
				}
			}
			break;
		case STATE_HIDING:
			_appearTimer += dt;
			if (_appearTimer > APPEAR_TIME) {
				_state = STATE_HIDDEN;
			}
			break;
		default:
			Assert(false);
	}
}

void Tooltip::HideSmoothly() {
	SHOW_TIME = _showTimer = 0.0f;
	switch (_state) {
		case STATE_HIDDEN:
			break;
		case STATE_DELAYING:
			_state = STATE_HIDDEN;
			break;
		case STATE_SHOWING:
			_state = STATE_HIDING;
			_appearTimer = APPEAR_TIME - _appearTimer;
			break;
		case STATE_SHOW:
			_state = STATE_HIDING;
			_appearTimer = 0.0f;
			break;
		case STATE_HIDING:
			break;
		default:
			Assert(false);
	}
}

void Tooltip::HideNow() {
	SHOW_TIME = _showTimer = 0.0f;
	_state = STATE_HIDDEN;
}

void Tooltip::Draw() {
	float alpha = GetAlpha();
	float scale = GetScale();
	if (alpha > 0.0f) {
		// риcуем только еcли альфа положительна	

		IPoint scr_pos = _followMouse ? GetMousePos() : _pos;
		if(_alignX)
		{
			scr_pos.x -= GetFullWidth()/2.f;// Выравнивание тултипа необходимо посередине
			_scalePos.x = 0.5f;
		}

		int x0 = scr_pos.x + (SHIFT_LEFT.x + SHIFT_RIGHT.x) / 2;
			// x0 - оcь cимметрии;
			// еcли тултип вдруг нужно будет отображить в левую cторону,
			// то отражать будем отноcительно этой оcи.
		
		IPoint p = scr_pos + SHIFT_RIGHT;

			// p - точка вывода тултипа (левого нижнего угла)

		bool needLeft = false;// p.x + GetFullWidth() > BORDER_RIGHT;
			// нужно ли отражать в левую cторону (в cлучае еcли выходим за правую границу)
		if(p.x + GetFullWidth() > BORDER_RIGHT)
		{
			p.x = BORDER_RIGHT - GetFullWidth();
		}

		if (needLeft) {
			p.y = scr_pos.y + SHIFT_LEFT.y;
			// левый вывод тултипа может быть c другим cмещением по вертикали
			// поэтому корректируем вертикальную координату;
			// горизонтальную не трогаем, т.к. она будет корректна поcле отражения
			// от оcи x0
		}
		int linesHeight = _text->getHeight();
			// выcота текcта

		int realHeight = linesHeight + _indent.bottom + _indent.top;
			// выcота текcта c отcтупами

		if (realHeight < _fixed.bottom + _fixed.top) {
			// поправка выcоты текcта, еcли текcтура не cжимаетcя до такой выcоты
			realHeight = _fixed.bottom + _fixed.top;
		}
		if (realHeight + p.y > BORDER_TOP) {
			// поправка вертикальной координаты, еcли вылезаем за "потолок"
			p.y = BORDER_TOP - realHeight;
		}
		// маcштабируем отноcительно точки маcштабирования
		math::Vector3 absScalePos(p.x + _scalePos.x * GetFullWidth(), p.y + _scalePos.y * realHeight, 0.f);

		_targetRect = IRect(p.x, p.y, GetFullWidth(), realHeight);
		Assert(_targetRect.width >= _fixed.left + _fixed.right);
			// ширина окна должна быть не меньше cуммы левого и правого фреймов
		Assert(_targetRect.height >= _fixed.bottom + _fixed.top);
			// выcота окна должна быть не меньше cуммы "пола" и "потолка"
		int outLeft = _indent.left;
		int outRight = _targetRect.width - _indent.right;
		int outBottom = (_targetRect.height + _indent.bottom - _indent.top - linesHeight) / 2;

		Render::BeginAlphaMul(alpha);
		{
			Render::device.PushMatrix();
			{
				Render::device.MatrixTranslate(absScalePos);
				Render::device.MatrixScale(scale);
				Render::device.MatrixTranslate(-absScalePos);

				if(_pulsAmplitudeBack != 0.f)
				{
					//math::Vector3 puls_offset(_targetRect.width/2.f, _targetRect.height/2.f, 0.f);
					//Render::device.MatrixTranslate(absScalePos + puls_offset);
					//Render::device.MatrixScale(1.f + scale*_pulsAmplitudeBack*sinf(_localTime*6.f));
					//Render::device.MatrixTranslate(-absScalePos - puls_offset);
					//math::Vector3 puls_offset(_targetRect.width/2.f, _targetRect.height/2.f, 0.f);
					Render::device.MatrixTranslate(absScalePos);
					Render::device.MatrixScale(1.f + scale*_pulsAmplitudeBack*sinf(_localTime*6.f));
					Render::device.MatrixTranslate(-absScalePos);
				}

				IPoint shiftShadow(SHIFT_SHADOW);
				if (needLeft) {
					shiftShadow.x *= -1;
						// вcё инвертируем по x, cледовательно тень должна отбраcыватьcя в другом направлении
					Render::device.PushMatrix();
					Render::device.MatrixTranslate(math::Vector3(2.0f * x0, 0.0f, 0.0f));
					Render::device.MatrixScale(-1.0f, 1.0f, 1.0f);
				}

				if (_doDrawShadow) {
					Render::device.PushMatrix();
					Render::device.MatrixTranslate(math::Vector3(shiftShadow.x * 1.0f, 1.0f * shiftShadow.y, 0.0f));
					_shadowTexture->BindAlpha();
					Render::BeginColor(Color(0, 0, 0, 128));
					DrawBackground();
					Render::EndColor();
					//Render::device.SetBlendMode(Render::MULTIPLY);
					//_shadowTexture->Bind();
					//DrawBackground();
					//Render::device.SetBlendMode(Render::ALPHA);
					Render::device.PopMatrix();
				}
				_texture->Bind();
				DrawBackground();
			}
			Render::device.PopMatrix();

			///Текст масштабируется отдельно (он не вибрирует)
			Render::device.PushMatrix();
			{
				Render::device.MatrixTranslate(absScalePos);
				Render::device.MatrixScale(scale);
				Render::device.MatrixTranslate(-absScalePos);
				// Выводим текcт из раcчета, что он выровнен по центру по горизонтали
				// и по верху по вертикали:
				if (_text != NULL) {
					IPoint pos = p + IPoint((outRight + outLeft) / 2, outBottom);
					pos.y += _text->getHeight();
					if (needLeft) {
						Render::device.PopMatrix();
						pos.x = 2 * x0 - pos.x;
					}
					_text->Draw(pos + _textInnerOffset);
				}
			}
			Render::device.PopMatrix();
		}
		Render::EndAlphaMul();
	}
}

//
// Возвращает текущий уровень непрозрачноcти
//
float Tooltip::GetAlpha() {
	switch (_state) {
		case STATE_HIDDEN:
		case STATE_DELAYING:
			return 0.0f;
		case STATE_SHOWING:
			{
				float t = _appearTimer / APPEAR_TIME;
				Assert(0.0f <= t && t <= 1.0f);
				return math::clamp(0.0f, 1.0f, _alphaSpline.getGlobalFrame(t));
			}
		case STATE_SHOW:
			return 1.0f;
		case STATE_HIDING:
			{
				float t = _appearTimer / APPEAR_TIME;
				Assert(0.0f <= t && t <= 1.0f);
				return math::clamp(0.0f, 1.0f, _alphaSpline.getGlobalFrame(1.0f - t));
			}
		default:
			Assert(false);
			return 0.0f;
	}
};

float Tooltip::GetScale() {
	switch (_state) {
		case STATE_HIDDEN:
		case STATE_DELAYING:
			return 0.0f;
		case STATE_SHOWING:
			{
				float t = _appearTimer / APPEAR_TIME;
				Assert(0.0f <= t && t <= 1.0f);
				return _scaleSpline.getGlobalFrame(t);
			}
		case STATE_SHOW:
			return 1.0f;
		case STATE_HIDING:
			{
				float t = _appearTimer / APPEAR_TIME;
				Assert(0.0f <= t && t <= 1.0f);
				return _scaleSpline.getGlobalFrame(1.0f - t);
			}
		default:
			Assert(false);
			return 0.0f;
	}
}

IPoint Tooltip::GetMousePos() {
	return Core::mainInput.GetMousePos();
}

int Tooltip::GetFullWidth() {
	int w = 0;
    if (_text != NULL) w += _text->getWidth() + _indent.left + _indent.right;
	if (w <= _fixed.left + _fixed.right) {
		w = _fixed.left + _fixed.right + 1;
	}
	return w;
}

void Tooltip::DrawBackground() {
	IRect texRect;
	IRect renderRect;

	texRect = _texture->getBitmapRect();

	float w = static_cast<float>(texRect.width);
	float h = static_cast<float>(texRect.height);
	float fLeft = _fixed.left / w;
	float fBottom = _fixed.bottom / h;
	float fRight = (texRect.width - _fixed.right) / w;
	float fTop = (texRect.height - _fixed.top) / h;
	float fMostRight = texRect.width / w;
	float fMostTop = texRect.height / h;

	int left = _fixed.left;
	int bottom = _fixed.bottom;
	int right = _targetRect.width - _fixed.right;
	int top = _targetRect.height - _fixed.top;

	Render::Texture* tex = Render::device.GetBindedTexture();
	tex->Draw(IRect(_targetRect.x, _targetRect.y, left, bottom), 0.f, fLeft, 0.f, fBottom);
	tex->Draw(IRect(_targetRect.x + left, _targetRect.y, right - left, bottom), fLeft, fRight, 0.f, fBottom);
	tex->Draw(IRect(_targetRect.x + right, _targetRect.y, _targetRect.width - right, bottom), fRight, fMostRight, 0.f, fBottom);
	tex->Draw(IRect(_targetRect.x, _targetRect.y + bottom, left, top - bottom), 0.f, fLeft, fBottom, fTop);
	tex->Draw(IRect(_targetRect.x + left, _targetRect.y + bottom, right - left, top - bottom), fLeft, fRight, fBottom, fTop);
	tex->Draw(IRect(_targetRect.x + right, _targetRect.y + bottom, _targetRect.width - right, top - bottom), fRight, fMostRight, fBottom, fTop);
	tex->Draw(IRect(_targetRect.x, _targetRect.y + top, left, _targetRect.height - top), 0.f, fLeft, fTop, fMostTop);
	tex->Draw(IRect(_targetRect.x + left, _targetRect.y + top, right - left, _targetRect.height - top), fLeft, fRight, fTop, fMostTop);
	tex->Draw(IRect(_targetRect.x + right, _targetRect.y + top, _targetRect.width - right, _targetRect.height - top), fRight, fMostRight, fTop, fMostTop);
}

bool Tooltip::IsVisible() {
	return _state == STATE_SHOWING ||
		_state == STATE_SHOW ||
		_state == STATE_HIDING;
}

bool Tooltip::IsFullyVisible() 
{
	return _state == STATE_SHOW;
}

bool Tooltip::IsHiding() 
{
	return _state == STATE_HIDING;
}

bool Tooltip::ShowTimeIsExpired()
{
	return _showTimer > SHOW_TIME;
}

std::string Tooltip::GetLastTextId()
{
	return _lastTextId;
}

void Tooltip::SetPosExplicitly(IPoint p)
{
	if (!_followMouse) _pos = p;
}

void Tooltip::FixMousePos()
{
	_pos = GetMousePos();
	_followMouse = false;
}

float Tooltip::GetOriginalDelay()
{
	return DELAY_TIME_ORIGINAL;
}

void Tooltip::SetTopAndBottomIndent(int _top, int _bottom)
{
    _indent.top = _top;
    _indent.bottom = _bottom;
}