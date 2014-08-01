#include "stdafx.h"
#include "Ryushka.h"
#include "GameField.h"
#include "RyushkaGroup.h"
#include "Game.h"
#include "EditorUtils.h"
#include "ParticleEffectWrapper.h"
#include "Energy.h"

RyushkaGroup::RyushkaGroup(rapidxml::xml_node<>* xmlElement)
	: Ryushka(xmlElement, 1.f)
	, _waitTime(1.f)
{
	_rect.x = utils::lexical_cast<int>(std::string(xmlElement->first_node("Position")->first_attribute("x")->value())); 
	_rect.y = utils::lexical_cast<int>(std::string(xmlElement->first_node("Position")->first_attribute("y")->value())); 

	_visibleRect.x = _rect.x;
	_visibleRect.y = _rect.y;

	rapidxml::xml_node<>* element = xmlElement->first_node();
	
	_onEnergy = Bool::Parse(Xml::GetStringAttributeOrDef(xmlElement, "energy", "false"));

	IPoint minPos(0,0);
	IPoint maxPos(0,0);

	IPoint minVisiblePos(0,0);
	IPoint maxVisiblePos(0,0);

	while (element != NULL)
	{
		RyushkaGroupItem  *ryushkaItem;
		std::string  type = element->name();
		if (type == "Texture") {
			ryushkaItem =  new ItemTexture(element , this);
		}else if (type == "Tree") {
			ryushkaItem =  new ItemTree(element , this);
		}else if (type == "PendantEffect") {
			ryushkaItem =  new ItemPendantEffect(element , this);
		}else if (type == "PendantTexture") {
			ryushkaItem =  new ItemPendantTexture(element , this);
		}else if (type == "Effect") {
			ryushkaItem =  new ItemEffect(element , this);
		}else if (type == "Flag") {
			ryushkaItem =  new ItemFlag(element , this);
		}else if (type == "Butterfly") {
			ryushkaItem =  new Butterfly::ItemButterfly(element , this);
		}else if (type == "TextureMove") {
			ryushkaItem =  new TextureMove(element , this);
		}else if (type == "EffectMove") {
			ryushkaItem =  new EffectMove(element , this);
		}else if (type == "Chain") {
			ryushkaItem =  new ItemChain(element , this);
		}else if (type == "Text") {
			ryushkaItem =  new ItemText(element , this);
		}else if (type == "WaterPlant")
		{
			ryushkaItem =  new ItemWaterPlant(element , this);

		}
		else{
			element = element ->next_sibling();
			continue;
		}
		_items.push_back(ryushkaItem);
		IRect rect = ryushkaItem->getClientRect();
		minPos.x = math::min(rect.x, minPos.x);
		minPos.y = math::min(rect.y, minPos.y);
		maxPos.x = math::max(rect.x+ rect.width, maxPos.x);
		maxPos.y = math::max(rect.y+ rect.height, maxPos.y);

		IRect rectVisible = ryushkaItem->getVisibleRect();
		minVisiblePos.x = math::min(rectVisible.x, minVisiblePos.x);
		minVisiblePos.y = math::min(rectVisible.y, minVisiblePos.y);
		maxVisiblePos.x = math::max(rectVisible.x+ rectVisible.width, maxVisiblePos.x);
		maxVisiblePos.y = math::max(rectVisible.y+ rectVisible.height, maxVisiblePos.y);
		element = element -> next_sibling();

	}
	_rect.width = maxPos.x - minPos.x;
	_rect.height = maxPos.y - minPos.y;

	_visibleRect.x += minVisiblePos.x;
	_visibleRect.y += minVisiblePos.y;
	_visibleRect.width = maxVisiblePos.x - minVisiblePos.x;
	_visibleRect.height = maxVisiblePos.y - minVisiblePos.y;

	_isActivEnergy = false;


	_wait = false;
	_timer = 0.f;

}



void RyushkaGroup::SaveToXml(Xml::TiXmlElement *parentElem)
{
	Xml::TiXmlElement *elem = Ryushka::CreateXmlElement(parentElem, "RyushkaGroup");
	if (_onEnergy)
	{
		Xml::SetBoolAttribute(elem, "energy", _onEnergy);
	}

	Xml::SetIntAttribute(elem,"radius", GetRadius());

	for(Iterator it = _items.begin(); it != _items.end(); ++it ) {
		Xml::TiXmlElement *itemElem = elem->InsertEndChild(Xml::TiXmlElement((*it)->_type.c_str()))->ToElement();
		(*it)->SaveToXml(itemElem);
	}
	Xml::TiXmlElement *posElem = elem->InsertEndChild(Xml::TiXmlElement("Position"))->ToElement();
	posElem->SetAttribute("x", _rect.x );
	posElem->SetAttribute("y", _rect.y );


}

void RyushkaGroup::Upload()
{
	Ryushka::Upload();
	for(Iterator it = _items.begin(); it != _items.end(); ++it ) 
	{
		(*it)->Upload();
	}
}

void RyushkaGroup::Release()
{
	Ryushka::Release();
	for(Iterator it = _items.begin(); it != _items.end(); ++it ) 
	{
		(*it)->Release();
	}
}

void RyushkaGroup::Update(float dt)
{
	UpdateState();
	 for(Iterator it = _items.begin(); it != _items.end(); ++it ) {
		 if ((*it)->_isActiv){	
			 (*it)->Update(dt);
		 }
	 }
	// ожидание - cекунда, перед обновлением (что бы эффекты не запуcкалиcь вне экрана, когда их не видно)
	if (_wait)
	{
		_timer+= dt;
		if (_timer > _waitTime)
		{
			_wait = false;
			_timer = 0.f;
			for(Iterator it = _items.begin(); it != _items.end(); ++it ) {
				(*it)->SetEnergy();
					
			}
		}
	}
}

FPoint RyushkaGroup::GetDeltaPos(){
	FPoint dp(0, 0);
	 for(Iterator it = _items.begin(); it != _items.end(); ++it ) {
		 dp +=  (*it)->GetDeltaPos();
	 }
	 return dp;
}

void RyushkaGroup::setPosition(const IPoint &pos)
{
	_visibleRect.x += pos.x - _rect.x;
	_visibleRect.y += pos.y - _rect.y;

	_rect.x = pos.x;
	_rect.y = pos.y;
}

IRect RyushkaGroup::getClientRect()
{
	return _rect;
}

IRect RyushkaGroup::getVisibleRect()
{
	return _visibleRect;
}

void RyushkaGroup::OnDraw()
{		
	Render::device.PushMatrix();
	math::Vector3 offset ((float) _rect.x, (float) _rect.y, 0.0f);
	Render::device.MatrixTranslate(offset);

	 for(Iterator it = _items.begin(); it != _items.end(); ++it ) {
		 if ((*it)->_isActiv){
			(*it)->Draw();
		} else
		 {
			 (*it)->DrawCenter();
		}
	} 
	Render::device.PopMatrix();

}

