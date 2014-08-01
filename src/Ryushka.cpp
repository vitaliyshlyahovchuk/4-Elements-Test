#include "stdafx.h"

#include "Ryushka.h"
#include "GameField.h"
#include "RyushkaGroup.h"
#include "MyApplication.h"
#include "RyushkaFaerie.h"
#include "Game.h"
#include "EditorUtils.h"

static void DrawTextureCentered(Render::Texture* texture, FPoint pos, bool isIntCenter = false)
{
	FPoint center = isIntCenter
		? IPoint(texture->getBitmapRect().width / 2, texture->getBitmapRect().height / 2)
		: FPoint(texture->getBitmapRect().width / 2.0f, texture->getBitmapRect().height / 2.0f);
	texture->Draw(pos - center);
}

const float Ryushka::SCALES[] = {0.25f, 0.5f, 0.75f, 1.0f, 1.25f, 1.5f};
int Ryushka::COUNT_ID = 0;
namespace  {
	
	//
	// Преобразование из cтроки в bool
	// TODO: заменить на Bool::Parse.
	//
	bool BoolParse(std::string  s) {
		if (s == "1" || s == "true") {
			return true;
		} else if (s == "0" || s == "false") {
			return false;
		} else {
			Assert(false);
			return false;
		}
	}
}

Ryushka::Ryushka(rapidxml::xml_node<>* xe, float time_scale_)
	: local_time(0.f)
	, time_scale(time_scale_)
	, _isSelected(false)
	, _visible(false)
	//, visible(false)
{
	//_field = GameField::Get();
	_zLevel = Xml::GetIntAttributeOrDef(xe, "zLevel", 1);
	_scale = Xml::GetFloatAttributeOrDef(xe, "scale", 1.0f);
	_angle = Xml::GetFloatAttributeOrDef(xe, "angle", 0.0f);
	_name = Xml::GetStringAttributeOrDef(xe, "name", "");
	_depth = Xml::GetFloatAttributeOrDef(xe, "depth", 0.0f);
	_xMirror = Xml::GetBoolAttributeOrDef(xe, "xmirror", false);
	_id = Xml::GetIntAttributeOrDef(xe, "id", 0);
	_id_connect = Xml::GetIntAttributeOrDef(xe, "id_con", 1);
	_radius = Xml::GetIntAttributeOrDef(xe, "radius", 1);

	Ryushka::COUNT_ID = math::max(Ryushka::COUNT_ID, _id); 
}

IRect Ryushka::getEditRect()
{
	return getClientRect();
}

void Ryushka::SetDepthStyle(Color depth_style)
{
	_currentDepthStyle = depth_style;
	SetDepth(_depth);
}

//риcуем рамку в редакторе
void Ryushka::DrawRamka(){
	IRect r = getVisibleRect();
	
	Render::device.SetTexturing(false);
	Render::DrawFrame(r);
	Render::device.SetTexturing(true);

	Render::FreeType::BindFont("debug");
	Render::PrintString(r.x, r.y, Int::ToString(GetId()), 1.f, LeftAlign, BottomAlign);			
	if (Ryushka::HardPtr connect = GetConnect().lock()) 
	{
		Render::PrintString(r.x, r.y-20, Int::ToString(connect->GetId()), 1.f, LeftAlign, BottomAlign);			
	}
}

//получаем id;
int Ryushka::GetId()
{
	return _id;
}

//уcтанавливаем id;
void Ryushka::SetId()
{
	if (_id == 0)
	{
		_id = ++COUNT_ID;
	}
}
int Ryushka::GetConnectId()
{

	return _id_connect;
}

Ryushka::HardPtr Ryushka::SelectRyushka()
{
	_isSelected = true;
	return shared_from_this();
}

Ryushka::HardPtr Ryushka::UnSelectRyushka()
{
	_isSelected = false;
	return Ryushka::HardPtr();
}

void Ryushka::SetConnect(Ryushka::HardPtr ryushka)
{
	_connect = ryushka; 
}

void Ryushka::AcceptMessage(const Message &message)
{

}

Ryushka::WeakPtr Ryushka::GetConnect()
{
	return _connect;
}
void Ryushka::ResetConnect()
{
	_connect.reset();
}
void Ryushka::Reset()
{
}


Ryushka::~Ryushka()
{
}

float Ryushka::GetNearestScale(float scale) {
	// Здеcь пользуемcя тем, что маccив SCALES возраcтает
	int size = sizeof(SCALES) / sizeof(SCALES[0]);
	for (int i = 0; i < size; ++i) {
		if (scale <= SCALES[i] + 0.01f) {
			return SCALES[i];
		}
	}
	return SCALES[size - 1];
}

void Ryushka::Update(float dt)
{
	//IRect r = getClientRect();
}

Render::Texture* Ryushka::getTexture(std::string name)
{
	if (name.at(0) == '#'){
		name.erase(0, 1);
	}
	_textureNames.push_back(name);
	Render::Texture *tex = Core::resourceManager.Get<Render::Texture>(name);
	tex->setFilteringType(Render::Texture::BILINEAR);
	return tex;
}

void Ryushka::Upload()
{
	for (size_t i = 0; i<_textureNames.size(); i++)
	{
		Core::resourceManager.Get<Render::Texture>(_textureNames[i])->BeginUse();
	}
}

void Ryushka::Release()
{
	for (size_t i = 0; i<_textureNames.size(); i++)
	{
		Core::resourceManager.Get<Render::Texture>(_textureNames[i])->EndUse();
	}
}


bool Ryushka::isRyushkaOnScreen()
{
	return _visible;
}


bool Ryushka::Contains(FPoint pos, const IRect &rect,const FPoint &rec_half)
{
	pos = pos -  rec_half;
	pos = _scale*pos;
	pos = pos + rec_half;
	return rect.Contains(pos.Rounded());
}

void Ryushka::UpdateVisible(float screenX, float screenY)
{
/*	int w = getClientRect().width+200;
	int h = getClientRect().height+200;

	int rectX = getClientRect().x;
	int rectY = getClientRect().y;

	// у некоторых рюшек почему-то облаcть видимоcти задаётcя отноcительно центра рюшки, поэтому увеличим облаcть видимоcти/
	// дополнительная рамка в 100 пикcелей - на вcякий cлучай, еcть рюшки у которых параметры не правильно определены.
	rectX = rectX - w/2 - 100;
	rectY = rectY - h/2 - 100;
	w = w + w/2 + 200;
	h = h + h/2 + 200;*/

	//IRect rect = getClientRect();
	IRect rect = getVisibleRect();
	// увеличиваем облаcть видимоcти при повороте и при маcштабе
	IPoint shift(0, 0);
	if (_angle != 0)
	{
		shift.x += math::abs((float)(fabs(sinf(_angle*math::PI/180.f))*rect.height - rect.width));
		shift.y += math::abs((float)(fabs(sinf(_angle*math::PI/180.f))*rect.width - rect.height));
	} 
	if (_scale > 1)
	{
		shift.x += math::round((_scale-1)*rect.width);		
		shift.y += math::round((_scale-1)*rect.height);
	}
	if (_xMirror)
	{
		IRect r = getClientRect();
		rect.width += r.width;
	}

	if ((screenX < rect.x + rect.width + shift.x)&&(screenX + GameSettings::FIELD_SCREEN_CONST.width > rect.x-shift.x)
		&&(screenY < rect.y + rect.height + shift.y)&&(screenY + GameSettings::FIELD_SCREEN_CONST.height > rect.y-shift.y))
	{
		_visible = true;
	}
	else 
	{
		_visible = false;
	}
}

void Ryushka::IncRadius() 
{
	if (_radius	> 4)
	{
		_radius = 0;
	}
	else
	{
		_radius++;
	}
}

Ryushka::HardPtr Ryushka::CreateRyushkaFromXml(rapidxml::xml_node<> *xmlElement)
{
	std::string  name = xmlElement->name();
	if (name == "StaticImage") {
		return Ryushka::HardPtr(new StaticImage(xmlElement));
	} else if (name == "EffectsStaticImage") {
		return Ryushka::HardPtr(new EffectsStaticImage(xmlElement));
	} else if (name == "TreeImage") {
		return Ryushka::HardPtr(new TreeImage(xmlElement));
	} else if (name == "TestRyushka") {
		return Ryushka::HardPtr(new TestRyushka(xmlElement));
	} else if (name == "FlyImage") {
		return Ryushka::HardPtr(new FlyImage(xmlElement));
	} else if (name == "LavaImage") {
		return Ryushka::HardPtr(new LavaImage(xmlElement));
	} else if (name == "FlagImage") {
		return Ryushka::HardPtr(new FlagImage(xmlElement));
	} else if (name == "LampImage") {
		return Ryushka::HardPtr(new LampImage(xmlElement));
	} else if (name == "EffectRyushka") {
		return Ryushka::HardPtr( new EffectRyushka(xmlElement));
	} else if (name == "RyushkaTrees") {
		return Ryushka::HardPtr(new RyushkaTrees(xmlElement));
	} else if (name == "RyushkaCone") {
		return Ryushka::HardPtr(new RyushkaCone(xmlElement));
	} else if (name == "RyushkaLamp") {
		return Ryushka::HardPtr(new RyushkaLamp(xmlElement));
	} else if (name == "RyushkaBridge") {
		return Ryushka::HardPtr(new RyushkaBridge(xmlElement));
	} else if (name == "RyushkaGroup") {
		return Ryushka::HardPtr(new RyushkaGroup(xmlElement));
	} else if (name == "RyushkaList") {
		return Ryushka::HardPtr(new RyushkaList(xmlElement));
	} else if (name == "RyushkaFaerie") {
		return Ryushka::HardPtr(new RyushkaFaerie(xmlElement));
	} else if (name == "RyushkaWeb") {
		return Ryushka::HardPtr(new RyushkaWeb(xmlElement));
	} else if (name == "RyushkaLightRope") {
		return Ryushka::HardPtr(new RyushkaLightRope(xmlElement));
	} else if (name == "RyushkaWindow") {
		return Ryushka::HardPtr(new RyushkaWindow(xmlElement));
	}
	Log::Error("No such ryushka: " + name);
	//Assert(false);
	Ryushka *r = NULL;
	return Ryushka::HardPtr(r);
}

void Ryushka::Draw()
{
    IRect rect = getClientRect();
    HardPtr connect = _connect.lock();

    if (_scale != 1 || _angle != 0 || _xMirror)
    {
        IPoint offset(rect.x + rect.width / 2, rect.y + rect.height / 2);
    
        Render::device.PushMatrix();
        
        if (connect) 
        {
            FPoint pos = connect->GetDeltaPos();
            if (pos.x == 0 && pos.y == 0)
            {
                Render::device.MatrixTranslate(math::Vector3(offset.x + 0.f, offset.y + 0.f, 0.f));
            } else {
                IRect rect_connect = connect->getClientRect();

                FPoint v;
                v.x = math::round(-rect_connect.x - rect_connect.width/2.f + rect.x + rect.width/2.f) + 0.f;
                v.y = math::round(-rect_connect.y - rect_connect.height/2.f + rect.y + rect.height/2.f) + 0.f;
                Render::device.MatrixTranslate(math::Vector3(pos.x, pos.y, 0.f));
                Render::device.MatrixTranslate(math::Vector3(rect_connect.x + rect_connect.width/2 + 0.f, rect_connect.y + rect_connect.height/2 +0.f , 0));
                Render::device.MatrixRotate(math::Vector3(0,0,1), 10*pos.x*math::PI/180.f);
                Render::device.MatrixTranslate(math::Vector3(v));
            }
        }
        else
        {
            Render::device.MatrixTranslate(math::Vector3(offset.x + 0.f, offset.y + 0.f, 0.f));
        }

        if (_scale != 1)
        {
            Render::device.MatrixScale(_scale);
        }
        if (_xMirror) {
            Render::device.MatrixScale(-1.0f, 1.0f, 1.0f);
        }
        if (_angle != 0)
        {
            Render::device.MatrixRotate(math::Vector3(0, 0, 1), _angle);
        }
        Render::device.MatrixTranslate(math::Vector3(-offset.x + 0.f, -offset.y + 0.f, 0.f));

        DepthOnDraw();

        Render::device.PopMatrix();
    } else {        
        if (connect) 
        {
            FPoint pos = connect->GetDeltaPos();
            if (pos.x == 0 && pos.y == 0)
            {
                DepthOnDraw();            
            } else {
                IRect rect_connect = connect->getClientRect();
                Render::device.PushMatrix();
                Render::device.MatrixTranslate(math::Vector3(pos.x, pos.y, 0.f));
                Render::device.MatrixTranslate(math::Vector3(rect_connect.x + rect_connect.width/2 + 0.f, rect_connect.y + rect_connect.height/2 +0.f , 0));
                Render::device.MatrixRotate(math::Vector3(0,0,1), 10*pos.x*math::PI/180.f);
                Render::device.MatrixTranslate(math::Vector3(-rect_connect.x + -rect_connect.width/2 + 0.f, -rect_connect.y + -rect_connect.height/2 +0.f , 0));
                DepthOnDraw();
                Render::device.PopMatrix();
            }
        } else {
            DepthOnDraw();
        }
        
    }
}

