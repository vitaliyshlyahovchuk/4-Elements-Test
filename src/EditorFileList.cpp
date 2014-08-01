#include "stdafx.h"

#include "EditorFileList.h"
#include "GameField.h"
#include "Game.h"
#include "Match3Gadgets.h"

EditorFileList::EditorFileList(std::string name_, rapidxml::xml_node<>* xmlElement)
	: Widget("EditorFileList")
	, textXLeft(20)
	, textYUp(560)
	, textYDown(100)
	, textDx(128)
	, textDy(20)
	, _offsetColumn(0)
	, _columnCount(0)
	, _columnMax(0)
{
	_fileName = "level_11";
	_cursorPos = 3;
	_cursorTime = 0.f;
	_currentName = -1;
	_destLayer = "GameLayer";
	_destWidget = "GameField";
}

bool EditorFileList::MouseDown(const IPoint &mouse_pos)
{
	if (mouse_pos.y >= textYDown)
	{
		_currentName = _underMouseName;
	}
	return false;
}


void EditorFileList::MouseDoubleClick(const IPoint &mouse_pos)
{
	if (_currentName >= 0)
	{
		_fileName = _names[_currentName];
		_cursorPos = (int) _fileName.length();

		GUI::Widget *target = Core::guiManager.getLayer(_destLayer) -> getWidget(_destWidget);
		target->AcceptMessage(Message("LoadLevelForEdit", _fileName));
	}
}

void EditorFileList::MouseUp(const IPoint &mouse_pos)
{
}

void EditorFileList::MouseMove(const IPoint &mouse_pos_)
{
	int x = textXLeft;
	int y = textYUp;
	int dx = textDx;
	int dy = textDy;
	IPoint mouse_pos(mouse_pos_.x + _offsetColumn*dx, mouse_pos_.y);
	Render::FreeType::BindFont("debug");
	for (size_t i = 0; i < _names.size(); i++)
	{
		if (mouse_pos.x >= x && mouse_pos.x < x+Render::getStringWidth(_names[i]) &&
			mouse_pos.y >= y && mouse_pos.y < y+Render::getFontHeight())
		{
			_underMouseName = static_cast<int>(i);
		}

		y -= dy;
		if (y < textYDown)
		{
			y = textYUp;
			x += dx;
		}
	}
}

void EditorFileList::Update(float dt)
{
	_cursorTime += dt*2;
	if (_cursorTime >= 1.f)
	{
		_cursorTime = 0.f;
	}
}

void EditorFileList::Draw()
{
	int x = textXLeft;
	int y = textYUp;
	int dx = textDx;
	int dy = textDy;
	_columnCount = 0;
	_columnMax = 0;
	Render::BindFont("debug");

	Render::device.PushMatrix();
	Render::device.MatrixTranslate(math::Vector3(-_offsetColumn*dx, 0.f, 0.f));

	for (size_t i = 0; i < _names.size(); i++)
	{
		if (i != _currentName){
			Render::BeginColor(Color(255, 255, 0, 200));
		} else {
			Render::BeginColor(Color(255, 0, 0, 255));
		}

		if (i == _underMouseName)
		{
			int w = Render::getStringWidth(_names[i]);
			int h = Render::getFontHeight();

			Render::device.SetTexturing(false);
			Render::DrawFrame(IRect(x-10, y-3, w+20, h));
			Render::device.SetTexturing(true);
		}

		Render::PrintString(IPoint(x, y), _names[i], 1.f, LeftAlign, BottomAlign);
		y -= dy;
		if (y < textYDown)
		{
			y = textYUp;
			x += dx;
			_columnCount++;
			if(x < Render::device.Width()){
				_columnMax++;
			}
		}

		Render::EndColor();
	}
	Render::device.PopMatrix();

	IPoint editBoxPoint(100, 15);
	IPoint editBoxText(editBoxPoint.x + 20, editBoxPoint.y + 4);
	IRect editBoxRect(editBoxPoint.x, editBoxPoint.y, 250, 25);

	Render::device.SetTexturing(false);
	Render::BeginColor(Color(150, 150, 150, 190));
	Render::DrawRect(editBoxRect);
	Render::DrawFrame(editBoxRect);
	Render::EndColor();
	Render::device.SetTexturing(true);

	Render::BindFont("debug");
	Render::PrintString(editBoxText, _fileName, 1.f, LeftAlign, BottomAlign);

	int a = static_cast<int>(128+127*cosf(2*math::PI*_cursorTime));
	if (a<0) a = 0;
	Render::BeginColor(Color(0, 255, 0, a));

	IPoint cursorOffset (editBoxText);
	Render::getStringWidth(_fileName.substr(0, static_cast<size_t>(_cursorPos)));
	cursorOffset.x += Render::getStringWidth(_fileName.substr(0, static_cast<size_t>(_cursorPos)));
	cursorOffset.y += -2;

	Render::PrintString(cursorOffset, "_", 1.f, LeftAlign, BottomAlign);

	Render::EndColor();
}