void RyushkaGroup::DrawRamka()
{
	IRect r = getVisibleRect();
	
	if (_onEnergy)
	{
		Render::device.SetTexturing(false);

		IRect r = getClientRect();

		Render::BeginColor(Color(127, 255, 0, 100));

		int x = (r.x + r.width/2);
		int y = (r.y + r.height/2);
		
		int radius = GetRadius() * GameSettings::SQUARE_SIDE;

		IRect draw (x - radius, y - radius, radius * 2, radius * 2);
		
		Render::DrawRect(draw);

		Render::EndColor();
		Render::device.SetTexturing(true);
	}

	Render::device.SetTexturing(false);
	Render::DrawFrame(r);
	Render::device.SetTexturing(true);

	Render::FreeType::BindFont("debug");
	Render::PrintString(r.x, r.y, Int::ToString(GetId()), 1.f, LeftAlign, BottomAlign);			
	if (Ryushka::HardPtr connect = GetConnect().lock()) 
	{
		Render::PrintString(r.x, r.y-20, Int::ToString(connect->GetId()), 1.f, LeftAlign, BottomAlign);			
	}
	Render::PrintString(r.x, r.y + 20, "_radius: " + Int::ToString(GetRadius()), 1.f,  LeftAlign, BottomAlign);
}

void RyushkaGroup::Reset(){
	 for(Iterator it = _items.begin(); it != _items.end(); ++it ) {
		(*it)->Reset();
	 }
	_isActivEnergy = false;

}

void RyushkaGroup::MouseMove(const IPoint &pos){
	IPoint p = CorrectionMove(pos);
	for(Iterator it = _items.begin(); it != _items.end(); ++it ) {
		if ((*it)->_isActiv)
		{
			(*it)->MouseMove(p);
		}
	}
}





void RyushkaGroup::MouseDown(const IPoint &pos)
{
	IPoint p = CorrectionMove(pos);
	for(Iterator it = _items.begin(); it != _items.end(); ++it ) {
		if ((*it)->_isActiv)
		{
			(*it)->MouseDown(p);
		}
	}
}

void RyushkaGroup::AcceptMessage(const Message &message)
{
	if( message.is("ActivateRyushka", _name)){
		_wait = true;
		_timer = _waitTime;
		_isActivEnergy = true;
	}
}

void RyushkaGroup::UpdateState()
{
	//еcли ещЄ эрнерги€ не подошла или еcть реакци€ на энергию
	if (!_isActivEnergy && _onEnergy)
	{
		int r = GetRadius();

		IRect rect = getClientRect();
		int x = (rect.x+rect.width/2)/GameSettings::SQUARE_SIDE;
		int y = (rect.y+rect.height/2)/GameSettings::SQUARE_SIDE;

		for (int i = -r; i <= r; i++)
		{
			for (int j = -r; j <= r; j++)
			{
				Game::FieldAddress address = Game::FieldAddress( x +i, y+j);
				if (address.IsValid())
				{				
					if (Energy::field.EnergyExists(address))
					{
						_wait = true;
						_isActivEnergy = true;
						return;
					}
				}
			}
		}
	}

}

RyushkaGroupItem::RyushkaGroupItem(rapidxml::xml_node<>* xmlElement, Ryushka *parent)
{
	_parent = parent;

	std::string  state = Xml::GetStringAttributeOrDef(xmlElement, "state_on", "");

	if (state == "shaw"){	
		_stateOnEnergy = SHAW;
	}
	else if (state == "hide"){
		_stateOnEnergy = HIDE;
	}
	else if (state == "reset"){
		_stateOnEnergy = RESET;
	}
	else {
		_stateOnEnergy = FREE;
	}
	ResetEnergy();

	_type = xmlElement->name();

}

void RyushkaGroupItem::Upload()
{
}

void RyushkaGroupItem::Release()
{
}

void RyushkaGroupItem::Reset(){
	ResetEnergy();

}

FPoint RyushkaGroupItem::GetDeltaPos()
{
	return FPoint(0, 0);
}

bool RyushkaGroupItem::IsEffect(){
	return false;
}

void RyushkaGroupItem::SetEnergy()
{
	if (_stateOnEnergy == HIDE)
	{
		_isActiv = false;

	}
	else
	{
		_isActiv = true;
	}
}

void RyushkaGroupItem::ResetEnergy()
{
	if (_stateOnEnergy == SHAW)
	{
		_isActiv = false;
	}
	else
	{
		_isActiv = true;
	}
}
IRect RyushkaGroupItem::getClientRect()
{
	return _rect;
}

IRect RyushkaGroupItem::getVisibleRect()
{
	return _rect;
}

void RyushkaGroupItem::SaveToXml(Xml::TiXmlElement *xmlElement)
{
	if (_stateOnEnergy == SHAW)
	{
		xmlElement->SetAttribute("state_on", "shaw");
	}
	else if (_stateOnEnergy == HIDE)
	{
		xmlElement->SetAttribute("state_on", "hide");
	}
	else if (_stateOnEnergy == RESET)
	{
		xmlElement->SetAttribute("state_on", "reset");
	}
}



ItemTexture::ItemTexture(rapidxml::xml_node<> *xmlElement, Ryushka *parent)
	: RyushkaGroupItem(xmlElement, parent)
{


	_name = std::string(xmlElement->first_attribute("name")->value());
	_tex = _parent->getTexture(_name);
	_rect.x = utils::lexical_cast<int>(std::string(xmlElement->first_attribute("x")->value()));
	_rect.y = utils::lexical_cast<int>(std::string(xmlElement->first_attribute("y")->value()));
	_rect.width = (int)_tex->getBitmapRect().width;
	_rect.height = (int)_tex->getBitmapRect().height;

}

void ItemTexture::SaveToXml(Xml::TiXmlElement *xmlElement)
{

	RyushkaGroupItem::SaveToXml(xmlElement);
	Xml::SetStringAttribute(xmlElement, "name", _name);
	xmlElement->SetAttribute("x", _rect.x);
	xmlElement->SetAttribute("y", _rect.y);
}

void ItemTexture::Draw()
{
	_tex->Draw(_rect, 0, 1, 0, 1);
}


ItemText::ItemText(rapidxml::xml_node<> *xmlElement, Ryushka *parent)
	: RyushkaGroupItem(xmlElement, parent)
{
	_name = std::string(xmlElement->first_attribute("name")->value());
	_text = Core::resourceManager.Get<Render::Text>(_name);
	//_text->Update();
	//_text->TrueUpdate();
	_rect.x = utils::lexical_cast<int>(std::string(xmlElement->first_attribute("x")->value()));
	_rect.y = utils::lexical_cast<int>(std::string(xmlElement->first_attribute("y")->value()));
	_rect.width = 100;
	_rect.height = 100;

	_visibleRect = _rect;

	_visibleRect.width = _text->GetSize().x;
	_visibleRect.height = 150;

	_visibleRect.x -= _visibleRect.width/2;
	_visibleRect.y -= _visibleRect.height/2;
	_visibleRect.width += _visibleRect.width/2;
	_visibleRect.height += _visibleRect.height/2;

}