void Ryushka::DepthOnDraw()
{
    if (_depth > 0)
    {
        Render::BeginColor(math::lerp(Color(255, 255, 255), _currentDepthStyle, _depth));
        OnDraw();
        Render::EndColor();
    } else {
        OnDraw();
    }
}


FPoint Ryushka::GetDeltaPos()
{
	return FPoint(0, 0);
}

float Ryushka::GetScale() {
	return _scale;
}

void Ryushka::SetScaleByFactor(float factor) {
	Assert(0.0f <= factor && factor <= 1.0f);
	
	// размер маccива SCALES:
	int size = sizeof(Ryushka::SCALES) / sizeof(Ryushka::SCALES[0]);
	
	// единичный отрезок делитcя на size чаcтей,
	// в какую чаcть попал factor,
	// тот (по номеру) элемент SCALES и возвращаетcя.
	// Граничные cлучаи наc не интереcуют, мы проcто избегаем их появления
	// (cм. GetFactorByScale())

	// Отбраcывание дробной чаcти в принципе наc cпаcет в cлучае, еcли factor != 1
	int index = int(factor * size);
	if (index == size) {
		index = size - 1;
	}
	Assert(0 <= index && index < size);
	_scale = Ryushka::SCALES[index];
}

float Ryushka::GetFactorByScale() {
	// размер маccива SCALES:
	int size = sizeof(Ryushka::SCALES) / sizeof(Ryushka::SCALES[0]);

	// единичный отрезок делитcя на size чаcтей,
	// возвращаетcя центр того отрезка, номер которого
	// cовпадает c номером элемента SCALES, значение которого
	// равно _scale.
	for (int i = 0; i < size; ++i) {
		if (Ryushka::SCALES[i] == _scale) {
			return (i + 0.5f) / size;
		}
	}
	// _scale обязан приcутcтвовать в SCALES
	Assert(false);
	return 0.0f;
}

Xml::TiXmlElement* Ryushka::CreateXmlElement(Xml::TiXmlElement *parentXml, std::string  name) {
	Xml::TiXmlElement *elem = parentXml->InsertEndChild(Xml::TiXmlElement(name.c_str()))->ToElement();
	Xml::SetFloatAttribute(elem, "angle", _angle);
	Xml::SetFloatAttribute(elem, "depth", _depth);
	Xml::SetIntAttribute(elem, "id", _id);
	Xml::SetStringAttribute(elem, "name", _name);
	Xml::SetFloatAttribute(elem, "scale", _scale);
	Xml::SetBoolAttribute(elem, "xmirror", _xMirror);
	Xml::SetIntAttribute(elem, "zLevel", _zLevel);
	if (Ryushka::HardPtr connect = _connect.lock())
	{
		Xml::SetIntAttribute(elem, "id_con", connect->GetId());
	}
	return elem;
}

int Ryushka::GetZLevel() {
	return _zLevel;
}

void Ryushka::SetZLevelByKey(int key) {
	switch(key) {
		case '1':
			_zLevel = -4;
			break;
		case '2':
			_zLevel = -3;
			break;
		case '3':
			_zLevel = -2;
			break;
		case '4':
			_zLevel = -1;
			break;
		case '5':
			_zLevel = 1;
			break;
		case '6':
			_zLevel = 2;
			break;
		case '7':
			_zLevel = 3;
			break;
		case '8':
			_zLevel = 4;
			break;
	}
}

float Ryushka::GetDepth() {
	return _depth;
}

void Ryushka::SetDepth(float depth) {
	_depth = depth;
}

float Ryushka::GetAngle() {
	return _angle;
}

void Ryushka::SetAngle(float angle) {
	_angle = angle;
}

void Ryushka::MirrorX() {
	_xMirror = !_xMirror;
}
bool Ryushka::GetMirrorX(){
	return _xMirror;
}
IPoint Ryushka::CorrectionMove(const IPoint &pos)
{
	FPoint p;
	IRect rect = getClientRect();
	p.x = (float)(pos.x - rect.x - rect.width/2.f);
	p.y = (float)(pos.y - rect.y - rect.height/2.f);
	if (GetMirrorX())
	{
		p.Scale(-1 /GetScale(), 1 /GetScale());
		p = p.Rounded();
	}
	else
	{
		p.Scale(1 /GetScale(), 1 /GetScale());
		p = p.Rounded();
	}
	p = FPoint(p).Rotated(-GetAngle()/ 180 * math::PI).Rounded();

	p.x += (float)rect.width/2.f;
	p.y += (float)rect.height/2.f;
	return p.Rounded();

}

FPoint Ryushka::CorrectionDp(FPoint dp)
{
	dp = dp.Rotated(GetAngle()/ 180 * math::PI);
	if (GetMirrorX())
	{
		dp.Scale(-GetScale(), GetScale());
	}
	else
	{
		dp.Scale(GetScale(), GetScale());
	}
	

	return dp;

}
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////

StaticImage::StaticImage(rapidxml::xml_node<>* xmlElement)
	: Ryushka(xmlElement, 1.f)
{
	_textureName = std::string(xmlElement->first_node("Texture")->first_attribute("name")->value());

	_tex = getTexture(_textureName);

	rapidxml::xml_node<> *pos_xml = xmlElement->first_node("Position");
	_posCenter = pos_xml ? IPoint(pos_xml) : IPoint(0,0);

	_rect.height = _tex->getBitmapRect().height;
	_rect.width = _tex->getBitmapRect().width;

	_rect.x = _posCenter.x -_rect.width/2; 
	_rect.y = _posCenter.y -_rect.height/2; 

	FRect rect(0,1,0,1);
	FRect uv(0,1,0,1);
	_tex->TranslateUV(rect, uv);
	u1 = uv.xStart;
	u2 = uv.xEnd;
	v1 = uv.yStart;
	v2 = uv.yEnd;
}

void StaticImage::SaveToXml(Xml::TiXmlElement *parentElem)
{
	Xml::TiXmlElement *elem = Ryushka::CreateXmlElement(parentElem, "StaticImage");
	Xml::TiXmlElement *texElem = elem->InsertEndChild(Xml::TiXmlElement("Texture"))->ToElement();
	texElem->SetAttribute("name", _textureName);
	Xml::TiXmlElement *posElem = elem->InsertEndChild(Xml::TiXmlElement("Position"))->ToElement();
	posElem->SetAttribute("x", _rect.x  + _tex->getBitmapRect().width / 2);
	posElem->SetAttribute("y", _rect.y  + _tex->getBitmapRect().height / 2);
}

void StaticImage::OnDraw()
{
	_tex->Draw(_rect, u1, u2, v1, v2);
}

void StaticImage::Update(float dt)
{
	local_time += dt;
}

void StaticImage::setPosition(const IPoint &pos)
{
	_rect.x = pos.x;
	_rect.y = pos.y;
	_posCenter.x = _rect.x +_rect.width/2; 
	_posCenter.y = _rect.y +_rect.height/2; 
}


IPoint StaticImage::Reduced(const IPoint& pos) 
{
	FPoint fPos;
	if (GetMirrorX())
	{
		fPos = (FPoint(pos)).Rotated( GetAngle()/ 180 * math::PI) ;
		fPos.Scale(-1 /GetScale(), 1 / GetScale());
	}
	else
	{
		fPos = (FPoint(pos)).Rotated( -GetAngle()/ 180 * math::PI) ;
		fPos.Scale(1 /GetScale(), 1 / GetScale()) ;
	}


	return fPos.Rounded();
}

IRect StaticImage::getClientRect()
{
	return _rect;
}

IRect StaticImage::getVisibleRect()
{
	return _rect;
}

RyushkaCone::RyushkaCone(rapidxml::xml_node<>* xmlElement)
	: StaticImage(xmlElement)
{

	rapidxml::xml_node<> *element = xmlElement->first_node("Cone");
	_posScale.x = utils::lexical_cast<int>(std::string(element->first_attribute("xS")->value()));
	_posScale.y = utils::lexical_cast<int>(std::string(element->first_attribute("yS")->value()));
	_timeShow = utils::lexical_cast<float>(std::string(element->first_attribute("timeShow")->value()));
	_sRotate = utils::lexical_cast<float>(std::string(element->first_attribute("sRotate")->value()));
	_a.y = -utils::lexical_cast<float>(std::string(element->first_attribute("g")->value()));
	_a.x = 0;
	SetState(STATE_HANG);

}
void RyushkaCone::SetState(State state)
{
	_state = state;
	if (_state == STATE_HANG)
	{
		_s.x = 0;
		_s.y = 0;
		_p.x = 0;
		_p.y = 0;
		_scale = 1;
		_alpha = 1;
		_angle = 0;
	}
	
	if (_state == STATE_SHOW)
	{
		_s.x = 0;
		_s.y = 0;
		_p.x = 0;
		_p.y = 0;
		_scale = 0;
		_angle = 0;
	}




}
void RyushkaCone::Update(float dt){
	StaticImage::Update(dt);
	if (_state == STATE_FALL)
	{
		_p += _s*dt + _a*dt*dt/2;
		_s += _a*dt;	
		_angle += dt*_sRotate;
		if (_p.y < -GameSettings::FIELD_SCREEN_CONST.height)
		{
			SetState(STATE_SHOW);
		}
	}


	if (_state == STATE_SHOW)
	{
		_scale += dt/_timeShow;

		if (_scale > 1)
		{
			SetState(STATE_HANG);
		}

	}


}
void RyushkaCone::OnDraw(){

	FPoint p  = FPoint(_posCenter) + Reduced(_p.Rounded());

	if (_state == STATE_SHOW)
	{
		IPoint p1 = _posCenter - _posScale;
		p1.x -= _rect.x;
		p1.y -= _rect.y;

		p += (_scale - 1)*FPoint(p1);	
	}

	Render::device.PushMatrix();
	Render::device.MatrixTranslate(math::Vector3(p));
	Render::device.MatrixRotate(math::Vector3(0, 0, 1), _angle);
	Render::device.MatrixScale(_scale, _scale, 1.0f);
	DrawTextureCentered(_tex, IPoint(0, 0), true);
	Render::device.PopMatrix();

}

void RyushkaCone::MouseDown(const IPoint &pos)
{
	// елcи попали

	IPoint p = CorrectionMove(pos);
	if (_tex->isPixelOpaque(p))
	{
		if (_state == STATE_HANG){
			SetState(STATE_FALL);
		}
	}


}






void RyushkaCone::SaveToXml(Xml::TiXmlElement *parentElem)
{
	Xml::TiXmlElement *elem = Ryushka::CreateXmlElement(parentElem, "RyushkaCone");
	Xml::TiXmlElement *texElem = elem->InsertEndChild(Xml::TiXmlElement("Texture"))->ToElement();
	texElem->SetAttribute("name", _textureName);
	Xml::TiXmlElement *posElem = elem->InsertEndChild(Xml::TiXmlElement("Position"))->ToElement();
	posElem->SetAttribute("x", _posCenter.x);
	posElem->SetAttribute("y", _posCenter.y);

	//по алфавиту!
	Xml::TiXmlElement *texCone = elem->InsertEndChild(Xml::TiXmlElement("Cone"))->ToElement();
	texCone->SetDoubleAttribute("g", -(double)_a.y);
	texCone->SetDoubleAttribute("sRotate", (double)_sRotate);
	texCone->SetDoubleAttribute("timeShow", (double)_timeShow);
	texCone->SetAttribute("xS", _posScale.x);
	texCone->SetAttribute("yS", _posScale.y);

}

void RyushkaCone::setPosition(const IPoint &pos)
{
	StaticImage::setPosition(pos);
}


Tree::Tree(rapidxml::xml_node<>* xmlElement, Ryushka *parent, IPoint shift)
{    
	_shift = shift;

    _parent = parent;
    _textureName = std::string(xmlElement->first_attribute("name")->value());
    _tex = _parent->getTexture(_textureName);
        

    _rect.width = (int)_tex->getBitmapRect().width;
    _rect.height = (int)_tex->getBitmapRect().height;
    
    FRect uv(0, 1, 0, 1);
    if (_tex->needTranslate()) {
        FRect rect(0, _rect.width + 0.f,  0, _rect.height + 0.f);
        _tex->TranslateUV( rect, uv);
        _rect = IRect(rect);
    }

    _rect.x += utils::lexical_cast<int>(std::string(xmlElement->first_attribute("x")->value()));
    _rect.y += utils::lexical_cast<int>(std::string(xmlElement->first_attribute("y")->value()));
    
    _mesh = new Mesh(Mesh::TREE);
    _mesh->Init(_rect, uv);

    _pendant = new Pendant(xmlElement, _parent, Pendant::ONE);

    _mesh->InitK(_pendant->_pStart, _pendant->_pEnd);
}

Tree::~Tree()
{
	delete _mesh;
	delete _pendant;
}


void Tree::Update(float dt)
{
    _pendant->Update(dt);
    _mesh->Update(_pendant->GetDx());
}

void Tree::SetColor(Color color)
{
    _mesh->SetColor(color);
}