void EditorFileList::AcceptMessage(const Message &message)
{
#ifdef ENGINE_TARGET_WIN32
	if (message.is("KeyPress"))
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
			if (_cursorPos < static_cast<int>(_fileName.length()))
			{
				_cursorPos++;
			}
			_cursorTime = 0.f;
		} else if (key == -VK_UP)
		{
			_cursorPos = static_cast<int>(_fileName.length());
			_cursorTime = 0.f;
		} else if (key == -VK_DOWN)
		{
			_cursorPos = 0;
			_cursorTime = 0.f;
		} else if (data > 0 && key > ' ' && _fileName.length() < 20)
		{
			_fileName.insert(static_cast<size_t>(_cursorPos), 1, key);
			_cursorPos++;
			_cursorTime = 0.f;
		} else if (key == 8)
		{
			if (_cursorPos > 0)
			{
				_cursorPos--;
				_fileName.erase(static_cast<size_t>(_cursorPos), 1);
			}
		}
	} else
#endif
	if (message.is("Init"))
	{
		_names.clear();
		StringVector vec = File::DirectoryListing::Get("Levels/*.*");
		size_t count = vec.size();
		_names.reserve(count);
		int fnd0;
		int fnd1;
		for(size_t i=0;i<count;i++){
			fnd0 = vec[i].find("/");
			fnd1 = vec[i].find(".xml");
			if( fnd0>0 && fnd1 > 0){
				_names.push_back( vec[i].substr(fnd0+1,fnd1-fnd0-1));
			}
		}
	/*} else if (message.is("SetDestLayer"))
	{
		_destLayer = message.getData();
	} else if (message.is("SetDestWidget"))
	{
		_destWidget = message.getData();*/
	} else if (message.is("SetLevelName"))
	{
		_fileName = message.getData();
		_cursorPos = (int) _fileName.length();
	} else if (message.is("AddItem"))
	{
		_names.push_back(message.getData());
	} else if (message.is("Load")){
		if (_currentName >= 0)
		{
			_fileName = _names[_currentName];
			_cursorPos = (int) _fileName.length();

			GUI::Widget *target = Core::guiManager.getLayer(_destLayer) -> getWidget(_destWidget);
			target -> AcceptMessage(Message("LoadLevelForEdit", _fileName));
			Core::messageManager.putMessage(Message("Return", "press"));
		}
	} else if (message.is("Save"))
	{
		bool f = false;
		for (size_t i = 0; i<_names.size(); i++)
		{
			if (_fileName == _names[i])
			{
				f = true;
			}
		}
		if (f)
		{
			Core::guiManager.getLayer("ConfirmRewriteLevel")->getWidget("ConfirmText")->
				AcceptMessage(Message("SetString", "Do you really want to rewrite " + _fileName + "?"));
			Core::messageManager.putMessage(Message("ShowConfirmRewrite"));
		} else {
			Core::guiManager.getLayer(_destLayer)->getWidget(_destWidget)->AcceptMessage(Message("SaveLevel", _fileName));
			Core::messageManager.putMessage(Message("Return", "press"));

		}
	} else if (message.is("SaveAlways"))
	{
		Core::guiManager.getLayer(_destLayer)->getWidget(_destWidget)->AcceptMessage(Message("SaveLevel", _fileName));
		Core::messageManager.putMessage(Message("Return", "press"));
	} else if (message.is("Delete"))
	{
		size_t i = static_cast<size_t>(_currentName);
		if(i >= _names.size())
		{
			return;
		}
		std::string filename = _names[i];
		if (filename == _fileName)
		{
			Log::log.WriteInfo("Вы не можете удалить текущий загруженый уровень!");
			return;
		}
		std::string full_name = "Levels/" + filename + ".xml";
		if(File::Exists(full_name)){
			File::rm(full_name);
			AcceptMessage(Message("Init"));
		}
	} else if (message.is("ResaveAll")) {  //открыть и пересохранить все файлы

		GUI::Widget *target = Core::guiManager.getLayer(_destLayer) -> getWidget(_destWidget);

		for (auto filename: _names)
		{
			target -> AcceptMessage(Message("LoadLevelForEdit", filename));
			target -> AcceptMessage(Message("SaveLevel", filename));
		}
		//target -> AcceptMessage(Message("LoadLevelForEdit", _fileName));

	}
}

void EditorFileList::MouseWheel(int delta)
{
	_offsetColumn -= delta;
	_offsetColumn = math::clamp(0, _columnCount - _columnMax, _offsetColumn);
	MouseMove(Core::mainInput.GetMousePos());
}