IRect ItemText::getVisibleRect()
{
	return _visibleRect;
}

void ItemText::SaveToXml(Xml::TiXmlElement *xmlElement)
{
	RyushkaGroupItem::SaveToXml(xmlElement);
	Xml::SetStringAttribute(xmlElement, "name", _name);
	xmlElement->SetAttribute("x", _rect.x);
	xmlElement->SetAttribute("y", _rect.y);
}

void ItemText::Draw()
{
	_text->Draw(IPoint(_rect.x, _rect.y));
}




ItemTree::ItemTree(rapidxml::xml_node<>* xmlElement, Ryushka *parent)
	: ItemTexture(xmlElement, parent)
{
	_pendant = new Pendant(xmlElement, parent);
	_mesh = new Mesh(Mesh::TREE);
	
	FRect uv(0, 1, 0, 1);
   
	if (_tex->needTranslate()) {
         FRect rect(0, _rect.width, 0.f, _rect.height + 0.f);
		_tex->TranslateUV( rect, uv);
        _rect.x = rect.xStart;
        _rect.width = rect.xEnd - rect.xStart;
        _rect.y = rect.yStart;
        _rect.height = rect.yEnd - rect.yStart;
	}
	
	_mesh->Init(_rect, uv);
	_mesh->InitK(_pendant->_pStart, _pendant->_pEnd);
}

ItemTree::~ItemTree()
{
	delete _mesh;
	delete _pendant;
}

void ItemTree::SaveToXml(Xml::TiXmlElement *xmlElement)
{
	ItemTexture::SaveToXml(xmlElement);
	_pendant->SaveToXml(xmlElement);
}

void ItemTree::Update(float dt)
{
	ItemTexture::Update(dt);
	_pendant->Update(dt);
	_mesh->Update(_pendant->GetDx());

}

void ItemTree::Draw()
{	
	math::Vector3 offset ((float) _rect.x, (float) _rect.y, 0.0f);
	Render::device.MatrixTranslate(offset);
	_tex->Bind();
	_mesh->Draw();
	_pendant->Draw();
	Render::device.MatrixTranslate(-offset);
}

void ItemTree::MouseMove(const IPoint &pos){

	IPoint p;
	p.x = pos.x - _rect.x;
	p.y = pos.y - _rect.y;
	_pendant->MouseMove(p, _parent->GetAngle(),_parent->GetScale() );

}


ItemEffect::ItemEffect(rapidxml::xml_node<>* xmlElement, Ryushka *parent)
	: RyushkaGroupItem(xmlElement, parent)
{
	_name = std::string(xmlElement->first_attribute("name")->value());
	_rect.x = utils::lexical_cast<int>(std::string(xmlElement->first_attribute("x")->value()));
	_rect.y = utils::lexical_cast<int>(std::string(xmlElement->first_attribute("y")->value()));
	_mask = new ItemMask(xmlElement, parent);
	_rect.width = _mask->_rect.width;
	_rect.height = _mask->_rect.height;

	_visibleRect = _rect;
	_visibleRect.x += _mask->_rect.x - 100;
	_visibleRect.y += _mask->_rect.y - 100;
	_visibleRect.width += 200;
	_visibleRect.height += 200;

	if (_visibleRect.width < 350)
	{
		_visibleRect.width = 350;
	}
	if (_visibleRect.height < 350) 
	{
		_visibleRect.height = 350;
	}

	_dt_start = (Xml::GetFloatAttributeOrDef(xmlElement, "dt", 0));
	_eff = new ParticleEffectWrapper(_name, IPoint(_rect.x, _rect.y));
	float i = 0;
	while (i < _dt_start){
		i += 0.1f;
		_eff->Update(0.1f);
	}

}

ItemEffect::~ItemEffect()
{
	delete _mask;
	delete _eff;
}

void ItemEffect::SetEnergy()
{
	RyushkaGroupItem::SetEnergy();
	if (_stateOnEnergy == RESET)
	{
		_eff->Restart();
	}
	else if (_stateOnEnergy == SHAW)
	{
		_eff->Restart();
		_eff->Update(_dt_start);
	}
}

void ItemEffect::SaveToXml(Xml::TiXmlElement *xmlElement)
{
	RyushkaGroupItem::SaveToXml(xmlElement);
	xmlElement->SetDoubleAttribute("dt", _dt_start);
	xmlElement->SetAttribute("name", _name);
	xmlElement->SetAttribute("x", _rect.x);
	xmlElement->SetAttribute("y", _rect.y);
	_mask->SaveToXml(xmlElement);
}

void ItemEffect::Upload()
{
	effectPresets.UploadEffect(_name);
}

void ItemEffect::Release()
{
	effectPresets.ReleaseEffect(_name);
}

void ItemEffect::Update(float dt)
{
	_eff->Update(dt);
}
	
void ItemEffect::Draw()
{	
	_eff->Draw();
	DrawCenter();
	if ( _parent->_isSelected && EditorUtils::editor)
	{	
		Render::device.PushMatrix();
		Render::device.MatrixTranslate(math::Vector3(_eff->GetPosition()));
		if (_mask->_activ)
		{
			_mask->Draw();
		}
		Render::device.PopMatrix();

	}

}
void ItemEffect::DrawCenter()
{
	if (EditorUtils::editor)
	{
		Render::device.PushMatrix();
		Render::device.MatrixTranslate(math::Vector3(_eff->GetPosition()));
		Render::device.SetTexturing(false);
		Render::BeginColor(Color(186, 32, 42));
		Render::DrawRect(IRect (-3, -3, 6, 6));
		Render::EndColor();
		Render::device.SetTexturing(true);
		Render::device.PopMatrix();

	}
}

void ItemEffect::ResetEnergy()
{
	_eff->Restart();
}

void ItemEffect::MouseDown(const IPoint& pos){
	 if (_mask->_activ)
	 {
		if (IsPixelOpaque(pos))
		{
			_eff->Restart();
		 }
	 }
}

bool ItemEffect::IsPixelOpaque(const IPoint& pos)
{
	IPoint p = pos;
	p.x -= _rect.x;
	p.y -= _rect.y;

	if (_mask->IsPixelOpaque(p))
	{
		return true;
	}
	return false;

}	

IRect ItemEffect::getClientRect()
{
	IRect rect;
	rect = _rect;
	rect.x += _mask->_rect.x;
	rect.y += _mask->_rect.y;
	return rect;

	/*IRect rect;
	rect = _rect;
	rect.x += _mask->_rect.x - _rect.width/2;
	rect.y += _mask->_rect.y - _rect.height/2;
	rect.width += rect.width/2;
	rect.height += rect.height/2;
	return rect;*/
}

