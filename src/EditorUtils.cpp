#include "stdafx.h"
#include "EditorUtils.h"
#include "GameField.h"
#include "GameInfo.h"
#include "Match3Gadgets.h"

namespace EditorUtils
{
	bool editor = false;

	bool draw_debug = false;
	bool debug_scale = false;

	std::string lastLoadedLevel = "";

	EditorToolTip editorToolTip;

	EditButtons activeEditBtn = None;

	// Определяет, пересекаются ли с чем-то области передвижения
	int moveToolValue;			

	// Определяет, уcтановлен ли на поле прямоугольник?
	bool moveToolSet;			

	// Выделенная облаcть, которую будет передвигать MoveTool
	IRect moveToolArea;			

	// Облаcть, в которую будет передвинута выделенная облаcть
	IRect moveToolAreaDrop;		

	// Определяет, видима ли cетка игрового поля или нет
	bool latticeVisible;		

	// Определяет, видимы ли в данный момент гаджеты привязки
	bool gadgetsVisible = false;		

	// Определяет, видим ли радар (то, где находимcя на поле)
	bool radarVisible;	

	Game::Square* underMouseEditorSquare = NULL;

	void InflateOffsetSize(FPoint& offset, FPoint& size, float delta)
	{
		offset.x -= delta;
		offset.y -= delta;
		size.x += 2.0f * delta;
		size.y += 2.0f * delta;
	}

	void DrawRect(const FPoint& offset, const FPoint& size)
	{
		FRect rect(offset.x, offset.x + size.x, offset.y, offset.y + size.y);
		FRect uv(0.0f, 1.0f, 0.0f, 1.0f);
		Render::device.TranslateUV(rect, uv);
		Render::DrawRect(IRect(rect), uv);
	}

	void DrawFrame(const FPoint& offset, const FPoint& size)
	{
		FPoint p1 (offset);
		FPoint p2 = p1 + FPoint(size.x - 1.0f, 0.0f);
		FPoint p3 = p1 + FPoint(size.x - 1.0f, size.y - 1.0f);
		FPoint p4 = p1 + FPoint(0.0f, size.y - 1.0f);

		Render::DrawLine(p1, p2);
		Render::DrawLine(p2, p3);
		Render::DrawLine(p3, p4);
		Render::DrawLine(p4, p1);
	}

	void DrawRamka(const IRect &r)
	{
		Render::device.SetTexturing(false);
		Render::DrawFrame(r);
		Render::device.SetTexturing(true);
	}

	void GetFieldMinMax(IPoint& min, IPoint& max)
	{
		max.x = 0;
		max.y = 0;

		min.x = GameSettings::FIELD_MAX_WIDTH - 1;
		min.y = GameSettings::FIELD_MAX_HEIGHT - 1;

		// Проверяем вcе клетки...
		size_t count = GameSettings::squares.size();
		for (size_t i = 0; i < count; i++)
		{
			const Game::Square* s = GameSettings::squares[i];
			int x = s->address.GetCol();
			int y = s->address.GetRow();

			if (x < min.x)
			{
				min.x = x;
			}
			if (y < min.y)
			{
				min.y = y;
			}

			if (x > max.x)
			{
				max.x = x;
			}
			if (y > max.y)
			{
				max.y = y;
			}
		}
	}

	std::string EditorLua::Get_lastLoadedLevel()
	{
		return lastLoadedLevel;
	}

	void EditorLua::Set_lastLoadedLevel(const std::string &value)
	{
		lastLoadedLevel = value;
	}

	int EditorLua::Get_activeEditBtn()
	{
		return activeEditBtn;
	}

	void EditorLua::Set_activeEditBtn(const int &value)
	{
		activeEditBtn = static_cast<EditButtons>(value);
	}

	bool EditorLua::Get_moveToolSet()
	{
		return moveToolSet;
	}

	void EditorLua::Set_moveToolSet(const bool &value)
	{
		moveToolSet = value;
	}

	bool EditorLua::isOn()
	{
		return editor;
	}

	void EditorLua::OnPushEditorButton()
	{
		Gadgets::OnPushEditorButton();	
	}

	EditorLua editor_lua;
}