#include "stdafx.h"

#include "Game.h"
#include "RyushkaSelect.h"

// Названия разделов в редакторе с рюшками
//static 
const std::string TAB_NAMES[] = 
{ 
	"MagickForest",
	"MagickArctic",
	"Desert",
	"Subterranean",
	"WaterCountry"
};

RyushkaSelect::RyushkaSelect(std::string name, rapidxml::xml_node<>* xmlElement)
: GUI::Widget(name)
{
	_activeTab = 0;
	_list = NULL;
	_time = 0.0f;
	_activeRyushka = "";

	Xml::RapidXmlDocument doc("SpisokRyushek.xml");

	rapidxml::xml_node<>* root = doc.first_node();

	rapidxml::xml_node<>* elem = root->first_node();
	while (elem)
	{
		std::string  name = std::string(elem->first_attribute("name")->value());
		int tab = utils::lexical_cast<int> (std::string(elem->first_attribute("tab")->value()));
		_tabs[tab - 1].push_back(name);
		elem = elem->next_sibling();
	}

	_texture = Core::resourceManager.Get<Render::Texture>("EditorButtonActive");
}


void RyushkaSelect::Draw()
{
	if (_list == NULL)
	{
		_list = Core::guiManager.getLayer("EditorList")->getWidget("List");
		AcceptMessage(Message("SelectTab", 1));
	}

	Render::BeginAlphaMul(math::abs(math::sin(4.0f * _time)) * 0.5f);
	_texture->Draw(IPoint(10 + _activeTab * 40, 550));
	Render::EndAlphaMul();

	if (_tempRyushka)
	{
		Render::device.SetCurrentMatrix(Render::MODELVIEW);
		Render::device.PushMatrix();
		Render::device.MatrixTranslate(math::Vector3(500.0f, 300.0f, 0.0f));
		_tempRyushka->Draw();
		Render::device.PopMatrix();
	}

	Render::FreeType::BindFont("TutorialPanel");
	Render::PrintString(IPoint(550, 50), TAB_NAMES[_activeTab], 1.f, RightAlign);
}

void RyushkaSelect::Update(float dt)
{
	if (_list == NULL)
	{
		_list = Core::guiManager.getLayer("EditorList")->getWidget("List");
		AcceptMessage(Message("SelectTab", 1));
	}
	_time += dt;

	Message state = _list->QueryState(Message("CurrentItem"));
	std::string  selectedRyushka = state.getData();
	if ((selectedRyushka != "") && (_activeRyushka != selectedRyushka))
	{
		if (_tempRyushka)
		{
			_tempRyushka.reset();
		}
		_activeRyushka = selectedRyushka;

		Xml::RapidXmlDocument doc("SpisokRyushek.xml");
		rapidxml::xml_node<>* root = doc.first_node();

		rapidxml::xml_node<>* elem = root->first_node();
		while (elem)
		{
			if (_activeRyushka == std::string(elem->first_attribute("name")->value()))
			{
				_tempRyushka = Ryushka::CreateRyushkaFromXml(elem);
				_tempRyushka->Upload();
				break;
			}
			elem = elem->next_sibling();
		}
	}

	if (_tempRyushka)
		_tempRyushka->Update(dt);
}

bool RyushkaSelect::MouseDown(const IPoint &mouse_pos)
{
	return false;
}

void RyushkaSelect::MouseUp(const IPoint &mouse_pos)
{
}

void RyushkaSelect::MouseMove(const IPoint &mouse_pos)
{
}

void RyushkaSelect::AcceptMessage(const Message &message)
{
	if (message.is("SelectTab"))
	{
		_activeTab = message.getIntegerParam() - 1;
		_list->AcceptMessage(Message("Clear"));

		for (std::vector<std::string>::iterator i = _tabs[_activeTab].begin() ; i != _tabs[_activeTab].end() ; ++i)
		{
			_list->AcceptMessage(Message("Add", *i));
		}
	}
}