IRect ItemEffect::getVisibleRect()
{
	return _visibleRect;
}

ItemPendantTexture::ItemPendantTexture(rapidxml::xml_node<>* xmlElement, Ryushka *parent)
	: ItemTexture(xmlElement, parent)
{
	_pendant = new Pendant(xmlElement, parent);
}

void ItemPendantTexture::Release()
{
	delete _pendant;
}

void ItemPendantTexture::Reset()
{
	_pendant->Reset();
}

void ItemPendantTexture::SaveToXml(Xml::TiXmlElement *xmlElement)
{
	ItemTexture::SaveToXml(xmlElement);
	_pendant->SaveToXml(xmlElement);
}

void ItemPendantTexture::Update(float dt)
{
	ItemTexture::Update(dt);
	_pendant->Update(dt);
}

void ItemPendantTexture::Draw()
{	
	Render::device.PushMatrix();
	Render::device.MatrixTranslate(math::Vector3(_pendant->_pos+_pendant->_pStart));
	Render::device.MatrixRotate(math::Vector3(0, 0, 1.0f), 	_pendant->GetDx());
	Render::device.MatrixTranslate(math::Vector3(-_pendant->_pos-_pendant->_pStart));
	ItemTexture::Draw();
	_pendant->Draw();
	Render::device.PopMatrix();
}

void ItemPendantTexture::MouseMove(const IPoint &pos){
	IPoint p;
	p.x = pos.x - _rect.x;
	p.y = pos.y - _rect.y;
	_pendant->MouseMove(p, _parent->GetAngle(),_parent->GetScale() );

}


ItemPendantEffect::ItemPendantEffect(rapidxml::xml_node<>* xmlElement, Ryushka *parent)
	: ItemEffect(xmlElement, parent)
{
	_pendant = new Pendant(xmlElement, parent);
}

void ItemPendantEffect::Release()
{   
	delete _pendant;
    ItemEffect::Release(); 
}

void ItemPendantEffect::Reset()
{
	_pendant->Reset();
}

void ItemPendantEffect::SaveToXml(Xml::TiXmlElement *xmlElement)
{
	ItemEffect::SaveToXml(xmlElement);
	_pendant->SaveToXml(xmlElement);
}

void ItemPendantEffect::Update(float dt)
{
	_pendant->Update(dt);
	ItemEffect::Update(dt);
}

void ItemPendantEffect::Draw()
{	
	Render::device.PushMatrix();
	Render::device.MatrixTranslate(math::Vector3(_pendant->_pos+_pendant->_pStart));
	Render::device.MatrixRotate(math::Vector3(0, 0, 1.0f), 	_pendant->GetDx());
	Render::device.MatrixTranslate(math::Vector3(-_pendant->_pos -_pendant->_pStart));
	ItemEffect::Draw();	
	_pendant->Draw();
	Render::device.PopMatrix();
}

void ItemPendantEffect::MouseMove(const IPoint &pos){
	IPoint p;
	p.x = pos.x - _rect.x;
	p.y = pos.y - _rect.y;
	_pendant->MouseMove(p, _parent->GetAngle(),_parent->GetScale() );

}


ItemMask::ItemMask(rapidxml::xml_node<>* xmlElement, Ryushka *parent)
	: _parent(parent)
{
	rapidxml::xml_node<>*element = xmlElement->first_node("Mask");

	_type = TYPE_RECT;
	_tex = NULL;
	_nameTex = "";
	_rect.x = 0;
	_rect.y = 0;
	_rect.width = 50;
	_rect.height = 50;
	_activ = false;

	if (element != NULL)
	{
		if(Xml::HasAttribute(element, "texture"))
		{
			_type = TYPE_TEXTURE;
			_nameTex = std::string(element->first_attribute("texture")->value());
			_tex = _parent->getTexture(_nameTex);
			_rect = _tex->getBitmapRect();
		}
		else
		{
			_rect.width = utils::lexical_cast<int>(std::string(element->first_attribute("w")->value()));
			_rect.height = utils::lexical_cast<int>(std::string(element->first_attribute("h")->value()));
		}
		_rect.x = utils::lexical_cast<int>(std::string(element->first_attribute("x")->value()));
		_rect.y = utils::lexical_cast<int>(std::string(element->first_attribute("y")->value()));
		_activ = utils::lexical_cast<bool>(std::string(element->first_attribute("activ")->value()));
	}
}

bool ItemMask::IsPixelOpaque(const IPoint& pos)
{

	if (_type == TYPE_RECT)
	{
		if (_rect.Contains(pos))
		{
			 return true;
		}
	}
	else if (_type == TYPE_TEXTURE)
	{
		IPoint p = pos;
		p.x -= _rect.x;
		p.y -= _rect.y;
		if (_tex->isPixelOpaque(p))
		{
			return true;
		}
	}
 
	return false;
}

void ItemMask::SaveToXml(Xml::TiXmlElement *parentElem){
		Xml::TiXmlElement *maskElem = parentElem->InsertEndChild(Xml::TiXmlElement("Mask"))->ToElement();
		maskElem->SetAttribute("activ", _activ);
		maskElem->SetAttribute("h", _rect.height);
		if (_type == TYPE_TEXTURE)
		{
			maskElem->SetAttribute("texture", _nameTex);
		}
		maskElem->SetAttribute("w", _rect.width );
		maskElem->SetAttribute("x", _rect.x );
		maskElem->SetAttribute("y", _rect.y );
}

void ItemMask::Draw(){
		Render::device.SetTexturing(false);
		Render::BeginColor(Color(186, 32, 42));

		if (_type == TYPE_RECT)
		{
		
			Render::DrawRect(IRect(_rect.x, _rect.y, _rect.width, 1));
			Render::DrawRect(IRect(_rect.x, _rect.y, 1, _rect.height));
			Render::DrawRect(IRect(_rect.x +_rect.width-1, _rect.y, 1, _rect.height));
			Render::DrawRect(IRect(_rect.x, _rect.y +_rect.height-1, _rect.width, 1));
		}
		else
		{
			IPoint p;
			p.x = _rect.x;
			p.y = _rect.y;
			Render::BeginAlphaMul(0.3f);
			_tex->Draw(p);
			Render::EndAlphaMul();

		}

		Render::EndColor();
		Render::device.SetTexturing(true);

}



ItemFlag::ItemFlag(rapidxml::xml_node<>* xmlElement, Ryushka *parent)
	: ItemTexture(xmlElement, parent)
{
	_mesh = new Mesh(Mesh::FLAG);
	
	FRect uv(0, 1, 0, 1);
	if (_tex->needTranslate()) {
        FRect rect(0, 0, _rect.width + 0.f, _rect.height + 0.f);
		_tex->TranslateUV( rect, uv);
	}
	
	_mesh->Init(_rect, uv);
	_mesh->InitK(IPoint(0,0), IPoint(0,0));
	_timer = 0;
}