void Tree::OnDraw()
{    
    Render::device.PushMatrix();
    Render::device.MatrixTranslate(math::Vector3(_rect.x + _shift.x + 0.f, _rect.y + _shift.y + 0.f, 0.f));
    _tex->Bind();
    _mesh->Draw();
    _pendant->Draw();
    Render::device.PopMatrix();
}

void Tree::MouseMove(const IPoint &pos)
{
	IPoint p;

	p.x = pos.x - _rect.x;//- _rect.width/2;
	p.y = pos.y - _rect.y;//- _rect.height/2;

	_pendant->MouseMove(p, _parent->GetAngle(),_parent->GetScale() );

}
void Tree::SaveToXml(Xml::TiXmlElement *parentElem)
{
	parentElem->SetAttribute("name", _textureName);
	_pendant->SaveToXml(parentElem);
}



Pendant::Pendant(IPoint pStart, IPoint pEnd,  float ampl, float  fr, float m, float c)
{	

}

Pendant::Pendant(rapidxml::xml_node<> *xmlElement, Ryushka *parent, Type type)
	: _parent(parent)
	, _type(type)
{	
	_pStart.x = utils::lexical_cast<int>(std::string(xmlElement->first_attribute("xS")->value()));
	_pStart.y = utils::lexical_cast<int>(std::string(xmlElement->first_attribute("yS")->value()));

	_pEnd.x = utils::lexical_cast<int>(std::string(xmlElement->first_attribute("xE")->value()));
	_pEnd.y = utils::lexical_cast<int>(std::string(xmlElement->first_attribute("yE")->value()));

	_ampl = utils::lexical_cast<float>(std::string(xmlElement->first_attribute("ampl")->value()));
	_random_ampl = _ampl*math::random(1.f, 2.f);
	_fr = utils::lexical_cast<float>(std::string(xmlElement->first_attribute("fr")->value())); 
	_m = utils::lexical_cast<float>(std::string(xmlElement->first_attribute("m")->value())); 
	_c = utils::lexical_cast<float>(std::string(xmlElement->first_attribute("c")->value())); 
	_w = utils::lexical_cast<float>(std::string(xmlElement->first_attribute("w")->value())); 

	_pos = IPoint(xmlElement);

	_mousePosOld = IPoint(0,0);
	_dx = math::random(-_ampl, _ampl);
	_speed = 0;
	Reset();
}

float Pendant::GetDx(){
	// что бы в начеле не дергалcя
	if (_timer < 1.f)
	{
		return _dx*_timer*_timer*_timer;
	}
	return _dx;
}

void Pendant::Update(float dt){
	if (_timer < 1.f)
	{
		_timer += dt;
	}

	_dx += _speed*dt;
	float old_speed = _speed;
	_speed -= ((_fr*_dx)+(_c*_speed))*dt;	

	float ds = 0;
	if (_speed > 0) ds = _random_ampl;
	else ds = -_random_ampl;
	_speed += ds*dt;
	
	if (old_speed*_speed < 0)
	{
		_random_ampl = _ampl*math::random(1/5.f, 1.f);

	}
	_speed = math::clamp(-1000.f, 1000.f, _speed);
}
void Pendant::Reset()
{
	_timer = 0.f;
}
void Pendant::SaveToXml(Xml::TiXmlElement *effElem)
{
	// по алфавиту!
		effElem->SetDoubleAttribute("ampl", _ampl);
		effElem->SetDoubleAttribute("c", _c);
		effElem->SetDoubleAttribute("fr", _fr);	
		effElem->SetDoubleAttribute("m", _m);	
		effElem->SetDoubleAttribute("w", _w);
		effElem->SetAttribute("x", _pos.x);
		effElem->SetAttribute("xE", _pEnd.x);
		effElem->SetAttribute("xS", _pStart.x);
		effElem->SetAttribute("y", _pos.y);
		effElem->SetAttribute("yE", _pEnd.y);
		effElem->SetAttribute("yS", _pStart.y);

}

void Pendant::MouseMove(IPoint pos, float angle, float scale)
{
	// Сжимаем

	float ds = 0;


		float a;
		if (_pEnd == _pStart)
		{
			a = 0;
		}
		else
		{
			a = FPoint(_pEnd-_pStart).GetAngle()+math::PI/2.0f;
		}

		IPoint pStart = FPoint(_pStart).Rotated(-a).Rounded();
		IPoint pEnd = FPoint(_pEnd).Rotated(-a).Rounded();
		pos = FPoint(pos).Rotated(-a).Rounded();
	
		if (FPoint(pos).GetDistanceTo(_mousePosOld) < 100)
		{
	
	
			 if (((pEnd.y <= pos.y)&& (pos.y <= pStart.y))&& 
				((pStart.x - _w/2 <= pos.x)&& (pos.x <= pEnd.x+_w/2)))

			{	
 				if (_type == ONE)
				{
					ds = FPoint(pos-pStart).GetVectorProduct(pos-_mousePosOld)/_m*scale;
				}
				else if (_type == TWO)
				{
					if (FPoint(pos).GetDistanceTo(pStart) < FPoint(pos).GetDistanceTo(pEnd))
					{
						ds = FPoint(pos-pStart).GetVectorProduct(pos-_mousePosOld)/_m*scale;
					}
					else
					{
						ds = FPoint(pEnd-pos).GetVectorProduct(pos-_mousePosOld)/_m*scale;

					}
				}
			}
		    if (math::abs(ds) > 5 && math::abs(_speed)> 40 && math::sign(_speed) == math::sign(ds))
			{
				ds = 5;
			}
			if (fabs(ds) > 15)
			{
				ds = math::sign(ds)*15;
			}
 			if (((_dx > 0.1f) && (_speed > 0.1)) || ((_dx < -0.1f) && (_speed < -0.1f)))
			{
				_speed += ds*_ampl/(math::abs(_dx)+_ampl);
			}
			else 
			{
				_speed += ds;
			}
		}

	



	_mousePosOld = pos;

}



void Pendant::Draw()
{
	if (_parent->_isSelected && EditorUtils::editor)
    {
        Render::device.SetTexturing(false);
        Render::BeginColor(Color(255, 0, 255));
        Render::DrawQuad(_pStart.x - 1 + _pos.x + 0.f, _pStart.y - 1 + _pos.y + 0.f, 2, 2);
        Render::DrawQuad(_pEnd.x - 1 + _pos.x + 0.f, _pEnd.y - 1 + _pos.y + 0.f, 2, 2);
        Render::EndColor();
        Render::device.SetTexturing(true);
    }
}

RyushkaTrees::RyushkaTrees(rapidxml::xml_node<>* xmlElement)
    : Ryushka(xmlElement, 1.f)
	, _editPos(IPoint(0, 0))
{
    rapidxml::xml_node<> *element = xmlElement->first_node("Texture");

    _rect.x = utils::lexical_cast<int>(std::string(xmlElement->first_node("Position")->first_attribute("x")->value())); 
    _rect.y = utils::lexical_cast<int>(std::string(xmlElement->first_node("Position")->first_attribute("y")->value())); 

    IPoint minPos(0,0);
    IPoint maxPos(0,0);
    while (element != NULL)
    {
        Tree *tree = new Tree(element, this, IPoint(_rect.x, _rect.y));
        minPos.x = math::min(tree->_rect.x, minPos.x);
        minPos.y = math::min(tree->_rect.y, minPos.y);
        maxPos.x = math::max(tree->_rect.x+tree->_rect.width, maxPos.x);
        maxPos.y = math::max(tree->_rect.y+tree->_rect.height, maxPos.y);
        element = element -> next_sibling("Texture");
        _trees.push_back(tree);
    }
    _rect.width = maxPos.x - minPos.x;
    _rect.height = maxPos.y - minPos.y;
}

RyushkaTrees::~RyushkaTrees()
{
	for(Iterator it = _trees.begin(); it != _trees.end(); ++it ) {
        delete (*it);
    }
}

void RyushkaTrees::SetDepth(float depth)
{
    Ryushka::SetDepth(depth);
    _currentColor = math::lerp(Color(255, 255, 255), _currentDepthStyle, _depth);
    for(Iterator it = _trees.begin(); it != _trees.end(); ++it ) {
        (*it)->SetColor(_currentColor);
    } 
}


void RyushkaTrees::Update(float dt){
    for(Iterator it = _trees.begin(); it != _trees.end(); ++it ) {
      (*it)->Update(dt);
    }
}

void RyushkaTrees::SaveToXml(Xml::TiXmlElement *parentElem)
{
    Xml::TiXmlElement *elem = Ryushka::CreateXmlElement(parentElem, "RyushkaTrees");
    for(Iterator it = _trees.begin(); it != _trees.end(); ++it ) {
        Xml::TiXmlElement *texElem = elem->InsertEndChild(Xml::TiXmlElement("Texture"))->ToElement();
        (*it)->SaveToXml(texElem);
    }
    Xml::TiXmlElement *posElem = elem->InsertEndChild(Xml::TiXmlElement("Position"))->ToElement();
    posElem->SetAttribute("x", _rect.x + _editPos.x);
    posElem->SetAttribute("y", _rect.y + _editPos.y);
}

void RyushkaTrees::setPosition(const IPoint &pos)
{
    _editPos = pos;
    _editPos.x -= _rect.x;
    _editPos.y -= _rect.y;
}

IRect RyushkaTrees::getClientRect()
{
    return IRect(_rect.x + _editPos.x, _rect.y + _editPos.y, _rect.width, _rect.height);
}

IRect RyushkaTrees::getVisibleRect()
{
	return IRect(_rect.x + _editPos.x, _rect.y + _editPos.y, _rect.width, _rect.height);
}

void RyushkaTrees::OnDraw()
{        
    if (_editPos.x != 0 || _editPos.y != 0)
    {
        Render::device.PushMatrix();
        Render::device.MatrixTranslate(math::Vector3(_editPos.x + 0.f, _editPos.y + 0.f, 0.f));
        Render::BeginColor(_currentColor);
        for(Iterator it = _trees.begin(); it != _trees.end(); ++it ) {
            (*it)->OnDraw();
        } 
        Render::EndColor();
        Render::device.PopMatrix();

    } else {
        Render::BeginColor(_currentColor);
        for(Iterator it = _trees.begin(); it != _trees.end(); ++it ) {
            (*it)->OnDraw();
        } 
        Render::EndColor();
    }

}

void RyushkaTrees::MouseMove(const IPoint &pos){
    IPoint p = CorrectionMove(pos);
    for(Iterator it = _trees.begin(); it != _trees.end(); ++it ) {
        (*it)->MouseMove(p);
    }
}


Mesh::Mesh(State state)
{	
	_state = state;
}

Mesh::~Mesh()
{
	delete _dist;
}


