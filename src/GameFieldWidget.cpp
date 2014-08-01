#include "stdafx.h"

#include "GameFieldWidget.h"
#include "GameField.h"
#include "MyApplication.h"
#include "GameInfo.h"
#include "EditorUtils.h"

GameFieldWidget::GameFieldWidget(const std::string& name_, rapidxml::xml_node<>* elem_)
	: Widget(name_)
{
}


bool GameFieldWidget::MouseDown(const IPoint& mouse_pos)
{
	return GameField::gameField->MouseDown(GameSettings::CorrectMousePos(mouse_pos));
}

void GameFieldWidget::MouseDoubleClick(const IPoint& mouse_pos)
{
    GameField::gameField->MouseDoubleClick(GameSettings::CorrectMousePos(mouse_pos));
}


void GameFieldWidget::MouseUp(const IPoint& mouse_pos)
{
    GameField::gameField->MouseUp(GameSettings::CorrectMousePos(mouse_pos));
}

void GameFieldWidget::MouseMove(const IPoint& mouse_pos)
{
    GameField::gameField->MouseMove(GameSettings::CorrectMousePos(mouse_pos));
}

void GameFieldWidget::MouseWheel(int delta)
{
    GameField::gameField->MouseWheel(delta);
}

void GameFieldWidget::Draw()
{
    GameField::gameField->Draw();
}

void GameFieldWidget::Update( float time )
{
	GameField::gameField->Update( time );
}

void GameFieldWidget::AcceptMessage(const Message& message)
{
    if(GameField::gameField != NULL) {
        GameField::gameField->AcceptMessage(message);
 
	}
	if(message.is("CreateGameField"))
	{
		MyAssert(GameField::gameField == NULL);
		GameField::gameField = new GameField();
		for( MessagesList::iterator it =  _precreate_messages_list.begin(); it != _precreate_messages_list.end(); ++it)
		{
			GameField::gameField->AcceptMessage((*it));
		}
	}else if(message.is("ReleaseGameField"))
	{
		delete GameField::gameField;
		GameField::gameField = NULL;
    } else {
        _precreate_messages_list.push_back(message);
    }
}

Message GameFieldWidget::QueryState(const Message& QueryState) const
{
    return GameField::gameField->QueryState(QueryState);
}

GameFieldUp::GameFieldUp(const std::string& name_, rapidxml::xml_node<>* elem_)
	:GUI::Widget(name_, elem_)
{
}

void GameFieldUp::Draw()
{
	if(GameSettings::isGlobalScaleExist)
	{
		Render::PushMatrix();
		Render::device.MatrixTransform(GameSettings::globalTransform);
	}
	GameField::gameField->DrawUp();
}

BlackFrame::BlackFrame(const std::string& name_, rapidxml::xml_node<>* elem_)
	: GUI::Widget(name_, elem_)
{
	_isIPhone5 = gameInfo.getGlobalString("device_id") == "iphone5";
}

void BlackFrame::Draw()
{	
	if(_isIPhone5)
	{
		return;
	}
	//Черное небытие вокруг
	Render::device.SetTexturing(false);
	Render::BeginColor(Color::BLACK);
	
	IRect rect_out = IRect(0, 0, MyApplication::GAME_WIDTH, MyApplication::GAME_HEIGHT);
	IRect rect(0,0,640, 960);
	IRect rect_in = IRect((MyApplication::GAME_WIDTH - rect.width)/2, (MyApplication::GAME_HEIGHT - rect.height)/2,rect.width,rect.height);
	
	//Render::DrawRect(0,0, rect_in.x, rect_out.height);
	//Render::DrawRect(rect_in.x + rect_in.width,0, rect_in.x, rect_out.height);
	Render::DrawRect(0,0, rect_out.width, rect_in.y);
	Render::DrawRect(0,rect_in.y + rect_in.height, rect_out.width, rect_in.y);

	Render::EndColor();
	Render::device.SetTexturing(true);
}