void ItemFlag::SaveToXml(Xml::TiXmlElement *xmlElement)
{
	ItemTexture::SaveToXml(xmlElement);
}

void ItemFlag::Update(float dt)
{
	_timer += dt;
	ItemTexture::Update(dt);
	_mesh->Update(_timer);

}

void ItemFlag::Draw()
{	
	math::Vector3 offset ((float) _rect.x, (float) _rect.y, 0.0f);
	Render::device.MatrixTranslate(offset);
	_tex->Bind();
	_mesh->Draw();
	Render::device.MatrixTranslate(-offset);
}

void ItemFlag::MouseMove(const IPoint &pos){

	IPoint p;

}





namespace Butterfly{
const float pi = 3.1415926f;
const float SCREEN_WIDTH = 800.0f;
const float SCREEN_HEIGHT = 600.0f;
const float NEW_FIELD_OF_VIEW = 45.0f;
const float NEW_Z_NEAR = 100.0f;
const float NEW_Z_FAR = 10000.0f;
const float NEW_Z_START = SCREEN_HEIGHT / 2.0f / tan(NEW_FIELD_OF_VIEW * pi / 360);
const float NEW_Y_MAX = NEW_Z_NEAR * tan(NEW_FIELD_OF_VIEW * pi / 360);
const float NEW_X_MAX = NEW_Y_MAX * SCREEN_WIDTH / SCREEN_HEIGHT;
const float BATTERFLY_ZOOM = 1.f;


BaseButterfly::BaseButterfly(Render::Texture *tex)
: _tex(tex)
, _angleX(-45)
, _angleY(0)
, _angleZ(0)
, _prevAngleZ(_angleZ)
, _nextAngleZ(_angleZ)
, local_time(math::random(0.f, 7.f))
, time_scale(10.f)
, _timerMove(1.f)
, _prevPos(math::Vector3(0, 0, 0))
, _nextPos(_prevPos)
, _pause(math::random(0.f, 10.f))
, _timeScaleMove(1)
{
	yBound = _prevPos.z;
	x = _prevPos.x;
	y = _prevPos.y;
}

void BaseButterfly::Draw()
{	
	//IPoint mouse_pos = Core::mainInput.GetMousePos();
	//Render::device.SetCurrentMatrix(Render::PROJECTION);
	Render::device.PushMatrix();
	//Render::device.SetViewFrustum(-NEW_X_MAX, NEW_X_MAX, -NEW_Y_MAX , NEW_Y_MAX , NEW_Z_NEAR, NEW_Z_FAR);

	//Render::device.SetCurrentMatrix(Render::MODELVIEW);
	//Render::device.PushMatrix();
	//Render::device.SetDepthTest(true);

	//Render::device.MatrixTranslate(math::Vector3(0, 0, -NEW_Z_START));		
	int h = _tex->getBitmapRect().height;

	//Render::device.MatrixTranslate(math::Vector3(-SCREEN_WIDTH/2, -SCREEN_HEIGHT/2, 0));
	Render::device.MatrixTranslate(math::Vector3(x, y, 0));
	Render::device.MatrixScale(_size);
	Render::device.MatrixRotate(math::Vector3(1, 0, 0), _angleX);
	Render::device.MatrixRotate(math::Vector3(0, 0, 1), _angleZ-90);



	Render::device.PushMatrix();
	Render::device.MatrixScale(-1, 1, 1);
	Render::device.MatrixRotate(math::Vector3(0, 1, 0), -_angleY);
	_tex->Draw(IPoint(0, -h/2));
	Render::device.PopMatrix();

	Render::device.PushMatrix();
	Render::device.MatrixRotate(math::Vector3(0, 1, 0), -_angleY);
	_tex->Draw(IPoint(0, -h/2));
	Render::device.PopMatrix();



	//Render::device.SetDepthTest(false);
	//Render::device.SetCurrentMatrix(Render::MODELVIEW);
	//Render::device.PopMatrix();
	//Render::device.SetCurrentMatrix(Render::PROJECTION);
	//Render::device.PopMatrix();
	//Render::device.SetCurrentMatrix(Render::MODELVIEW);
	Render::device.PopMatrix();
}

void BaseButterfly::Init()
{
	local_time = math::random(0.f, 7.f);
	_timerMove = 1.f;

	_prevPos = math::Vector3(1000.f, 300.f, 0.f);
	_nextPos = _prevPos;
	x = _prevPos.x;
	y = _prevPos.y;
	yBound = _prevPos.z;
	_timeScaleMove = 1;
	time_scale = 10;

	_angleY = 0.f;
	_angleZ = 0.f;

	_prevAngleZ = _angleZ;
	_nextAngleZ = _angleZ;
	_pause = math::random(0.f, 10.f);
}

void BaseButterfly::Update(float dt)
{
}

void BaseButterfly::Scare()
{
	_pause = 0;
}

void StandPoint::Init(rapidxml::xml_node<>* pointsElem) {

	float x = utils::lexical_cast<float>(std::string(pointsElem->first_attribute("x")->value()));
	float y = utils::lexical_cast<float>(std::string(pointsElem->first_attribute("y")->value()));
	
	_point = math::Vector3(x,y,1.f);
	_inUse = false;
}
void StandPoint::SaveToXml(Xml::TiXmlElement *parentItem) {
	Xml::TiXmlElement *itemElem = parentItem->InsertEndChild(Xml::TiXmlElement("Points"))->ToElement();
	Xml::SetFloatAttribute(itemElem, "x", _point.x);
	Xml::SetFloatAttribute(itemElem, "y", _point.y);
}
ItemButterfly::ItemButterfly(rapidxml::xml_node<>* xmlElement, Ryushka *parent)
	: ItemTexture(xmlElement, parent)
	, BaseButterfly(ItemTexture::_tex)
	, _pointIndex(0)
{

	rapidxml::xml_node<>* pointElem = xmlElement->first_node();
	while (pointElem != NULL)
	{
		StandPoint p;
		p.Init(pointElem);
		_standPoints.push_back(p);
		pointElem = pointElem->next_sibling();
	}
	
	_size = utils::lexical_cast<float>(std::string(xmlElement->first_attribute("size")->value()));
	ItemTexture::_tex->setAddressType(Render::Texture::CLAMP);

	Init();
}

void ItemButterfly::SaveToXml(Xml::TiXmlElement *parentElem)
{
	ItemTexture::SaveToXml(parentElem);
	Xml::SetFloatAttribute(parentElem, "size", _size);
	for(std::vector<StandPoint>::iterator  it = _standPoints.begin(); it != _standPoints.end(); ++it ) {
		(it)->SaveToXml(parentElem);
	}
}


void ItemButterfly::Init()
{
	StandPoint *p = GetStartPoint();
	
	_prevPos = p->_point;
	_nextPos = _prevPos;
	
	x = _prevPos.x;
	y = _prevPos.y;
	
	_timeScaleMove = 1;
	time_scale = 10;
	
	_angleZ = math::random(0.f, 360.f);
	_prevAngleZ = _angleZ;
	_nextAngleZ = _angleZ;


	_pause = math::random(0.f, 1.f);
	_state = STAND;
	
	yBound = _prevPos.z;
}


void ItemButterfly::Update(float dt)
{
	if (_state == STAND)
	{
		UpdateStand(dt);
	} else if (_state == CHANGEANGLE) {
		local_time += time_scale*dt;
		_angleY = 70*sinf(local_time);
		_timerMove += 2*dt;
		if (_timerMove > 1)
		{
			_timerMove = 0;
			_angleZ = _nextAngleZ;
			_prevAngleZ = _nextAngleZ;
			_prevPos.y = y;
			_prevPos.z = yBound;
			_state = FLY;
		} else {
			_angleZ = math::lerp(_prevAngleZ, _nextAngleZ, _timerMove);
			y += 20*dt;
			yBound -= 20*dt;
		}
	} else if (_state == FLY) {
		UpdateFly(dt);
	} else {
		Assert(false);
	}
}

void ItemButterfly::SetState()
{
	_state = STAND;

	yBound = _nextPos.z;
	_pause = math::random(4.f, 10.f);
	
}


void ItemButterfly::UpdateFly(float dt)
{
	local_time += time_scale*dt;
	_angleY = 70*sinf(local_time);
	_timerMove += _timeScaleMove*dt;
	if (_timerMove > 1)
	{

		_timerMove = 0;
		_prevPos = _nextPos;

		x = _nextPos.x;
		y = _nextPos.y;

		yBound = _nextPos.z;
		SetState();
	} else {

		x = math::lerp(_prevPos.x, _nextPos.x, math::ease(_timerMove, 0.2f, 0.2f));
		
		x += 5*sinf(_timerMove * math::PI * 4);

		y = math::lerp(_prevPos.y, _nextPos.y, math::ease(_timerMove, 0.2f, 0.2f));

		y += 20 * sinf(_timerMove * math::PI) - 5 * sinf(local_time);

		yBound = math::lerp(_prevPos.z, _nextPos.z, math::ease(_timerMove, 0.2f, 0.2f)) - 30*sinf(_timerMove*math::PI);

	}
}

void ItemButterfly::UpdateStand(float dt)
{
	if (_pause > 0)
	{
		_pause -= dt;
		
		local_time += 1.5f*dt;
		
		_angleY = 90*fabs(sinf(local_time*0.61f + 1.7f)*sinf(local_time*0.13f));
		//_angleY = 30.f;

	} else {
		

		if (math::random(0,9) == 5) {
			//MoveOutScreen();
			return;
		}


		_state = CHANGEANGLE;

		_timerMove = 0;
		
		time_scale = math::random(10.f, 14.f);

		_prevAngleZ = _angleZ;
		
		StandPoint *p;

		p = GetStartPoint();
	
		while (_nextPos == p->_point) {
			p = GetStartPoint();
		}

		_nextPos = p->_point;
		StartMove();
	}
}

void ItemButterfly::StartMove()
{
	float dx = _nextPos.x - _prevPos.x;
	float dy = _nextPos.y - _prevPos.y;
	
	if (dx == 0.f)
	{
		if (dy > 0.f)
		{
			_nextAngleZ = 90;
		} else {
			_nextAngleZ = -90;
		}
	} else {
		_nextAngleZ = 180*atanf(dy/dx)/math::PI;

		if (dx<0)
		{
			_nextAngleZ = 180+_nextAngleZ;
		}
	}
	SetAngle();
	
	if (_nextAngleZ > 360.f) {
		_nextAngleZ -= 360.f;
	}
	
	if (_nextAngleZ < 0.f) {
		_nextAngleZ += 360.f;
	}

	_timeScaleMove = 70.f/sqrt(dx*dx+dy*dy);
	if (_timeScaleMove > 0.6f) _timeScaleMove = 0.6f;
}

void ItemButterfly::SetAngle()
{
	_prevAngleZ = static_cast<int>(_prevAngleZ)%360 + 0.f;
	_nextAngleZ = static_cast<int>(_nextAngleZ)%360 + 0.f;
	if (_nextAngleZ - _prevAngleZ > 180)
	{
		_nextAngleZ -= 360;
	}
	if (_nextAngleZ - _prevAngleZ < -180)
	{
		_nextAngleZ += 360;
	}	
}



void ItemButterfly::MouseMove(const IPoint &mouse_pos)
{
	if (x - 10 < mouse_pos.x && mouse_pos.x < x + 10 &&
		y - 10 < mouse_pos.y && mouse_pos.y < y + 10)
	{
		_timerMove = 0;
		_prevPos = math::Vector3(x, y, yBound);
		_pause = 0;	
		_state = CHANGEANGLE;

		time_scale = math::random(10.f, 14.f);

		_prevAngleZ = _angleZ;
		
		StandPoint *p;

		p = GetStartPoint();

		_nextPos = p->_point;

		StartMove();
	}
}		

IRect ItemButterfly::getVisibleRect()
{
	IRect rect(_rect);
	rect.x -= 100;
	rect.y -= 100;
	rect.width += 200;
	rect.height += 200;
	return _rect;
}

void ItemButterfly::MouseDown(const IPoint& pos) {

	if ((pos.x < x + 10)&&(pos.x > x - 10)&&(pos.y < y + 10)&&(pos.y > y - 10)) {
		
		
	}
}
/*
void ItemButterfly::Upload()
{
	ItemTexture::Upload();
}

void ItemButterfly::Release()
{
	ItemTexture::Release();
}*/

void ItemButterfly::Draw() {
	
		float alpha = 1.f;
		alpha = math::max((yBound + 9.f) / 10.f,0.f); 
		BaseButterfly::Draw();
		
}

void ItemButterfly::DrawShade(float shadeAlpha)
{		
	Render::BeginColor(Color(0, 0, 0, 77));

	int h = ItemTexture::_tex->getBitmapRect().height;
	Render::device.PushMatrix();
	Render::BeginAlphaMul(shadeAlpha);
	Render::device.MatrixTranslate(math::Vector3(x, y, 0));
	
	Render::device.MatrixTranslate(math::Vector3(-2*yBound, 2*yBound, 0.f));
	
	Render::device.MatrixScale(_size + 0.05f);
	Render::device.MatrixRotate(math::Vector3(1, 0, 0), _angleX);
	Render::device.MatrixRotate(math::Vector3(0, 0, 1), _angleZ - 90.f);

	Render::device.PushMatrix();
	
	Render::device.MatrixScale(1 - _angleY/90.f, 1, 0);

	ItemTexture::_tex->Bind();
	ItemTexture::_tex->Draw(IPoint(0, -h/2));
	Render::device.PopMatrix();

	if (_angleY > 0 || yBound > 10)
	{
		Render::device.PushMatrix();
		Render::device.MatrixRotate(math::Vector3(0, 1, 0), -_angleY*0.9f);
		Render::device.MatrixScale(-1, 1, 1);
		ItemTexture::_tex->Bind();
		ItemTexture::_tex->Draw(IPoint(0, -h/2));
		Render::device.PopMatrix();
	}

	Render::device.PopMatrix();
	Render::EndAlphaMul();
	Render::EndColor();

}



};