void Mesh::Init(const IRect& rect, const FRect& uv)
{
	int w = rect.width;
	int h = rect.height;				

	_columnMesh = w/50 + 1;
	_rowMesh = h/50 + 1;

	_dist = new Distortion(_columnMesh + 1, _rowMesh + 1);
	_dist->SetRenderRect(IRect(0, 0, w, h), uv.xStart, uv.xEnd, uv.yStart, uv.yEnd);

	float stepC = w / (float)_columnMesh;
	float stepR = h / (float)_rowMesh;

	for (int i=0; i < _rowMesh+1; i++){
		std::vector<FPoint> column;
		for (int j=0; j < _columnMesh+1; j++){
			FPoint p;
			p.x = j*stepC; 
			p.y = i*stepR;
			column.push_back(p);
		}
		_mesh.push_back(column);
	}
}
void Mesh::InitK(IPoint pStart, IPoint pEnd){
	_pStart = pStart;
	_pEnd = pEnd;
	FPoint 	center;
	float max_l;
	if (_state == BRIDGE)
	{
		_normal = FPoint(_pEnd-_pStart).Rotated(math::PI/2.0f);
		_normal = _normal/_normal.GetDistanceToOrigin();

		center = FPoint((pStart+pEnd)/2);
		max_l = FPoint(_pStart).GetDistanceTo(center);
		for (int i=0; i <_rowMesh+1; i++){
			std::vector<Grid> column;
			for (int j=0; j<_columnMesh+1; j++){
				Grid grid;
				grid.pos.x = _mesh[i][j].x;
				grid.pos.y = _mesh[i][j].y;
				grid.l = _mesh[i][j].GetDistanceTo(center);
				if ((_mesh[i][j].GetDistanceTo(_pEnd+IPoint(13, -8)) < 10)||
					(_mesh[i][j].GetDistanceTo(_pEnd+IPoint(-13, 8)) < 10)||
					(_mesh[i][j].GetDistanceTo(_pStart+IPoint(8, 8)) < 10)||
					(_mesh[i][j].GetDistanceTo(_pStart+IPoint(-8, -8)) < 10))
				{
					grid.k = 0;
				}
				else
				{
					grid.k = math::cos(grid.l/max_l)*math::cos(grid.l/max_l)*math::cos(grid.l/max_l);
				}
				column.push_back(grid);
			}
			_d_mesh.push_back(column);
		}
	}
	else if (_state == TREE)
	{
		_normal = FPoint(0, 0);
		center = FPoint(pStart) - (_mesh[1][1] - _mesh[0][0])/2;
		max_l = _mesh[0][0].GetDistanceTo(_mesh[_rowMesh][_columnMesh]);
		for (int i=0; i <_rowMesh+1; i++){
			std::vector<Grid> column;
			for (int j=0; j<_columnMesh+1; j++){
				Grid grid;
				grid.pos.x = _mesh[i][j].x;
				grid.pos.y = _mesh[i][j].y;
				grid.l = _mesh[i][j].GetDistanceTo(center);
				grid.k = math::sin(grid.l/max_l);
				column.push_back(grid);
			}
			_d_mesh.push_back(column);
		}
	}
	else if (_state == FLAG)
	{
		max_l = _mesh[0][0].GetDistanceTo(_mesh[_rowMesh][_columnMesh]);
		for (int i=0; i <_rowMesh+1; i++){
			std::vector<Grid> column;
			for (int j=0; j<_columnMesh+1; j++){
				Grid grid;
				grid.pos.x = _mesh[i][j].x;
				grid.pos.y = _mesh[i][j].y;
				grid.l = _mesh[i][j].GetDistanceTo(center);
				grid.k = grid.l/max_l;
				column.push_back(grid);
			}
			_d_mesh.push_back(column);
		}
	}
	else if (_state == CHAIN)
	{
		_normal = FPoint(_pEnd-_pStart).Rotated(math::PI/2.0f);
		_normal = _normal/_normal.GetDistanceToOrigin();

		center = FPoint((pStart+pEnd)/2);
		max_l = FPoint(_pStart).GetDistanceTo(center);
		for (int i=0; i <_rowMesh+1; i++){
			std::vector<Grid> column;
			for (int j=0; j<_columnMesh+1; j++){
				Grid grid;
				grid.pos.x = _mesh[i][j].x;
				grid.pos.y = _mesh[i][j].y;
				grid.l = _mesh[i][j].GetDistanceTo(center)-10;
				if ((FPoint(_mesh[i][j]).GetDistanceTo(_pStart) < 10)||
					(FPoint(_mesh[i][j]).GetDistanceTo(_pEnd) < 10))
				{
					grid.k = 0;
				}
				else
				{
					grid.k = math::cos(grid.l/max_l)*math::cos(grid.l/max_l)*math::cos(grid.l/max_l);
				}
				column.push_back(grid);
			}
			_d_mesh.push_back(column);
		}
	}
	else if(_state == WEB)
	{
		_normal = FPoint(_pEnd-_pStart).Rotated(math::PI/2.0f);
		_normal = _normal/_normal.GetDistanceToOrigin();

		center = FPoint((pStart+pEnd)/2);
		max_l = FPoint(_pStart).GetDistanceTo(center);
		for (int i=0; i <_rowMesh+1; i++){
			std::vector<Grid> column;
			for (int j=0; j<_columnMesh+1; j++){
				Grid grid;
				grid.pos.x = _mesh[i][j].x;
				grid.pos.y = _mesh[i][j].y;
				grid.l = _mesh[i][j].GetDistanceTo(center);
				grid.k = grid.l/max_l;

				column.push_back(grid);
			}
			_d_mesh.push_back(column);
		}
	}
}

void Mesh::Update(float dx){
	if (_state == BRIDGE)
	{
		for (int i=0; i<_rowMesh+1; i++){
			std::vector<Grid>::iterator column = _d_mesh[i].begin();
			for (int j=0; j<_columnMesh+1; j++, ++column){
				_dist->SetDisplacement(j, i, _normal.x*dx* column->k, _normal.y*dx*column->k, REF_NODE);
			}
		}
	}
	else if (_state == CHAIN)
	{
		for (int i=0; i<_rowMesh+1; i++){
			std::vector<Grid>::iterator column = _d_mesh[i].begin();
			for (int j=0; j<_columnMesh+1; j++, ++column){
				_dist->SetDisplacement(j, i, _normal.x*dx* column->k, _normal.y*dx*column->k, REF_NODE);
			}
		}
	}
	else if (_state == TREE)
	{
		for (int i=0; i<_rowMesh+1; i++){
			std::vector<Grid>::iterator column = _d_mesh[i].begin();
			for (int j=0; j<_columnMesh+1; j++, ++column){
				FPoint disp(column->pos - _pStart);
				disp = disp.Rotated(dx*column->k*math::PI/180);
				disp += _pStart;
				disp -= column->pos;
				_dist->SetDisplacement(j, i, disp.x, disp.y, REF_NODE);
			}
		}
	}
	else if (_state == FLAG)
	{
		for (int i=0; i<_rowMesh+1; i++){
			std::vector<Grid>::iterator column = _d_mesh[i].begin();
			for (int j=0; j<_columnMesh+1; j++, ++column){
				_dist->SetDisplacement(j, i,
					math::cos(dx*5+(i+j))*column->k,
					math::sin(dx*5+(i+j))*column->k, REF_NODE);
			}
		}
	}
	else if(_state == WEB)
	{
		int half_rows = _rowMesh / 2;
		int half_columns = _columnMesh / 2;

		int rows_k, columns_k = 0;
		float tension_k; 

		for (int i=0; i<_rowMesh+1; i++){
			rows_k = half_rows - math::abs(half_rows - i);
			std::vector<Grid>::iterator column = _d_mesh[i].begin();
			for (int j=0; j<_columnMesh+1; j++, ++column){
				columns_k = half_columns - math::abs(half_columns - j);
				tension_k = ((float)columns_k / _columnMesh) + ((float)rows_k / _rowMesh); 

				_dist->SetDisplacement(j, i, _normal.x*dx* (column->k) * tension_k, _normal.y*dx*column->k * tension_k, REF_NODE);
			}
		}
	}
}

void Mesh::SetColor(Color color)
{
	_dist->SetColor( MAKECOLOR4(color.alpha, color.red, color.green, color.blue));
}

FPoint Mesh::ComputeGrid(int i, int j)
{
	return _mesh[i][j];
}


void Mesh::Draw()
{
	_dist->Draw();
}


TestRyushka::TestRyushka(rapidxml::xml_node<>* xmlElement)
	: Ryushka(xmlElement, 1.f)
{
	_textureName = std::string(xmlElement->first_node("Texture")->first_attribute("name")->value());

	_tex = getTexture(_textureName);

	_rect.x = utils::lexical_cast<int>(std::string(xmlElement->first_node("Position")->first_attribute("x")->value())) - _tex->getBitmapRect().width / 2;
	_rect.y = utils::lexical_cast<int>(std::string(xmlElement->first_node("Position")->first_attribute("y")->value())) - _tex->getBitmapRect().height / 2;
	_rect.width = _tex->getBitmapRect().width;
	_rect.height = _tex->getBitmapRect().height;

	FRect rect(0.0f, 1.0f, 0.0f, 1.0f);
	FRect uv(0.0f, 1.0f, 0.0f, 1.0f);
	_tex->TranslateUV(rect, uv);
	u1 = uv.xStart;
	u2 = uv.xEnd;
	v1 = uv.yStart;
	v2 = uv.yEnd;
}

void TestRyushka::SaveToXml(Xml::TiXmlElement *parentElem)
{
	Xml::TiXmlElement *elem = Ryushka::CreateXmlElement(parentElem, "TestRyushka");

	Xml::TiXmlElement *texElem = elem->InsertEndChild(Xml::TiXmlElement("Texture"))->ToElement();
	texElem->SetAttribute("name", _textureName);

	Xml::TiXmlElement *posElem = elem->InsertEndChild(Xml::TiXmlElement("Position"))->ToElement();
	posElem->SetAttribute("x", _rect.x + _tex->getBitmapRect().width / 2);
	posElem->SetAttribute("y", _rect.y + _tex->getBitmapRect().height / 2);
}

void TestRyushka::OnDraw()
{
	_tex->Bind();
	Render::DrawRect(_rect, u1, u2, v1, v2, Normal);
}

void TestRyushka::setPosition(const IPoint &pos)
{
	_rect.x = pos.x;
	_rect.y = pos.y;
}

IRect TestRyushka::getClientRect()
{
	return _rect;
}

IRect TestRyushka::getVisibleRect()
{
	return _rect;
}


EffectsStaticImage::EffectsStaticImage(rapidxml::xml_node<> *xmlElement)
: StaticImage(xmlElement)
{
	rapidxml::xml_node<> *effectElem = xmlElement->first_node("Effect");
	while (effectElem)
	{
		EffectDesc desc;
		desc.name = effectElem->first_attribute("name")->value();
		desc.x = utils::lexical_cast<int>(std::string(effectElem->first_attribute("x")->value()));
		desc.y = utils::lexical_cast<int>(std::string(effectElem->first_attribute("y")->value()));
		desc.zOrder = utils::lexical_cast<int>(std::string(effectElem->first_attribute("zOrder")->value()));	
	
		if (effectElem->first_attribute("distance"))
		{
			desc.distance = utils::lexical_cast<int>(std::string(effectElem->first_attribute("distance")->value()));	
		} else {
			desc.distance = 0;
		}
		
		ParticleEffect *eff = desc.zOrder == 0 ? _effectsBefore.AddEffect(desc.name) : _effectsAfter.AddEffect(desc.name);
		eff->posX = (float)desc.x;
		eff->posY = (float)desc.y;
		eff->Reset();

		desc.eff = eff;

		_descs.push_back(desc);

		effectElem = effectElem->next_sibling("Effect");
	}
}

void EffectsStaticImage::Update(float dt)
{
	_effectsBefore.Update(dt);
	_effectsAfter.Update(dt);

	IPoint mouse_pos = Core::mainInput.GetMousePos();	
	for (std::vector<EffectDesc>::iterator it = _descs.begin() ; it != _descs.end() ; ++it)
	{
		if (it->distance == 0)
			continue;
		float dx = (float)(mouse_pos.x) - (float)(_rect.x + _rect.width / 2) + GameSettings::FieldCoordMouse().x;
		float dy = (float)(mouse_pos.y) - (float)(_rect.y + _rect.height) + GameSettings::FieldCoordMouse().y;
		float dist2 = dx * dx + dy * dy;
		float alpha = 1.0f - dist2 / (it->distance * it->distance);
		it->eff->SetAlphaFactor(alpha);
	}
}

void EffectsStaticImage::Upload()
{
	StaticImage::Upload();
	for (size_t i = 0; i < _descs.size(); i++)
	{
		effectPresets.UploadEffect(_descs[i].name);
	}
}

void EffectsStaticImage::Release()
{
	StaticImage::Release();
	for (size_t i = 0; i < _descs.size(); i++)
	{
		effectPresets.ReleaseEffect(_descs[i].name);
	}
}

void EffectsStaticImage::OnDraw()
{
	Render::device.PushMatrix();
	Render::device.MatrixTranslate(math::Vector3((float)(_rect.x + _rect.width / 2), (float)(_rect.y + _rect.height / 2), 0.0f));
	_effectsBefore.Draw();
	Render::device.PopMatrix();
	StaticImage::OnDraw();
	Render::device.PushMatrix();
	Render::device.MatrixTranslate(math::Vector3((float)(_rect.x + _rect.width / 2), (float)(_rect.y + _rect.height / 2), 0.0f));
	_effectsAfter.Draw();
	Render::device.PopMatrix();

	/*
	IPoint mouse_pos = Core::mainInput.GetMousePos();
	Render::FreeType::BindFont("TitlePanel");

	for (std::vector<EffectDesc>::iterator it = _descs.begin() ; it != _descs.end() ; ++it)
	{
		if (it->distance == 0)
			continue;
		float dx = (float)(_rect.x + _rect.width / 2 - mouse_pos.x);
		float dy = (float)(_rect.y + _rect.height / 2 - mouse_pos.y);
		float dist2 = dx * dx + dy * dy;
		float alpha = 1.0f - dist2 / (it->distance * it->distance);
		Render::PrintString(IPoint(300, 50), utils::lexical_cast(mouse_pos.x) + " " + utils::lexical_cast(mouse_pos.y) +
			" " + utils::lexical_cast(dist2) +
			" " + utils::lexical_cast(_rect.x) + " " + utils::lexical_cast(_rect.y));
		//it->eff->SetAlphaFactor(alpha);
	}

	*/
}

void EffectsStaticImage::SaveToXml(Xml::TiXmlElement *parentElem)
{
	Xml::TiXmlElement *elem = Ryushka::CreateXmlElement(parentElem, "EffectsStaticImage");

	Xml::TiXmlElement *texElem = elem->InsertEndChild(Xml::TiXmlElement("Texture"))->ToElement();
	texElem->SetAttribute("name", _textureName);

	Xml::TiXmlElement *posElem = elem->InsertEndChild(Xml::TiXmlElement("Position"))->ToElement();
	posElem->SetAttribute("x", _rect.x  + _tex->getBitmapRect().width / 2);
	posElem->SetAttribute("y", _rect.y  + _tex->getBitmapRect().height / 2);

	for (std::vector<EffectDesc>::iterator it = _descs.begin() ; it != _descs.end() ; ++it)
	{
		// по алфавиту!
		Xml::TiXmlElement *effElem = elem->InsertEndChild(Xml::TiXmlElement("Effect"))->ToElement();
		effElem->SetAttribute("distance", it->distance);
		effElem->SetAttribute("name", it->name);
		effElem->SetAttribute("x", it->x);
		effElem->SetAttribute("y", it->y);
		effElem->SetAttribute("zOrder", it->zOrder);
	}
}

