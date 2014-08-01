#include "stdafx.h"
#include "DebugSelectLevel.h"
#include "Core/MessageManager.h"
#include "GameInfo.h"
#include "EditorUtils.h"
#include "Game.h"
#include "MyApplication.h"
#include "LevelInfoManager.h"

static const std::string debugFont = "EdirotTexts";
int offset_h = 15;
/*
 * Button
 */

DebugSelectLevel::Button::Button(const std::string& message, const std::string& text, const Color clr)
	: message(message)
	, text(text)
	, pressed(false)
	, _color(clr)
{
}

void DebugSelectLevel::Button::Draw()
{
	Render::device.SetTexturing(false);
	Render::BeginColor(_color);
	Render::DrawRect(rect);
	Render::EndColor();
	Render::device.SetTexturing(true);
	Render::FreeType::BindFont(debugFont);
	Render::PrintString(rect.x + (rect.width >> 1), rect.y + (rect.height >> 1), text, 1.3f, CenterAlign, CenterAlign);
}

bool DebugSelectLevel::Button::MouseDown(const IPoint& mouse_position)
{
	if (rect.Contains(mouse_position)) {
		pressed = true;
	}
	return pressed;
}

void DebugSelectLevel::Button::MouseUp(const IPoint& mouse_position)
{
	if (pressed && rect.Contains(mouse_position)) {
		Core::messageManager.putMessage(Message(message));
	}
	pressed = false;
}

void DebugSelectLevel::Button::SetRect(const IRect& rect)
{
	this->rect = rect;
}


/*
 * DebugSelectLevel
 */


DebugSelectLevel::DebugSelectLevel(std::string name, rapidxml::xml_node<>* xmlElement)
	:GUI::Widget(name, xmlElement)
	, left("lua:DebugLeftPressed", "<")
	, right("lua:DebugRightPressed", ">")
	, select("lua:DebugSelectPressed", "", Color::BLUE)
	, current(-1)
	, _scroll(MyApplication::GAME_HEIGHT, 0.f, 0.f)
	, _fileListActive(false)
	, _appearTimer(0.f)
	, _appearMove(false)
	, _appearOnScreen(false)
	, _mouseDown(false)

{
	Core::LuaDoFile("scripts/selectFile.lua");
	int btn_size = Xml::GetIntAttributeOrDef(xmlElement, "size", 40);
	int btn_size2 = btn_size / 2;
	if (Xml::GetBoolAttributeOrDef(xmlElement, "vertical", false)) {
		left.SetRect(IRect(-btn_size2, -btn_size2 + btn_size, btn_size, btn_size));
		right.SetRect(IRect(-btn_size2, -btn_size2 - btn_size, btn_size, btn_size));
		select.SetRect(IRect(right.rect.LeftTop().x, right.rect.LeftTop().y, left.rect.RightBottom().x -  right.rect.LeftTop().x,  left.rect.RightBottom().y - right.rect.LeftTop().y).Inflated(-1));
		fileNameAlign = RightAlign;
	} else {
		left.SetRect(IRect(-btn_size2 - btn_size * 3, -btn_size2, btn_size, btn_size));
		right.SetRect(IRect(-btn_size2 + btn_size * 3, -btn_size2, btn_size, btn_size));
		select.SetRect(IRect(left.rect.RightBottom().x + 40, left.rect.RightBottom().y, right.rect.LeftTop().x -  left.rect.RightBottom().x - 80,  right.rect.LeftTop().y - left.rect.RightBottom().y));
		fileNameAlign = CenterAlign;
	}
	_screenRect = IRect(0, 0, MyApplication::GAME_WIDTH, MyApplication::GAME_HEIGHT);

	_font_h = Render::FreeType::getFontHeight(debugFont)*1.f + 20;

	//_scroll.SetContentsSize(Xml::GetIntAttribute(xml_root, "height"));
	_scroll.SetDeceleration(1000.f);
	_scroll.SetBounceDeceleration(1000.f);
	_scroll.SetMaxVelocity(3000.f);
	_scroll.SetMagnetNet(1.f);

}

void DebugSelectLevel::ReloadNames()
{
	if (gameInfo.IsDevMode())
	{
		fileNames.clear();
		std::vector<std::string> file_list;
		Core::fileSystem.FindFiles("Levels/*.xml", file_list);
		for(size_t i = 0, count = file_list.size(); i < count; ++i){
			std::string& filename = file_list[i];
			size_t fnd1 = filename.find_last_of("/");
			size_t fnd2 = filename.find(".xml");
			if( fnd1 != std::string::npos && fnd2 != std::string::npos) {
				fileNames.push_back(filename.substr(fnd1 + 1, fnd2 - fnd1 - 1));
			}
		}
		_scroll.SetContentsSize(fileNames.size()*_font_h);
		getChild("ListType")->AcceptMessage(Message("SetState", gameInfo.getGlobalBool("alphaBetListType", true) ?"0":"1" ));
	}
}