ChipSelecter::ChipSelecter(std::string name_, rapidxml::xml_node<>* xmlElement)
	: Widget("ChipSelecter")
	, _chipXLeft(270)
	, _chipYDown(235)
	, _timer(0.f)
	, _underMouseChip(-1)
{
	_chipsTex = Core::resourceManager.Get<Render::Texture>("Chips");
}

void ChipSelecter::MouseDoubleClick(const IPoint &mouse_pos)
{
	MouseDown(mouse_pos);
}

bool ChipSelecter::MouseDown(const IPoint &mouse_pos)
{
	if (_underMouseChip>=0)
	{
		bool f = false;
		std::vector<int>::iterator i = _chipColors.begin(), e = _chipColors.end();
		for (; i!=e; i++)
		{
			if ((*i) == _underMouseChip)
			{
				f = true;
				_chipColors.erase(i);
				break;
			}
		}
		if (!f)
		{
			_chipColors.push_back(_underMouseChip);
		}
	}
	return false;
}

void ChipSelecter::MouseUp(const IPoint &mouse_pos)
{
}

void ChipSelecter::MouseMove(const IPoint &mouse_pos)
{
	_underMouseChip = -1;
	for (int i = 0; i<7; i++)
	{
		int color = i;
		int x = _chipXLeft + 50*(color%7);
		int y = _chipYDown + 50*(3 - color/7);

		if (mouse_pos.x > x-GameSettings::SQUARE_SIDE/2 && mouse_pos.x < x+GameSettings::SQUARE_SIDE/2 &&
			mouse_pos.y > y-GameSettings::SQUARE_SIDE/2 && mouse_pos.y < y+GameSettings::SQUARE_SIDE/2)
		{
			_underMouseChip = i;
		}
	}
}

void ChipSelecter::Update(float dt)
{
	_timer += dt;
}

void ChipSelecter::Draw()
{
	Render::device.SetTexturing(false);
	Render::BeginColor(Color(100, 100, 100, 180));
	Render::DrawRect(IRect(200, 150, 400, 300));
	Render::EndColor();
	
	Render::device.SetTexturing(true);
	
	//IRect rect = Game::ChipColor::DRAW_RECT;
	//Тут должно быть 44х44!
	IRect rect = IRect(0,0,44,44);

	for (int i = 0; i <= 6; i++)
	{
		int x = _chipXLeft + 50*(i%7);
		int y = _chipYDown + 50*(3 - i/7);
		FRect uv = Game::GetChipRect(i, false, false, false);
		_chipsTex->Draw(rect.MovedBy(IPoint(x - rect.width/2, y - rect.height/2)), uv);
	}

	for (size_t i = 0; i < _chipColors.size(); i++)
	{
		int color = _chipColors[i];
		int x = _chipXLeft + 50*(color%7);
		int y = _chipYDown + 50*(3 - color/7);

		Render::BeginColor(Color(100, 255, 255, 255));
		for (int q = -1; q<=1; q++)
		{
			for (int w = -1; w<=1; w++)
			{
				DrawRamka(x+q, y+w);
			}
		}
		Render::EndColor();

		FRect uv = Game::GetChipRect(color, false, false, false);	

		Render::device.SetAlpha(static_cast<int>(150+80*sinf(_timer*6 + x - y/3)));
		Render::device.SetBlendMode(Render::ADD);
		_chipsTex->Draw(rect.MovedBy(IPoint(x - rect.width/2, y - rect.height/2)), uv);
		Render::device.SetBlendMode(Render::ALPHA);
	}

	if (_underMouseChip >= 0)
	{
		int color = _underMouseChip;
		int x = _chipXLeft + 50*(color%7);
		int y = _chipYDown + 50*(3 - color/7);
		Render::BeginColor(Color(0, 255, 0, 255));
		DrawRamka(x, y);
		Render::EndColor();
	}

	Render::BindFont("debug");
	Render::PrintString(IPoint(370, 170), utils::lexical_cast(static_cast<int>(_chipColors.size())) + " colors", 1.f, CenterAlign);
}

void ChipSelecter::DrawRamka(int x, int y)
{
	//Тут должно быть 44х44!
	IRect rect = IRect(0,0,44,44);

	Render::device.SetTexturing(false);
	Render::DrawFrame(rect.MovedBy(IPoint(x - rect.width/2, y - rect.height/2)));
	Render::device.SetTexturing(true);
}

void ChipSelecter::AcceptMessage(const Message &message)
{
	if (message.is("Init"))
	{
		_chipColors.clear();
		size_t count = Gadgets::levelColors.GetCount();
		_chipColors.resize(count);
		for(size_t i = 0; i < count; i++)
		{
			_chipColors[i] = Gadgets::levelColors[i];
		}
	}
	else if (message.is("ApplyColors"))
	{
		Gadgets::levelColors.ApplyColors(_chipColors);
	}
}