EffectRyushka::EffectRyushka(rapidxml::xml_node<> *xmlElement)
    : Ryushka(xmlElement, 1.0f)
    , WIDTH(550)
    , HEIGHT(550)
    , _editorRect(IPoint(0, 0))
{
	if (xmlElement->first_attribute("width"))
	{
		WIDTH = utils::lexical_cast<int> (std::string(xmlElement->first_attribute("width")->value()));
		HEIGHT = utils::lexical_cast<int> (std::string(xmlElement->first_attribute("height")->value()));
	}
    rapidxml::xml_node<>* positionXml = xmlElement->first_node("Position");
    Assert(positionXml != NULL);
    IPoint position(positionXml);
    _rect.x = position.x - WIDTH / 2;
    _rect.y = position.y - HEIGHT / 2;
    _rect.width = WIDTH;
    _rect.height = HEIGHT;

    rapidxml::xml_node<> *effectElem = xmlElement->first_node("Effect");
    while (effectElem) {
        EffectDesc desc;
        desc.name = effectElem->first_attribute("name")->value();
        desc.x = utils::lexical_cast<int>(std::string(effectElem->first_attribute("x")->value()));    
        desc.y = utils::lexical_cast<int>(std::string(effectElem->first_attribute("y")->value()));    
        desc.zOrder = utils::lexical_cast<int>(std::string(effectElem->first_attribute("zOrder")->value()));    
        

        if (effectElem->first_attribute("distance"))
        {
            desc.distance = utils::lexical_cast<int>(std::string(effectElem->first_attribute("distance")->value()));    
        } else {
            desc.distance = 0;
        }
        
        ParticleEffect *eff = _effects.AddEffect(desc.name);
		int width = 0;
		int height = 0;
		for (size_t i = 0; i < eff->_systems.size(); i++)
		{
			int w = eff->_systems[i]->FrameWidth();
			int h = eff->_systems[i]->FrameHeight();
			if (w > width)
			{
				width = w;
			}
			if (h > height)
			{
				height = h;
			}
		}
        eff->posX = (float)desc.x + _rect.x + _rect.width / 2;
        eff->posY = (float)desc.y + _rect.y + _rect.height / 2;
        eff->Reset();

        desc.eff = eff;

        _descs.push_back(desc);

        effectElem = effectElem->next_sibling("Effect");
    }
}

void EffectRyushka::Update(float dt)
{
	_effects.Update(dt);
}

void EffectRyushka::Upload()
{
    for (size_t i = 0; i < _descs.size(); i++)
    {
        effectPresets.UploadEffect(_descs[i].name);
    }
}

void EffectRyushka::Release()
{
    for (size_t i = 0; i < _descs.size(); i++)
    {
        effectPresets.ReleaseEffect(_descs[i].name);
    }
}

void EffectRyushka::OnDraw()
{
    if (_editorRect.x != 0 || _editorRect.y != 0)
    {
        Render::device.PushMatrix();
        Render::device.MatrixTranslate(math::Vector3(_editorRect.x + 0.f, _editorRect.y + 0.f, 0.f));
        _effects.Draw();
        Render::device.PopMatrix();
    } else {
        _effects.Draw();
    }

}

void EffectRyushka::SaveToXml(Xml::TiXmlElement *parentElem)
{
    Xml::TiXmlElement *elem = Ryushka::CreateXmlElement(parentElem, "EffectRyushka");

	if (WIDTH != 550 || HEIGHT != 550)
	{
		//по алфавиту!
		Xml::SetIntAttribute(elem, "height", HEIGHT);		
		Xml::SetIntAttribute(elem, "width", WIDTH);
	}
    //Xml::TiXmlElement *texElem = elem->InsertEndChild(Xml::TiXmlElement("Texture"))->ToElement();
    //texElem->SetAttribute("name", _textureName);

    Xml::TiXmlElement *posElem = elem->InsertEndChild(Xml::TiXmlElement("Position"))->ToElement();
    //posElem->SetAttribute("x", _rect.x  + _tex->getBitmapRect().width / 2);
    //posElem->SetAttribute("y", _rect.y  + _tex->getBitmapRect().height / 2);
    Xml::SetIntAttribute(posElem, "x", _rect.x + _editorRect.x + WIDTH / 2);
    Xml::SetIntAttribute(posElem, "y", _rect.y + _editorRect.y + HEIGHT / 2);

    for (std::vector<EffectDesc>::iterator it = _descs.begin() ; it != _descs.end() ; ++it)
    {
        Xml::TiXmlElement *effElem = elem->InsertEndChild(Xml::TiXmlElement("Effect"))->ToElement();
		//по алфавиту!
        effElem->SetAttribute("distance", it->distance);
        effElem->SetAttribute("name", it->name);
        effElem->SetAttribute("x", it->x);
        effElem->SetAttribute("y", it->y);
        effElem->SetAttribute("zOrder", it->zOrder);
    }
}

IRect EffectRyushka::getClientRect() {
    return IRect(_rect.x + _editorRect.x, _rect.y + _editorRect.y, _rect.width, _rect.height);
}

IRect EffectRyushka::getVisibleRect()
{
	return IRect(_rect.x + _editorRect.x, _rect.y + _editorRect.y, _rect.width, _rect.height);
}

IRect EffectRyushka::getEditRect() 
{
    IRect rect(_rect);
    rect.Inflate(-150);
    rect.x += _editorRect.x;
    rect.y += _editorRect.y;        
    return rect;
}

void EffectRyushka::setPosition(const IPoint& position) {
    //_rect = _rect.MoveTo(position);
    _editorRect = position;
    _editorRect.x -= _rect.x;
    _editorRect.y -= _rect.y;
}

RyushkaLamp::RyushkaLamp(rapidxml::xml_node<> *xmlElement)
	: StaticImage(xmlElement)
{
	rapidxml::xml_node<> *effectElem = xmlElement->first_node("Effect");
	_nameEff = effectElem->first_attribute("name")->value();

	ParticleEffect *eff = _effects.AddEffect(_nameEff);
	eff->posX = 0;
	eff->posY = 0;
	eff->Reset();

	rapidxml::xml_node<> *parElem = xmlElement->first_node("Effect");
	_pendant = new Pendant(parElem, this);

	_visibleRect = _rect;
	_visibleRect.x -= _pendant->_pStart.x;
	_visibleRect.y -= _pendant->_pStart.y;
	_visibleRect.width += _pendant->_pStart.x;
	_visibleRect.height += _pendant->_pStart.y;
}

RyushkaLamp::~RyushkaLamp()
{
	delete _pendant;
}

void RyushkaLamp::Upload()
{
	StaticImage::Upload();
	effectPresets.UploadEffect(_nameEff);
}

void RyushkaLamp::Release()
{
	StaticImage::Release();
	effectPresets.ReleaseEffect(_nameEff);
}

IRect RyushkaLamp::getVisibleRect()
{
	return _visibleRect;
}

void RyushkaLamp::MouseMove(const IPoint &pos)
{
	IPoint p = CorrectionMove(pos) - _pendant->_pos;
	_pendant->MouseMove(p,  GetAngle(), GetScale());
}

void RyushkaLamp::Update(float dt)
{
	_effects.Update(dt);
	_pendant->Update(dt);
}
void RyushkaLamp::setPosition(const IPoint &pos)
{
	StaticImage::setPosition(pos);

	_visibleRect = _rect;
	_visibleRect.x -= _pendant->_pStart.x;
	_visibleRect.y -= _pendant->_pStart.y;
	_visibleRect.width += _pendant->_pStart.x;
	_visibleRect.height += _pendant->_pStart.y;
}

void RyushkaLamp::OnDraw()
{
	StaticImage::OnDraw();
	Render::device.PushMatrix();
	IPoint p = _pendant->_pos + _pendant->_pStart;
	p.x += _rect.x;
	p.y += _rect.y;


	Render::device.MatrixTranslate(math::Vector3(p));
	Render::device.MatrixRotate(math::Vector3(0, 0, 1.0f), 	_pendant->GetDx());
	Render::device.MatrixTranslate(math::Vector3(-_pendant->_pStart));
	_effects.Draw();
	_pendant->Draw();
	Render::device.PopMatrix();
}

void RyushkaLamp::SaveToXml(Xml::TiXmlElement *parentElem)
{
	Xml::TiXmlElement *elem = Ryushka::CreateXmlElement(parentElem, "RyushkaLamp");

	Xml::TiXmlElement *texElem = elem->InsertEndChild(Xml::TiXmlElement("Texture"))->ToElement();
	texElem->SetAttribute("name", _textureName);

	Xml::TiXmlElement *posElem = elem->InsertEndChild(Xml::TiXmlElement("Position"))->ToElement();
	posElem->SetAttribute("x", _rect.x  + _tex->getBitmapRect().width / 2);
	posElem->SetAttribute("y", _rect.y  + _tex->getBitmapRect().height / 2);

	Xml::TiXmlElement *effElem = elem->InsertEndChild(Xml::TiXmlElement("Effect"))->ToElement();
	effElem->SetAttribute("name", _nameEff);

	_pendant->SaveToXml(effElem);
}


TreeImage::TreeImage(rapidxml::xml_node<> *xmlElement)
: StaticImage(xmlElement)
{
	rapidxml::xml_node<> *leafElem = xmlElement->first_node("Leaf");
	while (leafElem)
	{
		LeafDesc desc;

		desc.texName = leafElem->first_attribute("texture")->value();
		desc.x = utils::lexical_cast<int>(std::string(leafElem->first_attribute("x")->value()));	
		desc.y = utils::lexical_cast<int>(std::string(leafElem->first_attribute("y")->value()));	
		desc.cx = utils::lexical_cast<int>(std::string(leafElem->first_attribute("cx")->value()));	
		desc.cy = utils::lexical_cast<int>(std::string(leafElem->first_attribute("cy")->value()));	
		desc.angle = utils::lexical_cast<int>(std::string(leafElem->first_attribute("angle")->value()));	

		desc.tex = getTexture(desc.texName);
		desc.time = math::random(0.0f, 6.0f);
		_descs.push_back(desc);

		leafElem = leafElem->next_sibling("Leaf");
	}
}


void TreeImage::Update(float dt)
{
	for (std::vector<LeafDesc>::iterator it = _descs.begin() ; it != _descs.end() ; ++it)
	{
		it->time += dt;
	}
}

void TreeImage::OnDraw()
{
	StaticImage::OnDraw();
	Render::device.PushMatrix();
	Render::device.MatrixTranslate(math::Vector3((float)(_rect.x + _rect.width / 2), (float)(_rect.y + _rect.height / 2), 0.0f));
	for (std::vector<LeafDesc>::iterator it = _descs.begin() ; it != _descs.end() ; ++it)
	{
		Render::device.PushMatrix();
		Render::device.MatrixTranslate(math::Vector3((float)it->cx, (float)it->cy, 0.0f));
		Render::device.MatrixRotate(math::Vector3(0.0f, 0.0f, 1.0f), (float)(it->angle) * sinf(it->time));
		Render::device.MatrixTranslate(-math::Vector3((float)it->cx, (float)it->cy, 0.0f));
		Render::device.MatrixTranslate(math::Vector3((float)it->x, (float)it->y, 0.0f));
		it->tex->Bind();
		it->tex->Draw(IPoint(-it->tex->getBitmapRect().width / 2, -it->tex->getBitmapRect().height / 2));
		Render::device.PopMatrix();
	}
	Render::device.PopMatrix();
}

void TreeImage::SaveToXml(Xml::TiXmlElement * parentElem)
{
	Xml::TiXmlElement *elem = parentElem->InsertEndChild(Xml::TiXmlElement("TreeImage"))->ToElement();

	Xml::TiXmlElement *texElem = elem->InsertEndChild(Xml::TiXmlElement("Texture"))->ToElement();
	texElem->SetAttribute("name", _textureName);

	Xml::TiXmlElement *posElem = elem->InsertEndChild(Xml::TiXmlElement("Position"))->ToElement();
	posElem->SetAttribute("x", _rect.x  + _tex->getBitmapRect().width / 2);
	posElem->SetAttribute("y", _rect.y  + _tex->getBitmapRect().height / 2);

	for (std::vector<LeafDesc>::iterator it = _descs.begin() ; it != _descs.end() ; ++it)
	{
		Xml::TiXmlElement *effElem = elem->InsertEndChild(Xml::TiXmlElement("Leaf"))->ToElement();
		effElem->SetAttribute("angle", it->angle);
		effElem->SetAttribute("cx", it->cx);
		effElem->SetAttribute("cy", it->cy);
		effElem->SetAttribute("texture", it->texName);
		effElem->SetAttribute("x", it->x);
		effElem->SetAttribute("y", it->y);
	}
}

FlyImage::FlyImage(rapidxml::xml_node<> *xmlElement)
: StaticImage(xmlElement)
, _time(math::random(0.0f, 3.14f))
{
	_speed = utils::lexical_cast<float>(xmlElement->first_attribute("speed")->value());
	_amplitude = utils::lexical_cast<float>(xmlElement->first_attribute("amplitude")->value());
}

