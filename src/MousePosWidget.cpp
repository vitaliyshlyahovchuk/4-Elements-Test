#include "stdafx.h"
#include "MousePosWidget.h"
#include "Core/Window.h"
#include "GameInfo.h"

MousePosWidget::MousePosWidget(const std::string& name, rapidxml::xml_node<>* xe)
	: GUI::Widget(name, xe)
	, _helpFile("")
{
	rapidxml::xml_node<>* fontXml = xe->first_node("font");
	_fontName = Xml::GetStringAttribute(fontXml, "name");
	if(xe->first_node("help"))
	{
		_helpFile = Xml::GetStringAttributeOrDef(xe->first_node("help"), "path", "");
	}
}

void MousePosWidget::Draw()
{
	IPoint mousePos = Core::mainInput.GetMousePos();
	std::string output = Int::ToString(mousePos.x) + ", " + Int::ToString(mousePos.y);
	Render::FreeType::BindFont(_fontName);
	Render::PrintString(position, output);
	const float SIDE = 100.f;
	if(gameInfo.getGlobalBool("game_debug_cursor", false))
	{
		Render::device.SetTexturing(false);
		Render::BeginColor(Color::BLACK);
		FPoint p(mousePos);
		Render::DrawLine(FPoint(p.x - SIDE, p.y), FPoint(p.x + SIDE, p.y));
		Render::DrawLine(FPoint(p.x, p.y - SIDE), FPoint(p.x, p.y + SIDE));
		Render::EndColor();
		Render::device.SetTexturing(true);
	}
}


void MousePosWidget::AcceptMessage(const Message &message)
{
#ifdef ENGINE_TARGET_WIN32
	if (message.is("KeyPress")) 
	{
		int key = utils::lexical_cast<int>(message.getData());
		if(key == -VK_F1)
		{
			if(!_helpFile.empty() && Core::fileSystem.FileExists(_helpFile))
			{
				utils::OpenPath(File::pwd() + "/" + _helpFile);
			}
		}
	}
#endif
}