TextureMove::TextureMove(rapidxml::xml_node<>* xmlElement, Ryushka *parent)
	: ItemTexture(xmlElement, parent)
{
	_add.x = utils::lexical_cast<float>(std::string(xmlElement->first_attribute("add")->value()));
	_add.y = _add.x*0.5f;
	_random_add = _add;
	_k = utils::lexical_cast<float>(std::string(xmlElement->first_attribute("k")->value())); 
	_m = utils::lexical_cast<float>(std::string(xmlElement->first_attribute("m")->value())); 
	_c = utils::lexical_cast<float>(std::string(xmlElement->first_attribute("c")->value())); 




	_mousePosOld = IPoint(0,0);
	_sp.x  = _add.x*math::random(-1.f, 1.f);
	_sp.y  = _add.y*math::random(-1.f, 1.f);
	_dp  = FPoint(0, 0);

}

void TextureMove::SaveToXml(Xml::TiXmlElement *xmlElement)
{
	ItemTexture::SaveToXml(xmlElement);
	xmlElement->SetDoubleAttribute("add", _add.x);
	xmlElement->SetDoubleAttribute("k", _k);	
	xmlElement->SetDoubleAttribute("m", _m);	
	xmlElement->SetDoubleAttribute("c", _c);

}

void TextureMove::Draw()
{
	Render::device.PushMatrix();
	Render::device.MatrixTranslate(math::Vector3(_dp));
	Render::device.MatrixTranslate(math::Vector3(_rect.width/2.f, _rect.height/2.f, 0));
	Render::device.MatrixRotate(math::Vector3(0,0,1), 10*_dp.x*math::PI/180.f);
	Render::device.MatrixTranslate(math::Vector3(-_rect.width/2.f, -_rect.height/2.f, 0));
	ItemTexture::Draw();
	Render::device.PopMatrix();
}