void FlyImage::OnDraw()
{
	Render::device.PushMatrix();
	Render::device.MatrixTranslate(math::Vector3(0.0f, _amplitude * sinf(_time * _speed), 0.0f));
	StaticImage::OnDraw();
	Render::device.PopMatrix();
}

void FlyImage::Update(float dt)
{
	_time += dt;
}

void FlyImage::SaveToXml(Xml::TiXmlElement *parentElem)
{
	Xml::TiXmlElement *elem = Ryushka::CreateXmlElement(parentElem, "FlyImage");
	//по алфавиту!
	elem->SetAttribute("amplitude", utils::lexical_cast(_amplitude));
	elem->SetAttribute("speed", utils::lexical_cast(_speed));

	Xml::TiXmlElement *texElem = elem->InsertEndChild(Xml::TiXmlElement("Texture"))->ToElement();
	texElem->SetAttribute("name", _textureName);

	Xml::TiXmlElement *posElem = elem->InsertEndChild(Xml::TiXmlElement("Position"))->ToElement();
	posElem->SetAttribute("x", _rect.x  + _tex->getBitmapRect().width / 2);
	posElem->SetAttribute("y", _rect.y  + _tex->getBitmapRect().height / 2);
}

LavaImage::LavaImage(rapidxml::xml_node<> *xmlElement)
: StaticImage(xmlElement)
{
	rapidxml::xml_node<> *lavaElem = xmlElement->first_node("Lava");
	while (lavaElem)
	{
		_strips.push_back(Lava());
		Lava &strip = _strips.back();

		strip._time = math::random(0.0f, 6.0f);
		strip._speed = utils::lexical_cast<float>(lavaElem->first_attribute("speed")->value());
		strip._scale = utils::lexical_cast<float>(lavaElem->first_attribute("scale")->value());

		strip._texLavaName = std::string(lavaElem->first_node("LavaTexture")->first_attribute("name")->value());
		strip._texLava = getTexture(strip._texLavaName);

		strip._strip.setTextureSpeed(strip._speed);
		strip._strip.setTextureScale(strip._scale);
		strip._strip.setStripLength(100.0f);

		rapidxml::xml_node<> *pathElem = lavaElem->first_node("Path")->first_node("Key");
		int keys = 0;
		while (pathElem)
		{
			strip._strip.addPathKey(	utils::lexical_cast<float>(pathElem->first_attribute("x")->value()),
										utils::lexical_cast<float>(pathElem->first_attribute("y")->value()),
										utils::lexical_cast<float>(pathElem->first_attribute("h")->value()));
			strip._params.push_back(math::Vector3(utils::lexical_cast<float>(pathElem->first_attribute("x")->value()),
									utils::lexical_cast<float>(pathElem->first_attribute("y")->value()),
									utils::lexical_cast<float>(pathElem->first_attribute("h")->value())));
			pathElem = pathElem->next_sibling();
			keys++;
		}
		strip._strip.CalculateBuffer(keys * 2);
		strip._strip.setStripTime(0.5f);

		lavaElem = lavaElem->next_sibling("Lava");
	}
}

void LavaImage::OnDraw()
{
	for (std::vector<Lava>::iterator i = _strips.begin() ; i != _strips.end() ; ++i)
	{
		i->_texLava->Bind();
		Render::device.SetCurrentMatrix(Render::TEXTURE);
		Render::device.MatrixTranslate(math::Vector3(- i->_time * 0.1f, 0.0f, 0.0f));
		Render::device.SetCurrentMatrix(Render::MODELVIEW);

		Render::device.PushMatrix();
		Render::device.MatrixTranslate(math::Vector3((float)(_rect.x + _rect.width / 2), (float)(_rect.y + _rect.height / 2), 0.0f));
		i->_strip.setColor(Render::device.GetCurrentColor());
		i->_strip.DrawSimple();
		Render::device.PopMatrix();
		
		Render::device.SetCurrentMatrix(Render::TEXTURE);
		Render::device.ResetMatrix();
		Render::device.SetCurrentMatrix(Render::MODELVIEW);
	}
	_tex->Bind();
	Render::DrawRect(_rect, u1, u2, v1, v2, Normal);
}

void LavaImage::Update(float dt)
{
	for (std::vector<Lava>::iterator i = _strips.begin() ; i != _strips.end() ; ++i)
	{
		i->_time += dt * i->_speed;
	}
}

void LavaImage::SaveToXml(Xml::TiXmlElement *parentElem)
{
	Xml::TiXmlElement *elem = Ryushka::CreateXmlElement(parentElem, "LavaImage");
		
	Xml::TiXmlElement *texElem = elem->InsertEndChild(Xml::TiXmlElement("Texture"))->ToElement();
	texElem->SetAttribute("name", _textureName);

	Xml::TiXmlElement *posElem = elem->InsertEndChild(Xml::TiXmlElement("Position"))->ToElement();
		posElem->SetAttribute("x", _rect.x  + _tex->getBitmapRect().width / 2);
		posElem->SetAttribute("y", _rect.y  + _tex->getBitmapRect().height / 2);

	for (std::vector<Lava>::iterator i = _strips.begin() ; i != _strips.end() ; ++i)
	{
		Xml::TiXmlElement *lavaElem = elem->InsertEndChild(Xml::TiXmlElement("Lava"))->ToElement();

		texElem = lavaElem->InsertEndChild(Xml::TiXmlElement("LavaTexture"))->ToElement();
		texElem->SetAttribute("name", i->_texLavaName);

		lavaElem->SetAttribute("scale", utils::lexical_cast(i->_scale));
		lavaElem->SetAttribute("speed", utils::lexical_cast(i->_speed));

		Xml::TiXmlElement *pathElem = lavaElem->InsertEndChild(Xml::TiXmlElement("Path"))->ToElement();
		for (size_t j = 0 ; j < i->_params.size() ; ++j)
		{
			Xml::TiXmlElement *keyElem = pathElem->InsertEndChild(Xml::TiXmlElement("Key"))->ToElement();
			// по алфавиту!
			keyElem->SetAttribute("h", utils::lexical_cast(i->_params[j].z));
			keyElem->SetAttribute("x", utils::lexical_cast(i->_params[j].x));
			keyElem->SetAttribute("y", utils::lexical_cast(i->_params[j].y));
		}
	}
}

FlagImage::FlagImage(rapidxml::xml_node<> *xmlElement)
: StaticImage(xmlElement)
, _time(math::random(0.0f, 6.38f))
{
	_steps = utils::lexical_cast<int> (std::string(xmlElement->first_attribute("steps")->value()));
	_freq = utils::lexical_cast<float>(xmlElement->first_attribute("freq")->value());

	float step = 1.0f / (float)_steps;
	_buffer._buffer.reserve((_steps + 1) * 2);

	FRect uv(0,1,0,1);
	FRect rect(0,1,0,1);
	_tex->TranslateUV(rect, uv);
	float v = uv.yEnd;
	float u = uv.xEnd;

	for (int i = 0; i < (_steps + 1); ++i)
	{
		_buffer._buffer.push_back(Render::QuadVert((float)(_tex->getBitmapRect().width) * step * i - _rect.width / 2, (float)_tex->getBitmapRect().height - _rect.height / 2, 0.0f, Color::WHITE, step * i * u, v));
		_buffer._buffer.push_back(Render::QuadVert((float)(_tex->getBitmapRect().width) * step * i - _rect.width / 2, 0.0f - _rect.height / 2, 0.0f, Color::WHITE, step * i * u, 0.0f));
	}
	_buffer.numVertices = _steps * 2 + 2;

	if (xmlElement->first_attribute("ampl"))
		_ampl = utils::lexical_cast<float>(xmlElement->first_attribute("ampl")->value());
	else
		_ampl = 0.4f * (float)_tex->getBitmapRect().height;

	if (xmlElement->first_attribute("period"))
		_period = utils::lexical_cast<float>(xmlElement->first_attribute("period")->value());
	else
		_period = (float)_tex->getBitmapRect().width / 3.14f;
}

void FlagImage::OnDraw()
{
	_tex->Bind();
	Render::device.SetCurrentMatrix(Render::MODELVIEW);
	Render::device.PushMatrix();
	Render::device.MatrixTranslate(math::Vector3((float)(_rect.x + _rect.width / 2), (float)(_rect.y + _rect.height / 2), 0.0f));
	Render::device.DrawStrip(&_buffer);
	Render::device.PopMatrix();
}

void FlagImage::Update(float dt)
{
	_time += dt;
	float step = 1.0f / _steps;
	for (int i = 0; i < (_steps + 1); ++i)
	{
		_buffer._buffer[2 * i].y = step * i * _ampl * sinf(_freq * _time + step * i * 3.14f * _period) + (float)_tex->getBitmapRect().height - _rect.height / 2;
		_buffer._buffer[2 * i + 1].y = step * i * _ampl * sinf(_freq * _time + step * i * 3.14f * _period) - _rect.height / 2;

		//_buffer._buffer.push_back(Render::QuadVert((float)(_tex->getBitmapRect().width) * step * i - _rect.width / 2, (float)_tex->getBitmapRect().height - _rect.height / 2, 0.0f, 0xffffffff, step * i * u, v));
		//_buffer._buffer.push_back(Render::QuadVert((float)(_tex->getBitmapRect().width) * step * i - _rect.width / 2, 0.0f - _rect.height / 2, 0.0f, 0xffffffff, step * i * u, 0.0f));
	}
}

void FlagImage::SaveToXml(Xml::TiXmlElement *parentElem)
{
	Xml::TiXmlElement *elem = Ryushka::CreateXmlElement(parentElem, "FlagImage");
	// по алфавиту!
	elem->SetAttribute("ampl", utils::lexical_cast(_ampl));
	elem->SetAttribute("freq", utils::lexical_cast(_freq));
	elem->SetAttribute("period", utils::lexical_cast(_period));
	elem->SetAttribute("steps", _steps);
		
	Xml::TiXmlElement *texElem = elem->InsertEndChild(Xml::TiXmlElement("Texture"))->ToElement();
	texElem->SetAttribute("name", _textureName);

	Xml::TiXmlElement *posElem = elem->InsertEndChild(Xml::TiXmlElement("Position"))->ToElement();
	posElem->SetAttribute("x", _rect.x  + _tex->getBitmapRect().width / 2);
	posElem->SetAttribute("y", _rect.y  + _tex->getBitmapRect().height / 2);
}

LampImage::LampImage(rapidxml::xml_node<> *xmlElement)
: StaticImage(xmlElement)
, _time(math::random(0.0f, 1.0f))
{
	_texLightName = std::string(xmlElement->first_node("LightTexture")->first_attribute("name")->value());
	_texLight = getTexture(_texLightName);

	for (int i = 0 ; i < 10 ; ++i)
		_alpha.addKey(math::random(0.6f, 1.0f));
	_alpha.CalculateGradient(true);
}

void LampImage::OnDraw()
{
	_tex->Bind();
	Render::DrawRect(_rect, u1, u2, v1, v2, Normal);
	_texLight->Bind();

	float alpha = _alpha.getGlobalFrame(_time);
	if (alpha > 1.0f) alpha = 1.0f;
	if (alpha < 0.0f) alpha = 0.0f;

	Render::BeginAlphaMul(alpha);
	Render::DrawRect(_rect, u1, u2, v1, v2, Normal);
	Render::EndAlphaMul();
}

void LampImage::Update(float dt)
{
	_time += dt * 0.2f;
	if (_time > 1.0f)
		_time -= floorf(_time);
}

void LampImage::SaveToXml(Xml::TiXmlElement *parentElem)
{
	Xml::TiXmlElement *elem = Ryushka::CreateXmlElement(parentElem, "LampImage");
		
	Xml::TiXmlElement *texElem = elem->InsertEndChild(Xml::TiXmlElement("Texture"))->ToElement();
	texElem->SetAttribute("name", _textureName);
	texElem = elem->InsertEndChild(Xml::TiXmlElement("LightTexture"))->ToElement();
	texElem->SetAttribute("name", _texLightName);

	Xml::TiXmlElement *posElem = elem->InsertEndChild(Xml::TiXmlElement("Position"))->ToElement();
	posElem->SetAttribute("x", _rect.x  + _tex->getBitmapRect().width / 2);
	posElem->SetAttribute("y", _rect.y  + _tex->getBitmapRect().height / 2);
}

