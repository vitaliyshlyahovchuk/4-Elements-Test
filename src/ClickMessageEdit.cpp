#include "stdafx.h"

#include "ClickMessageEdit.h"
#include "GameField.h"

ClickMessageEdit::ClickMessageEdit(std::string name_, rapidxml::xml_node<>* xmlElement)
	: Widget("ClickMessageEdit")
	, _cursorTime(0.f)
	, _tempStr("")
{
}

void ClickMessageEdit::Draw()
{
	IPoint editBoxPoint (300, 300);
	IPoint editBoxText (editBoxPoint.x + 10, editBoxPoint.y + 4);
	IRect editBoxRect (editBoxPoint.x, editBoxPoint.y, 200, 25);

	Render::device.PushMatrix();
	//Render::device.MatrixTranslate(math::Vector3(_deltaX,_deltaY,0));
	
	Render::device.SetTexturing(false);
	
	Render::BeginColor(Color(150, 150, 150, 190));
	Render::DrawRect(editBoxRect);
	Render::EndColor();

	Render::BeginColor(Color(0, 0, 0, 190));
	Render::DrawFrame(editBoxRect);
	Render::EndColor();
	
	Render::device.SetTexturing(true);

	Render::FreeType::BindFont("debug");
	Render::PrintString(editBoxText, _tempStr);

	int a = static_cast<int>(128+127*cosf(2*3.14159f*_cursorTime));
	if (a<0) a = 0;
	Render::BeginColor(Color(0, 255, 0, a));

	IPoint cursorOffset (editBoxText);
	Render::getStringWidth(_tempStr.substr(0, static_cast<size_t>(_cursorPos)));
	cursorOffset.x += Render::getStringWidth(_tempStr.substr(0, static_cast<size_t>(_cursorPos)));
	cursorOffset.y += -2;

	Render::PrintString(cursorOffset, "_");

	Render::EndColor();

	Render::device.PopMatrix();
}

void ClickMessageEdit::Update(float dt)
{
	_cursorTime += dt*2;
	if (_cursorTime >= 1.f)
	{
		_cursorTime = 0.f;
	}
}

void ClickMessageEdit::AcceptMessage(const Message &message)
{
    /*	if (message.is("KeyPress"))
	{
		int data = utils::lexical_cast<int>(message.getData());
		char key = data;

		if (key == -VK_LEFT)
		{
			if (_cursorPos > 0)
			{
				_cursorPos--;
			}
			_cursorTime = 0.f;
		} else if (key == -VK_RIGHT)
		{
			if (_cursorPos < static_cast<int>(_tempStr.length()))
			{
				_cursorPos++;
			}
			_cursorTime = 0.f;
		} else if (key == -VK_UP)
		{
			_cursorPos = static_cast<int>(_tempStr.length());
			_cursorTime = 0.f;
		} else if (key == -VK_DOWN)
		{
			_cursorPos = 0;
			_cursorTime = 0.f;
		} else if (data > 0 && key > ' ' && key < 128 && _tempStr.length() < 20)
		{
			_tempStr.insert(static_cast<size_t>(_cursorPos), 1, key);
			_cursorPos++;
			_cursorTime = 0.f;
		} else if (key == 8)
		{
			if (_cursorPos > 0)
			{
				_cursorPos--;
				_tempStr.erase(static_cast<size_t>(_cursorPos), 1);
			}
		}
	}
	else*/ if (message.is("SetTempStr"))
	{
		_tempStr =  message.getVariables().getString("str");
		_from = message.getVariables().getString("from");

		_cursorPos = static_cast<int>(_tempStr.length());
	}
	else if (message.is("Save"))
	{
		GUI::Widget *gameWidget = Core::guiManager.getLayer("GameLayer") -> getWidget("GameField");
		Message msg("SaveClickMessage");

		msg.variables.setString("str", _tempStr);
		msg.variables.setString("to", _from);
				
		gameWidget -> AcceptMessage(msg);
	}
	else if (message.is("SetDelta"))
	{
		_deltaX =  message.getVariables().getInt("deltaX");
		_deltaY =  message.getVariables().getInt("deltaY");
	}
	
}
