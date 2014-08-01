#pragma once
#include "GUI/Widget.h"
#include "Game.h"
#include "GameOrder.h"

namespace EditorUtils
{
	class FieldClipboard
	{
		std::vector< std::vector<Game::Square*> > _data;
		IRect _rect;
	public:
		void Init(IRect rect)
		{
			_rect = rect;
			_data.clear();
			_data.resize(rect.width);
			for(int i = 0; i < rect.width; ++i){
				_data[i].resize(rect.height, Game::bufSquare);
			}
		}

		void Release()
		{
			_data.clear();
		}

		IRect GetRect() const
		{
			return _rect;
		}

		void Set(int x, int y, Game::Square *value)
		{
			_data[x][y] = value;
		}

		Game::Square *Get(int x, int y) const
		{
			return _data[x][y];
		}
	};

	class EditorWidget
		: public GUI::Widget
	{
	private:
		bool _mouseDown;
		float _localTime;
		// ���������� ������, �� ������� ���� ������ ����� ������ ����
		IPoint _mouseDownPos;					

		int _wallColorBrush;
		Render::Texture *chipsTex;

		// �������������� �������
		Game::Order::HardPtr *_selectedOrder;
		IPoint _selectedOrderAddress;
		std::set<Game::FieldAddress> _selectedOrderArea;
	public:
		EditorWidget(const std::string& name_, rapidxml::xml_node<>* elem_);
		~EditorWidget();
		void Update(float dt);
		void Draw();
		bool MouseDown(const IPoint &mouse_pos);
		void MouseMove(const IPoint &mouse_pos);
		void MouseUp(const IPoint &mouse_pos);
		void AcceptMessage(const Message &message);
		void MouseWheel(int delta);

	private:
		void Editor_DrawFieldLattice();
		void Editor_DrawMoveTool();
		void Editor_DrawFieldRadar();
		void Editor_DrawColoredWallTool();
		void Editor_DrawStartRectTool();

		// ����� ������, ����������� ������� ������� ����.
		// ����������� ���� �� �������������, �.�. ����� ������ �������� � ����������� �� ������, � ���� �� �������
		// ������������� �����, �� ����� ������� ������� ��������� �� ���� � �� �� ������ � ������ ������ ���� �
		// �����-������ ��������
		void Editor_CutToClipboard(IRect part);
		bool Editor_PasteFromClipboard(IPoint pos);
		void Editor_ClearFieldPart(IRect part, IRect preservePart); // preservePart - ������� ��� ������ � ����, �� �� �� ������
		FieldClipboard _clipboard;

		// ������� c������ �c� �������� ���� � ����� ������ ���� 
		// ��cc��� c ��c����� � ���� ����c�. � c������� ���������c�
		// �c� �������� � �c� ������� �� ����, ������� ����� ��������
		//void Editor_MoveFieldOptimize();
		void Editor_MoveFieldBy(int dx, int dy);
		bool Editor_CanMoveFieldBy(int dx, int dy);

		bool Editor_CheckMinigamesArea(const IRect& part);
		int Editor_MoveCheckIntersection(const IRect& part, const IPoint& delta);

		//������� ���� � ������ ������������ �������
		bool Editor_PressMoveToolButton(int key);
	};
}//EditorUtils