RyushkaBridge::RyushkaBridge(rapidxml::xml_node<>* xmlElement)
	: Ryushka(xmlElement, 1.f)
{	
	rapidxml::xml_node<> *elementTex = xmlElement->first_node("Texture");
	_textureNameBridge = std::string(elementTex->first_attribute("nameBridge")->value());
	_textureNameBg = std::string(elementTex->first_attribute("nameBg")->value());
	_textureNamePim = std::string(elementTex->first_attribute("namePim")->value());
	_texBridge = getTexture(_textureNameBridge);
	_texBg = getTexture(_textureNameBg);
	_texPim = getTexture(_textureNamePim);
	rapidxml::xml_node<> *elementPos = xmlElement->first_node("Position");
	_rect.x = utils::lexical_cast<int>(std::string(elementPos->first_attribute("x")->value()));
	_rect.y = utils::lexical_cast<int>(std::string(elementPos->first_attribute("y")->value()));
	_rect.width = (int)_texBridge->getBitmapRect().width;
	_rect.height = (int)_texBridge->getBitmapRect().height;
	
	_pendant = new Pendant(elementTex, this);
	
	FRect uv(0, 1, 0, 1);
	if (_texBridge->needTranslate()) {
        FRect rect(0, 0, _rect.width + 0.f, _rect.height + 0.f);
		_texBridge->TranslateUV( rect, uv);
	}

	_mesh = new Mesh(Mesh::BRIDGE);
	_mesh->Init(_rect, uv);
	_mesh->InitK(_pendant->_pStart, _pendant->_pEnd);
}

RyushkaBridge::~RyushkaBridge()
{
	delete _pendant;
	delete _mesh;
}

void RyushkaBridge::Update(float dt)
{
	_pendant->Update(dt);
	_mesh->Update(_pendant->GetDx());
}

void RyushkaBridge::OnDraw()
{	
	Render::device.PushMatrix();
	math::Vector3 offset ((float) _rect.x, (float) _rect.y, 0.0f);
	Render::device.MatrixTranslate(offset);
	DrawTextureCentered(_texBg, IPoint(_rect.width/2, _rect.height/2), true);
	_texBridge->Bind();
	_mesh->Draw();
	_pendant->Draw();
	DrawTextureCentered(_texPim, IPoint(_rect.width/2, _rect.height/2), true);
	Render::device.PopMatrix();
}

void RyushkaBridge::MouseMove(const IPoint &pos){

	IPoint p = CorrectionMove(pos);
	_pendant->MouseMove(p, GetAngle(), GetScale() );
}

void RyushkaBridge::SaveToXml(Xml::TiXmlElement *xnlElement)
{
	Xml::TiXmlElement *elem = Ryushka::CreateXmlElement(xnlElement, "RyushkaBridge");
	Xml::TiXmlElement *texElem = elem->InsertEndChild(Xml::TiXmlElement("Texture"))->ToElement();
	texElem->SetAttribute("nameBg", _textureNameBg);
	texElem->SetAttribute("nameBridge", _textureNameBridge);
	texElem->SetAttribute("namePim", _textureNamePim);
	_pendant->SaveToXml(texElem);
	Xml::TiXmlElement *posElem = elem->InsertEndChild(Xml::TiXmlElement("Position"))->ToElement();
	posElem->SetAttribute("x", _rect.x );
	posElem->SetAttribute("y", _rect.y );
}



void RyushkaBridge::setPosition(const IPoint &pos)
{
	_rect.x = pos.x;
	_rect.y = pos.y;
}

IRect RyushkaBridge::getClientRect()
{
	return _rect;
}

IRect RyushkaBridge::getVisibleRect()
{
	return _rect;
}


RyushkaList::RyushkaList(rapidxml::xml_node<>* xmlElement)
	: RyushkaCone(xmlElement)

{
	rapidxml::xml_node<> *element = xmlElement->first_node("Cone");
	if (element->first_attribute("ampl"))
	{
		_ampl = utils::lexical_cast<float> (std::string(element->first_attribute("ampl")->value()));
	} else {
		_ampl = 10;
	}
	if (element->first_attribute("fr"))
	{
		_fr = utils::lexical_cast<float> (std::string(element->first_attribute("fr")->value()));
	} else {
		_fr = 1;
	}
	_timer = math::random(10.f, 30.f);
}

void RyushkaList::Update(float dt){
	_timer -= dt;
	if (_timer < 0)
	{
		_timer = 0.f;
		_timer = math::random(10.f, 30.f);
		SetState(STATE_FALL);
	}

	if (_state == STATE_FALL)
	{
		_timerFall += dt;
		_p.x = -_ampl*math::sin(_timerFall*_fr + _f)*math::min(1.f, _timerFall);
	}

	RyushkaCone::Update(dt);
}

void RyushkaList::MouseMove(const IPoint &pos)
{
	// еcли попали
	MyAssert(!EditorUtils::editor);

	IPoint p = CorrectionMove(pos);
	if (_tex->isPixelOpaque(p))
	{
		if (_state == STATE_HANG){
			SetState(STATE_FALL);
		}
	}
}


void RyushkaList::SaveToXml(Xml::TiXmlElement *parentElem)
{
	Xml::TiXmlElement *elem = Ryushka::CreateXmlElement(parentElem, "RyushkaList");
	Xml::TiXmlElement *texElem = elem->InsertEndChild(Xml::TiXmlElement("Texture"))->ToElement();
	texElem->SetAttribute("name", _textureName);
	Xml::TiXmlElement *posElem = elem->InsertEndChild(Xml::TiXmlElement("Position"))->ToElement();
	posElem->SetAttribute("x", _posCenter.x);
	posElem->SetAttribute("y", _posCenter.y);

	Xml::TiXmlElement *texCone = elem->InsertEndChild(Xml::TiXmlElement("Cone"))->ToElement();
	//по алфавиту!
	texCone->SetDoubleAttribute("ampl", (double)_ampl);
	texCone->SetDoubleAttribute("fr", (double)_fr);
	texCone->SetDoubleAttribute("g", -(double)_a.y);
	texCone->SetDoubleAttribute("sRotate", (double)_sRotate);
	texCone->SetDoubleAttribute("timeShow", (double)_timeShow);
	texCone->SetAttribute("xS", _posScale.x);
	texCone->SetAttribute("yS", _posScale.y);
	_timer = 0.f;

}

void RyushkaList::SetState(State state)
{
 	_state = state;
	if (_state == STATE_FALL)
	{
		_timerFall = 0;
		_f = math::random(-math::PI, math::PI);
		
	}
	RyushkaCone::SetState(state);
}


RyushkaWeb::RyushkaWeb(rapidxml::xml_node<>* xmlElement)
	: Ryushka(xmlElement, 1.f)
{	
	rapidxml::xml_node<> *elementTex = xmlElement->first_node("Texture");
	_textureNameBridge = std::string(elementTex->first_attribute("name")->value());
	_texBridge = getTexture(_textureNameBridge);
	rapidxml::xml_node<> *elementPos = xmlElement->first_node("Position");
	_rect.x = utils::lexical_cast<int>(std::string(elementPos->first_attribute("x")->value()));
	_rect.y = utils::lexical_cast<int>(std::string(elementPos->first_attribute("y")->value()));
	_rect.width = (int)_texBridge->getBitmapRect().width;
	_rect.height = (int)_texBridge->getBitmapRect().height;

	rapidxml::xml_node<> *elementLights = xmlElement->first_node("Lights");
	rapidxml::xml_node<> *elementLight = elementLights->first_node("Light");

	_lights_settings.clear();
	
	while(elementLight){
		WebLightSettings light;

		light.texture = getTexture(utils::lexical_cast<std::string>(std::string(elementLight->first_attribute("texture")->value())));
		light.min_alpha = utils::lexical_cast<int>(std::string(elementLight->first_attribute("min_alpha")->value()));
		light.max_alpha = utils::lexical_cast<int>(std::string(elementLight->first_attribute("max_alpha")->value()));
		light.start_alpha = utils::lexical_cast<int>(std::string(elementLight->first_attribute("start_alpha")->value()));
		light.current_alpha = light.start_alpha;
		light.time = utils::lexical_cast<int>(std::string(elementLight->first_attribute("time")->value()));

		_lights_settings.push_back(light);

		elementLight = elementLight -> next_sibling("Light");
	}
	
	_pendant = new Pendant(elementTex, this);
	
	FRect uv(0, 1, 0, 1);
	if (_texBridge->needTranslate()) {
        FRect rect(0, 0, _rect.width + 0.f, _rect.height + 0.f);
		_texBridge->TranslateUV( rect, uv);
	}

	_light_mesh = new MeshWithLights(Mesh::WEB, _lights_settings);
	_light_mesh->Init(_rect, uv);
	_light_mesh->InitK(_pendant->_pStart, _pendant->_pEnd);
}

RyushkaWeb::~RyushkaWeb()
{
	delete _pendant;
	delete _light_mesh;
}

void RyushkaWeb::Update(float dt)
{
	_pendant->Update(dt);
	_light_mesh->Update(_pendant->GetDx());
	_light_mesh->UpdateLights(dt);
}

void RyushkaWeb::OnDraw()
{	
	Render::device.PushMatrix();
	math::Vector3 offset ((float) _rect.x, (float) _rect.y, 0.0f);
	Render::device.MatrixTranslate(offset);
	_texBridge->Bind();
	_light_mesh->Draw();
	_pendant->Draw();

	Render::device.PopMatrix();
}

void RyushkaWeb::MouseMove(const IPoint &pos){

	IPoint p = CorrectionMove(pos);
	_pendant->MouseMove(p, GetAngle(), GetScale() );

}
void RyushkaWeb::SaveToXml(Xml::TiXmlElement *xnlElement)
{
	Xml::TiXmlElement *elem = Ryushka::CreateXmlElement(xnlElement, "RyushkaWeb");
	Xml::TiXmlElement *texElem = elem->InsertEndChild(Xml::TiXmlElement("Texture"))->ToElement();
	texElem->SetAttribute("name", _textureNameBridge);
	_pendant->SaveToXml(texElem);
	Xml::TiXmlElement *posElem = elem->InsertEndChild(Xml::TiXmlElement("Position"))->ToElement();
	posElem->SetAttribute("x", _rect.x );
	posElem->SetAttribute("y", _rect.y );
	Xml::TiXmlElement *lightsElem = elem->InsertEndChild(Xml::TiXmlElement("Lights"))->ToElement();
	
	for(WebLightsSettings::iterator it = _lights_settings.begin(); it != _lights_settings.end(); ++it) {
		Xml::TiXmlElement *lightElem = lightsElem->InsertEndChild(Xml::TiXmlElement("Light"))->ToElement();
		
		// по алфавиту!
		lightElem->SetAttribute("current_alpha", it->current_alpha);
		lightElem->SetAttribute("max_alpha", it->max_alpha);
		lightElem->SetAttribute("min_alpha", it->min_alpha);
		lightElem->SetAttribute("start_alpha", it->start_alpha);
		lightElem->SetAttribute("texture", it->texture->textureID);
		lightElem->SetAttribute("time", it->time);
	}
}

void RyushkaWeb::setPosition(const IPoint &pos)
{
	_rect.x = pos.x;
	_rect.y = pos.y;
}

IRect RyushkaWeb::getClientRect()
{
	return _rect;
}

IRect RyushkaWeb::getVisibleRect()
{
	return _rect;
}


RyushkaLightRope::RyushkaLightRope(rapidxml::xml_node<>* xmlElement)
	: Ryushka(xmlElement, 1.f)
{	
	rapidxml::xml_node<> *elementTex = xmlElement->first_node("Texture");
	_textureName = std::string(elementTex->first_attribute("name")->value());
	_tex = getTexture(_textureName);
	rapidxml::xml_node<> *elementPos = xmlElement->first_node("Position");
	_rect.x = utils::lexical_cast<int>(std::string(elementPos->first_attribute("x")->value()));
	_rect.y = utils::lexical_cast<int>(std::string(elementPos->first_attribute("y")->value()));
	_rect.width = (int)_tex->getBitmapRect().width;
	_rect.height = (int)_tex->getBitmapRect().height;

	rapidxml::xml_node<> *elementLights = xmlElement->first_node("Lights");
	rapidxml::xml_node<> *elementLight = elementLights->first_node("Light");

	_lights_settings.clear();
	
	while(elementLight){
		WebLightSettings light;

		light.texture = getTexture(utils::lexical_cast<std::string>(std::string(elementLight->first_attribute("texture")->value())));
		light.min_alpha = utils::lexical_cast<int>(std::string(elementLight->first_attribute("min_alpha")->value()));
		light.max_alpha = utils::lexical_cast<int>(std::string(elementLight->first_attribute("max_alpha")->value()));
		light.start_alpha = utils::lexical_cast<int>(std::string(elementLight->first_attribute("start_alpha")->value()));
		light.current_alpha = light.start_alpha;
		light.time = utils::lexical_cast<int>(std::string(elementLight->first_attribute("time")->value()));

		_lights_settings.push_back(light);

		elementLight = elementLight -> next_sibling("Light");
	}
	
	_pendant = new Pendant(elementTex, this);
	
	FRect uv(0, 1, 0, 1);
	if (_tex->needTranslate()) {
        FRect rect(0, 0, _rect.width + 0.f, _rect.height + 0.f);
		_tex->TranslateUV( rect, uv);
	}

	_light_mesh = new MeshWithLights(Mesh::CHAIN, _lights_settings);
	_light_mesh->Init(_rect, uv);
	_light_mesh->InitK(_pendant->_pStart, _pendant->_pEnd);
}

RyushkaLightRope::~RyushkaLightRope()
{
	delete _pendant;
	delete _light_mesh;
}

void RyushkaLightRope::Update(float dt)
{
	_pendant->Update(dt);
	_light_mesh->Update(_pendant->GetDx());
	_light_mesh->UpdateLights(dt);
}