void TextureMove::Update(float dt)
{
	_a = -((_k*_dp)+(_c*_sp))/_m;
	_sp += _a*dt;	
	_dp += _sp*dt + _a*dt*dt/2;
	_dp.x = math::clamp(-1000.f, 1000.f, _dp.x);
	_dp.y = math::clamp(-1000.f, 1000.f, _dp.y);
	int oldSignX = math::sign(_sp.x);
	int oldSignY = math::sign(_sp.y);

	if (_sp.x > 0) 
	{
		_sp.x += _random_add.x*dt;
	} else {
		_sp.x -= _random_add.x*dt;
	}
	if (_sp.y > 0) 
	{
		_sp.y += _random_add.y*dt;
	} else {
		_sp.y -= _random_add.y*dt;
	}
	
	if (oldSignX != math::sign(_sp.x))
	{
		_random_add.x = _add.x*math::random(0.2f, 1.f);
	}
	if (oldSignY != math::sign(_sp.y))
	{
		_random_add.y = _add.y*math::random(0.2f, 1.f);
	}

}



void TextureMove::MouseMove(const IPoint& pos)
{
	// —жимаем
	
	FPoint ds;
	ds.x = 0;
	ds.y = 0;

	IPoint pixel = pos - _dp.Rounded();
	if (_tex->isPixelOpaque(pixel.x, pixel.y))
	{	
		ds = (pos-_mousePosOld)/_m;
	}

	if ((_dp.x > 0)&&(_sp.x > 0)||(_dp.x < 0)&&(_sp.x < 0))
	{
		ds.x /= math::abs(_dp.x)+1;
	}
	if ((_dp.y > 0)&&(_sp.y > 0)||(_dp.y < 0)&&(_sp.y < 0))
	{
		ds.y /= math::abs(_dp.y)+1;
	}

	_sp += ds;

	_mousePosOld = pos;
}

FPoint TextureMove::GetDeltaPos()
{
	return _parent->CorrectionDp(_dp);
}

EffectMove::EffectMove(rapidxml::xml_node<>* xmlElement, Ryushka *parent)
	: ItemEffect(xmlElement, parent)
{
	_add.x = utils::lexical_cast<float>(std::string(xmlElement->first_attribute("add")->value()));
	_add.y = _add.x*0.5f;
	_random_add = _add;
	_k = utils::lexical_cast<float>(std::string(xmlElement->first_attribute("k")->value())); 
	_m = utils::lexical_cast<float>(std::string(xmlElement->first_attribute("m")->value())); 
	_c = utils::lexical_cast<float>(std::string(xmlElement->first_attribute("c")->value())); 
	_mousePosOld = IPoint(0,0);
	_sp  = _add*math::random(-1.f, 1.f);
	_dp  = FPoint(0, 0);

}

void EffectMove::SaveToXml(Xml::TiXmlElement *xmlElement)
{
	ItemEffect::SaveToXml(xmlElement);
	xmlElement->SetDoubleAttribute("add", _add.x);
	xmlElement->SetDoubleAttribute("k", _k);	
	xmlElement->SetDoubleAttribute("m", _m);	
	xmlElement->SetDoubleAttribute("c", _c);

}

void EffectMove::Draw()
{
	Render::device.PushMatrix();
	Render::device.MatrixTranslate(math::Vector3(_dp));
	Render::device.MatrixTranslate(math::Vector3(_rect.width/2.f, _rect.height/2.f, 0));
	Render::device.MatrixRotate(math::Vector3(0,0,1), 10*_dp.x*math::PI/180.f);
	Render::device.MatrixTranslate(math::Vector3(-_rect.width/2.f, -_rect.height/2.f, 0));

	ItemEffect::Draw();
	Render::device.PopMatrix();
}




void EffectMove::Update(float dt){
	ItemEffect::Update(dt);

	_a = -((_k*_dp)+(_c*_sp))/_m;
	_sp += _a*dt;	
	_dp += _sp*dt + _a*dt*dt/2;
	FPoint old_sp = _sp;

	FPoint ds;
	if (_sp.x > 0) ds.x = _random_add.x;
	else ds.x = -_random_add.x;
	if (_sp.y > 0) ds.y = _random_add.y;
	else ds.y = -_random_add.y;

	_sp += ds*dt;
	
	if (old_sp.x*_sp.x < 0)
	{
		_random_add.x = _add.x*math::random(0.2f, 1.f);
	}
	if (old_sp.y*_sp.y < 0)
	{
		_random_add.y = _add.y*math::random(0.2f, 1.f);
	}

}