void DebugSelectLevel::SetLevel(const std::string& levelName)
{
	current = 0;
	for (int i = 0; i < static_cast<int>(fileNames.size()); ++i) {
		if (fileNames[i] == levelName) {
			current = i;
			return;
		}
	}
}

void DebugSelectLevel::Draw()
{
	if((_appearOnScreen || _appearMove || _appearTimer > 0.f) && !fileNames.empty())
	{
		Render::device.PushMatrix();
		Render::device.MatrixTranslate(position);
		Render::device.SetTexturing(false);
		Render::BeginColor(Color(150, 150, 150, 220));
		Render::DrawRect(IRect(left.rect.x - 5, left.rect.y - 5, 10 + right.rect.RightTop().x - left.rect.x, 10 + right.rect.RightTop().y - left.rect.y));
		Render::EndColor();
		Render::device.SetTexturing(true);
		select.Draw();
		left.Draw();
		right.Draw();
		Render::FreeType::BindFont(debugFont);
		Render::PrintString(0, 0, fileNames[current], 0.8f, fileNameAlign, CenterAlign);
		Render::device.PopMatrix();


		if(_fileListActive)
		{
			Render::device.SetTexturing(false);
			Render::BeginColor(Color(0, 0, 0, 220));
			Render::DrawRect(_screenRect);
			Render::EndColor();
			Render::device.SetTexturing(true);

			Render::FreeType::BindFont(debugFont);
			//Render::device.MatrixTranslate(GameSettings::FIELD_SCREEN_CONST.LeftTop());
			int count = fileNames.size();
			IPoint pos = IPoint(_screenRect.Width()/2, int(_scroll.GetPosition()));

			if(current  < count)
			{
				Render::device.SetTexturing(false);
				Render::BeginColor(Color(200, 200, 200, 200));
				IPoint p = pos + IPoint(0, _font_h*current);
				Render::DrawRect(IRect(2, p.y - offset_h, _screenRect.Width()-4, _font_h));
				Render::EndColor();
				Render::device.SetTexturing(true);
			}


			int i_start = math::max(-pos.y/_font_h, 0); 
			int i_finish = math::min((-pos.y + _screenRect.Height())/_font_h, count-1);
			for(int i = i_start; i <= i_finish; i++)
			{
				IPoint p = pos + IPoint(0, _font_h*i);
				Render::PrintString(pos.x, p.y, fileNames[i], 1.f, CenterAlign, BottomAlign);
			}
		}
	}
}

bool DebugSelectLevel::MouseDown(const IPoint &mouse_pos)
{
	if (gameInfo.IsDevMode())
	{
		_mouseDown = true;
		_mouseDownPos = mouse_pos;
		if(IRect(0,0, 100, 100).Contains(mouse_pos) || _appearOnScreen)
		{
			_appearMove = true;
		}else{
			_appearMove = false;
		}
		if(_fileListActive)
		{
			IPoint pos = IPoint(_screenRect.Width()/2, int(_scroll.GetPosition()));
			current = math::clamp(0, (int)fileNames.size(), (mouse_pos.y - pos.y + offset_h)/_font_h); 
			_scroll.MouseDown(mouse_pos.y);
			return true;
		}
		if(IsButtonsActive())
		{
			return left.MouseDown(mouse_pos - position) || right.MouseDown(mouse_pos - position) || select.MouseDown(mouse_pos - position);
		}
	}
	return false;
}

void DebugSelectLevel::Update(float dt)
{
	if (_appearOnScreen || _appearMove || _appearTimer > 0.f)
	{
		_scroll.Update(dt);
		if(_fileListActive)
		{
			return;
		}
		if(!_appearMove)
		{
			if(_appearTimer > 0.5f)
			{
				_appearTimer += dt*3.f;
				if(_appearTimer >= 1)
				{
					_appearTimer = 1.f;
					_appearOnScreen = true;
				}
			}else if(_appearTimer <= 0.5f)
			{
				_appearTimer -= dt*3.f;
				if(_appearTimer <= 0)
				{
					_appearTimer = 0.f;
					_appearOnScreen = false;
				}
			}
		}
		position.x = 310 - 768.f*(1.f - _appearTimer);
		position.y = 40;
	}
}

bool DebugSelectLevel::IsAlphaBetType()
{
	return gameInfo.getGlobalBool("alphaBetListType", true);
}

bool DebugSelectLevel::IsButtonsActive() const
{
	return _appearOnScreen && _appearTimer>= 1.f;
}


void DebugSelectLevel::MouseMove(const IPoint &mouse_pos)
{
	if (_mouseDown)
	{
		if(_fileListActive)
		{
			_scroll.MouseMove(mouse_pos.y);
			return;
		}else if(_appearMove){
			if(mouse_pos.y < 100)
			{
				int delta = (mouse_pos.x - _mouseDownPos.x);
				if(!_appearOnScreen)
				{
					if(delta > 100)
					{
						_appearTimer = math::clamp(0.f, 1.f, (delta - 100.f)/200.f);
					}
				}else{
					delta = -delta;
					if(delta > 100)
					{
						_appearTimer = math::clamp(0.f, 1.f, 1.f - (delta - 100.f)/200.f);
					}
				}
			}else{
				_appearMove = false;
			}
		}
	}
}