void RyushkaLightRope::OnDraw()
{	
	Render::device.PushMatrix();
	math::Vector3 offset ((float) _rect.x, (float) _rect.y, 0.0f);
	Render::device.MatrixTranslate(offset);
	_tex->Bind();
	_light_mesh->Draw();
	_pendant->Draw();

	Render::device.PopMatrix();
}

void RyushkaLightRope::MouseMove(const IPoint &pos){

	IPoint p = CorrectionMove(pos);
	_pendant->MouseMove(p, GetAngle(), GetScale() );

}
void RyushkaLightRope::SaveToXml(Xml::TiXmlElement *xnlElement)
{
	Xml::TiXmlElement *elem = Ryushka::CreateXmlElement(xnlElement, "RyushkaLightRope");
	Xml::TiXmlElement *texElem = elem->InsertEndChild(Xml::TiXmlElement("Texture"))->ToElement();
	texElem->SetAttribute("name", _textureName);
	_pendant->SaveToXml(texElem);
	Xml::TiXmlElement *posElem = elem->InsertEndChild(Xml::TiXmlElement("Position"))->ToElement();
	posElem->SetAttribute("x", _rect.x );
	posElem->SetAttribute("y", _rect.y );
	Xml::TiXmlElement *lightsElem = elem->InsertEndChild(Xml::TiXmlElement("Lights"))->ToElement();
	
	for(WebLightsSettings::iterator it = _lights_settings.begin(); it != _lights_settings.end(); ++it) {
		Xml::TiXmlElement *lightElem = lightsElem->InsertEndChild(Xml::TiXmlElement("Light"))->ToElement();

		// по алфавиту!
		lightElem->SetAttribute("current_alpha", it->current_alpha);
		lightElem->SetAttribute("max_alpha", it->max_alpha);
		lightElem->SetAttribute("min_alpha", it->min_alpha);
		lightElem->SetAttribute("start_alpha", it->start_alpha);
		lightElem->SetAttribute("texture", it->texture->textureID);
		lightElem->SetAttribute("time", it->time);
	}
}

void RyushkaLightRope::setPosition(const IPoint &pos)
{
	_rect.x = pos.x;
	_rect.y = pos.y;
}

IRect RyushkaLightRope::getClientRect()
{
	return _rect;
}

IRect RyushkaLightRope::getVisibleRect()
{
	return _rect;
}

MeshWithLights::MeshWithLights(State state, WebLightsSettings lights_settings)
	: Mesh(state)
{	
	WebLight light;

	_lights.clear();
	for(WebLightsSettings::iterator it = lights_settings.begin(); it != lights_settings.end(); ++it) {
		light.texture = it->texture;
		light.min_alpha = it->min_alpha;
		light.max_alpha = it->max_alpha;
		light.current_alpha = it->current_alpha;
		light.time = it->time;
		light.way = 1;

		light.alpha_speed = (float)(light.max_alpha - light.min_alpha) / light.time;

		_lights.push_back(light);
	}
}

MeshWithLights::~MeshWithLights()
{
	delete _dist;

	for(WebLights::iterator it = _lights.begin(); it != _lights.end(); ++it) {
		delete it->distortion;
	}
}


void MeshWithLights::Init(const IRect& rect, const FRect& uv)
{
	int w = rect.width;
	int h = rect.height;				

	_columnMesh = w/50 + 1;
	_rowMesh = h/50 + 1;

	_dist = new Distortion(_columnMesh + 1, _rowMesh + 1);
	_dist->SetRenderRect(IRect(0, 0, w, h), uv.xStart, uv.xEnd, uv.yStart, uv.yEnd);

	float stepC = w / (float)_columnMesh;
	float stepR = h / (float)_rowMesh;

	for (int i=0; i < _rowMesh+1; i++){
		std::vector<FPoint> column;
		for (int j=0; j < _columnMesh+1; j++){
			FPoint p;
			p.x = j*stepC; 
			p.y = i*stepR;
			column.push_back(p);
		}
		_mesh.push_back(column);
	}

	for(WebLights::iterator it = _lights.begin(); it != _lights.end(); ++it) {
		it->distortion = new Distortion(_columnMesh + 1, _rowMesh + 1);
		it->distortion->SetRenderRect(IRect(0, 0, w, h), uv.xStart, uv.xEnd, uv.yStart, uv.yEnd);
	}
}

void MeshWithLights::InitK(IPoint pStart, IPoint pEnd){
	_pStart = pStart;
	_pEnd = pEnd;
	FPoint 	center;
	float max_l;
	
	if (_state == CHAIN)
	{
		_normal = FPoint(_pEnd-_pStart).Rotated(math::PI/2.0f);
		_normal = _normal/_normal.GetDistanceToOrigin();

		center = FPoint((pStart+pEnd)/2);
		max_l = FPoint(_pStart).GetDistanceTo(center);
		for (int i=0; i <_rowMesh+1; i++){
			std::vector<Grid> column;
			for (int j=0; j<_columnMesh+1; j++){
				Grid grid;
				grid.pos.x = _mesh[i][j].x;
				grid.pos.y = _mesh[i][j].y;
				grid.l = _mesh[i][j].GetDistanceTo(center)-10;
				if ((FPoint(_mesh[i][j]).GetDistanceTo(_pStart) < 10)||
					(FPoint(_mesh[i][j]).GetDistanceTo(_pEnd) < 10))
				{
					grid.k = 0;
				}
				else
				{
					grid.k = math::cos(grid.l/max_l)*math::cos(grid.l/max_l)*math::cos(grid.l/max_l);
				}
				column.push_back(grid);
			}
			_d_mesh.push_back(column);
		}
	}
	else if(_state == WEB)
	{
		_normal = FPoint(_pEnd-_pStart).Rotated(math::PI/2.0f);
		_normal = _normal/_normal.GetDistanceToOrigin();

		center = FPoint((pStart+pEnd)/2);
		max_l = FPoint(_pStart).GetDistanceTo(center);
		for (int i=0; i <_rowMesh+1; i++){
			std::vector<Grid> column;
			for (int j=0; j<_columnMesh+1; j++){
				Grid grid;
				grid.pos.x = _mesh[i][j].x;
				grid.pos.y = _mesh[i][j].y;
				grid.l = _mesh[i][j].GetDistanceTo(center);
				grid.k = grid.l/max_l;

				column.push_back(grid);
			}
			_d_mesh.push_back(column);
		}
	}
}

void MeshWithLights::Update(float dx)
{
	int half_rows = _rowMesh / 2;
	int half_columns = _columnMesh / 2;

	int rows_k, columns_k = 0;
	float tension_k; 

	if (_state == CHAIN)
	{
		for (int i=0; i<_rowMesh+1; i++){
			std::vector<Grid>::iterator column = _d_mesh[i].begin();
			for (int j=0; j<_columnMesh+1; j++, ++column){
				_dist->SetDisplacement(j, i, _normal.x*dx* column->k, _normal.y*dx*column->k, REF_NODE);

				for(WebLights::iterator it = _lights.begin(); it != _lights.end(); ++it) {
					it->distortion->SetDisplacement(j, i, _normal.x*dx* column->k, _normal.y*dx*column->k, REF_NODE);
				}
			}
		}
	}
	else if(_state == WEB)
	{
		for (int i=0; i<_rowMesh+1; i++){
			rows_k = half_rows - math::abs(half_rows - i);
			std::vector<Grid>::iterator column = _d_mesh[i].begin();
			for (int j=0; j<_columnMesh+1; j++, ++column){
				columns_k = half_columns - math::abs(half_columns - j);
				tension_k = ((float)columns_k / _columnMesh) + ((float)rows_k / _rowMesh); 

				_dist->SetDisplacement(j, i, _normal.x*dx* (column->k) * tension_k, _normal.y*dx*column->k * tension_k, REF_NODE);
			
				for(WebLights::iterator it = _lights.begin(); it != _lights.end(); ++it) {
					it->distortion->SetDisplacement(j, i, _normal.x*dx* (column->k) * tension_k, _normal.y*dx*column->k * tension_k, REF_NODE);
				}
			}
		}
	}

}

void MeshWithLights::UpdateLights(float dt){
	for(WebLights::iterator it = _lights.begin(); it != _lights.end(); ++it) {
		it->current_alpha += it->way * dt * it->alpha_speed; 

		if(it->current_alpha >= it->max_alpha){
			it->current_alpha = it->max_alpha;
			it->way *= -1;
		}

		if(it->current_alpha <= it->min_alpha){
			it->current_alpha = it->min_alpha;
			it->way *= -1;
		}
	}
}

void MeshWithLights::Draw()
{
	_dist->Draw();

	for(WebLights::iterator it = _lights.begin(); it != _lights.end(); ++it) {
		it->texture->Bind();

		Render::BeginAlphaMul(it->current_alpha / 255);
		it->distortion->Draw();
		Render::EndAlphaMul();
	}
}


RyushkaWindow::RyushkaWindow(rapidxml::xml_node<>* xmlElement)
	: Ryushka(xmlElement, 1.f)
{
	rapidxml::xml_node<> *elementTex = xmlElement->first_node("Texture");
	_textureName = std::string(elementTex->first_attribute("name")->value());
	_tex = getTexture(_textureName);
	rapidxml::xml_node<> *elementPos = xmlElement->first_node("Position");
	_rect = _tex->getBitmapRect().MovedTo( IPoint(elementPos) );

	rapidxml::xml_node<> *elementBackground = xmlElement->first_node("Background");
	_backgroundTextureName = std::string(elementBackground->first_attribute("texture")->value());
	_backgroundTex = getTexture(_backgroundTextureName);

	rapidxml::xml_node<> *elementWindow = xmlElement->first_node("Window");
	int winX = utils::lexical_cast<int>(std::string(elementWindow->first_attribute("x")->value()));
	int winY = utils::lexical_cast<int>(std::string(elementWindow->first_attribute("y")->value()));
	windowPos = IPoint(winX, winY);

	windowHeight = utils::lexical_cast<int>(std::string(elementWindow->first_attribute("height")->value()));
	windowWidth = utils::lexical_cast<int>(std::string(elementWindow->first_attribute("width")->value()));

	windowRect = IRect((winX + _rect.x), (winY + _rect.y), windowWidth, windowHeight);
}

void RyushkaWindow::Update(float dt)
{
}

void RyushkaWindow::OnDraw()
{
	IRect bitmapRect = _backgroundTex->getBitmapRect();

	windowRect = IRect((windowPos.x + _rect.x), (windowPos.y + _rect.y), windowWidth, windowHeight);
	_backgroundTex->Bind();

	float dx = math::max((_rect.x - GameSettings::fieldX), 0.f);
	float dy = math::max((_rect.y - (windowRect.height + windowPos.y) - GameSettings::fieldY), 0.f);

	float u0 = math::lerp(0.f, ((float)(GameSettings::FIELD_SCREEN_CONST.width - _rect.width)/GameSettings::FIELD_SCREEN_CONST.width), (dx/GameSettings::FIELD_SCREEN_CONST.width));
	float u1 = u0 + ((float)windowRect.width / bitmapRect.width);

	float v0 = math::lerp(0.f, ((float)(GameSettings::FIELD_SCREEN_CONST.height - _rect.height)/GameSettings::FIELD_SCREEN_CONST.height), (dy/GameSettings::FIELD_SCREEN_CONST.height));
	float v1 = v0 + ((float)windowHeight / bitmapRect.height);

	Render::DrawRect(windowRect, u0, u1, v0, v1, Normal);


	_tex->Bind();
	Render::DrawRect(_rect, 0.0f, 1.0f, 0.0f, 1.0f, Normal);
}

void RyushkaWindow::SaveToXml(Xml::TiXmlElement *xnlElement)
{
	Xml::TiXmlElement *elem = Ryushka::CreateXmlElement(xnlElement, "RyushkaWindow");
	Xml::TiXmlElement *texElem = elem->InsertEndChild(Xml::TiXmlElement("Texture"))->ToElement();
	texElem->SetAttribute("name", _textureName);
	Xml::TiXmlElement *posElem = elem->InsertEndChild(Xml::TiXmlElement("Position"))->ToElement();
	posElem->SetAttribute("x", _rect.x );
	posElem->SetAttribute("y", _rect.y );

	Xml::TiXmlElement *backgroundElem = elem->InsertEndChild(Xml::TiXmlElement("Background"))->ToElement();
	backgroundElem->SetAttribute("texture", _backgroundTextureName);

	Xml::TiXmlElement *windowElem = elem->InsertEndChild(Xml::TiXmlElement("Window"))->ToElement(); 
	//по алфавиту!
	windowElem->SetAttribute("height", windowHeight);
	windowElem->SetAttribute("width", windowWidth);
	windowElem->SetAttribute("x", windowPos.x);
	windowElem->SetAttribute("y", windowPos.y);
}

void RyushkaWindow::setPosition(const IPoint &pos)
{
	_rect.x = pos.x;
	_rect.y = pos.y;
}

IRect RyushkaWindow::getClientRect()
{
	return _rect;
}

IRect RyushkaWindow::getVisibleRect()
{
	return _rect;
}