void EffectMove::MouseMove(const IPoint& pos)
{
	// —жимаем
	
	FPoint ds;
	ds.x = 0;
	ds.y = 0;

	IPoint pixel = pos - _dp.Rounded();
	if (IsPixelOpaque(pixel))
	{	
		ds = (pos-_mousePosOld)/_m;

		if ((_dp.x > 0)&&(_sp.x > 0)||(_dp.x < 0)&&(_sp.x < 0))
		{
			ds.x /= math::abs(_dp.x)+1;
		}
		if ((_dp.y > 0)&&(_sp.y > 0)||(_dp.y < 0)&&(_sp.y < 0))
		{
			ds.y /= math::abs(_dp.y)+1;
		}
	}

	_sp += ds;
	_mousePosOld = pos;
}

FPoint EffectMove::GetDeltaPos()
{
	return _parent->CorrectionDp(_dp);
}

ItemChain::ItemChain(rapidxml::xml_node<>* xmlElement, Ryushka *parent)
	: ItemTexture(xmlElement, parent)
{
	_pendant = new Pendant(xmlElement, parent, Pendant::TWO);
	_mesh = new Mesh(Mesh::CHAIN);
	
	FRect uv(0, 1, 0, 1);
	if (_tex->needTranslate()) {
        FRect  rect(0, 0, _rect.width + 0.f, _rect.height + 0.f);
		_tex->TranslateUV( rect, uv);
	}
	
	_mesh->Init(_rect, uv);
	_mesh->InitK(_pendant->_pStart, _pendant->_pEnd);
}

void ItemChain::SaveToXml(Xml::TiXmlElement *xmlElement)
{
	ItemTexture::SaveToXml(xmlElement);
	_pendant->SaveToXml(xmlElement);
}

void ItemChain::Update(float dt)
{
	ItemTexture::Update(dt);
	_pendant->Update(dt);
	_mesh->Update(_pendant->GetDx());

}

void ItemChain::Draw()
{	
	math::Vector3 offset ((float) _rect.x, (float) _rect.y, 0.0f);
	math::Vector3 offsetShadow (10.f, 10.f, 0.0f);


	Render::device.MatrixTranslate(offset);


	
	

	_tex->Bind();
	_mesh->Draw();
	_pendant->Draw();

	Render::device.MatrixTranslate(-offset);
}

void ItemChain::MouseMove(const IPoint &pos){

	IPoint p;
	p.x = pos.x - _rect.x;
	p.y = pos.y - _rect.y;
	_pendant->MouseMove(p, _parent->GetAngle(),_parent->GetScale() );

}

					

	ItemWaterPlant::ItemWaterPlant(rapidxml::xml_node<>* xmlElement, Ryushka *parent)
		:ItemTexture(xmlElement, parent)
	{
		_mesh = TextureMesh::SharedPtr(new TextureMesh(_tex, 2, _tex->getBitmapRect().height / 10 + 1));
		_maxShift = Xml::GetFloatAttribute(xmlElement, "maxShift") ;
		_timeScale = Xml::GetFloatAttribute(xmlElement, "timescale") ;
		_blockLeft = Xml::GetIntAttributeOrDef(xmlElement, "blockLeft", 0);
		_blockRight = Xml::GetIntAttributeOrDef(xmlElement, "blockRight", 0);
		_blockUp = Xml::GetIntAttributeOrDef(xmlElement, "blockUp", 0);
		_blockDown = Xml::GetIntAttributeOrDef(xmlElement, "blockDown", 0);

		_oldPos = IPoint();

		_pendant = new Pendant(xmlElement, parent);
		FPoint center = FPoint((_pendant->_pStart+_pendant->_pEnd)/2);
		float max = FPoint(_pendant->_pStart).GetDistanceTo(center);
		for (int i = 0; i <= _mesh->Columns(); i++) {
			_paths.push_back(std::vector<CirclePath>(0));
			for (int j = 0; j <= _mesh->Rows(); j++) {
				float x, y;
				_mesh->GetKnotPosition(i, j).GetXY(x, y);
				float f = sqrt(math::abs(_mesh->GetTexture()->getBitmapRect().height-y) / _mesh->GetTexture()->getBitmapRect().height * _maxShift);
				_paths[i].push_back(CirclePath(_pendant->_pStart, max, f, f, 0));
			}
		}
	};
	
	void ItemWaterPlant::Draw()
	{
		Render::device.PushMatrix();
		Render::device.MatrixTranslate(FPoint(_pos));
		_pendant->Draw();
		_mesh->Draw(IPoint(0, 0));
		Render::device.PopMatrix();
	}

	void ItemWaterPlant::Update(float dt) 
	{
		_pendant->Update(dt);
		float dx = _pendant->GetDx();
		for (int i = _blockLeft; i <= _mesh->Columns()-_blockRight; i++) {
			for (int j = _blockDown; j <= _mesh->Rows()-_blockUp; j++) {
				_paths[i][j].Update(dt*_timeScale);
				FPoint p =  _mesh->GetKnotPosition(i, j);
				p -= _pendant->_pStart;
				p = p.Rotated(dx*_paths[i][j].k) - p;
				FPoint tmp = _paths[i][j].GetFPoint();
				tmp.Scale(1.f, 0.3f);
				_mesh->SetKnotShift(i, j, tmp + p);
			}
		}

				//_mesh[i][j] = _d_mesh[i][j].pos;
				//_mesh[i][j] -= _pStart;
				//_mesh[i][j] = _mesh[i][j].Rotate(dx*_d_mesh[i][j].k*math::PI/180);
				//_mesh[i][j] += _pStart;
	}

	void ItemWaterPlant::SaveToXml(Xml::TiXmlElement *parentElem)
	{


		ItemTexture::SaveToXml(parentElem);
		_pendant->SaveToXml(parentElem);
		parentElem->SetAttribute("blockDown", _blockDown);
		parentElem->SetAttribute("blockLeft", _blockLeft);
		parentElem->SetAttribute("blockRight", _blockRight);
		parentElem->SetAttribute("blockUp", _blockUp);
		parentElem->SetDoubleAttribute("maxShift", _maxShift);
		parentElem->SetDoubleAttribute("timescale", _timeScale);	


	}

	void ItemWaterPlant::MouseMove(const IPoint& pos)
	{
		IPoint p;
		p.x = pos.x - _rect.x;
		p.y = pos.y - _rect.y;
		_pendant->MouseMove(p, _parent->GetAngle(),_parent->GetScale() );
	}