void DebugSelectLevel::MouseUp(const IPoint &mouse_pos)
{
	if (_mouseDown)
	{
		_mouseDown = false;
		_appearMove = false;
		if(_fileListActive)
		{
			_scroll.MouseUp(mouse_pos.y);
			if(math::abs(mouse_pos.y - _mouseDownPos.y) < 15)
			{
				_fileListActive = false;
				IPoint pos = IPoint(_screenRect.Width()/2, int(_scroll.GetPosition()));
				current = math::clamp(0, (int)fileNames.size(), (mouse_pos.y - pos.y + 20)/_font_h); 
				Core::messageManager.putMessage(Message("lua:DebugLevelPressed"));
				// лик по уровню
			}
			return;
		}
		if(IsButtonsActive())
		{
			left.MouseUp(mouse_pos - position);
			right.MouseUp(mouse_pos - position);
			select.MouseUp(mouse_pos - position);
		}
	}
}

void DebugSelectLevel::AcceptMessage(const Message &message)
{
	if (message.is("Layer", "LayerInit") && current == -1)
	{
		ReloadNames();
		SetLevel( gameInfo.getLocalString("DEBUG:EditorUtils::lastLoadedLevel") );
		AcceptMessage(Message("OnLoadLevel", "Find"));
	}
	else if(message.is("next"))
	{
		if(IsAlphaBetType())
		{
			++current;
			if (current >= static_cast<int>(fileNames.size())) {
				current = 0;
			}
		}else{
			std::string current_name = fileNames[current];
			gameInfo.setLocalString("DEBUG:EditorUtils::lastLoadedLevel", "");
			int level_num = levelsInfo.GetLevelIndex(fileNames[current])+1;
			std::string name_next = levelsInfo.GetLevelName(level_num);
			if(name_next == current_name)
			{
				name_next = levelsInfo.GetLevelName(0);
			}
			if(!name_next.empty())
			{
				for(size_t i = 0; i < fileNames.size(); i++)
				{
					if(name_next == fileNames[i])
					{
						current = i;
						break;
					}
				}
			}
		}
	}
	else if(message.is("prev"))
	{
		if(IsAlphaBetType())
		{
			--current;
			if (current < 0) {
				current = static_cast<int>(fileNames.size() - 1);
			}
		}else{
			std::string current_name = fileNames[current];
			gameInfo.setLocalString("DEBUG:EditorUtils::lastLoadedLevel", "");
			int level_num = levelsInfo.GetLevelIndex(fileNames[current])-1;
			std::string name_next;
			if(level_num < 0)
			{
				name_next = levelsInfo.GetLevelName(levelsInfo.GetSize()-1);
			}else
			{
				name_next = levelsInfo.GetLevelName(level_num);
			}
			if(!name_next.empty())
			{
				for(size_t i = 0; i < fileNames.size(); i++)
				{
					if(name_next == fileNames[i])
					{
						current = i;
						break;
					}
				}
			}
		}
	}
	else if(message.is("OnLoadLevel"))
	{
		if(fileNames.empty())
		{
			return;
		}
		if(message.getData() == "Find")
		{
			std::string level_name = levelsInfo.GetLevelName(gameInfo.getLocalInt("current_level", 0));
			for(size_t i = 0; i < fileNames.size(); i++)
			{
				if(fileNames[i] == level_name)
				{
					current = i;
					break;
				}
			}
		}
		int level_num = levelsInfo.GetLevelIndex(fileNames[current]);
		std::string text = utils::lexical_cast(level_num + 1);
		if(level_num < 0)
		{
			text = ".";
		}
		getChild("ListType")->AcceptMessage(Message("SetCaption", text));
		_scroll.SetPosition((1-current)*_font_h + MyApplication::GAME_WIDTH/2);
	}
	else if (message.is("LoadLevelForEdit"))
	{
		SetLevel( EditorUtils::lastLoadedLevel );
	}
	else if (message.is("ReloadNames"))
	{
		ReloadNames();
	}
	else if (message.is("SetLevel")) {
		current = message.getIntegerParam();
		if (current < 0 || current >= static_cast<int>(fileNames.size())) {
			current = 0;
		}
	}
	else if(message.is("fileListShow"))
	{
		_fileListActive = true;
	}
	else if(message.is("fileListHide"))
	{
		_fileListActive = false;
	}
}

Message DebugSelectLevel::QueryState(const Message& message) const
{
	if (message.is("CurrentLevel"))
	{
		return Message(fileNames.empty() ? "" : fileNames[current]);
	}
	Assert(false);
	return Message("");
}

void DebugSelectLevel::MouseWheel(int delta)
{
}