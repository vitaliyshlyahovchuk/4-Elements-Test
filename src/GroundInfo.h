#ifndef _GROUND_MAKER_
#define _GROUND_MAKER_
#include "BaseEditorMaker.h"

class GameField;

namespace Gadgets
{
	struct WoodInfo
	{
		int wood;
		bool isGrowingWood;
		bool isGoldWood;

		WoodInfo()
			: wood(0), isGrowingWood(false), isGoldWood(false) 
		{}
	};

	class GroundMaker
		: public BaseEditorMaker
	{
	private:
		Game::Square* _prev_square;
		std::map<Game::FieldAddress, WoodInfo> _info;
		std::map<Game::FieldAddress, WoodInfo> _clipboard;

		void Apply();
		void Editor_ClearFieldPart(IRect part);
		void Editor_CopyToClipboard(IRect part);
	public:
		void Init(GameField *field, const bool &with_editor);
		void LoadLevel(rapidxml::xml_node<> *root);
		void SaveLevel(Xml::TiXmlElement *root);
		
		void Clear();
		void Release();
		bool Editor_LeftMouseDown(const IPoint &mouse_pos, Game::Square *sq);
		bool Editor_MouseMove(const IPoint &mouse_pos, Game::Square *sq);
		bool Editor_RightMouseDown(const IPoint &mouse_pos, Game::Square *sq);
		bool Editor_MouseUp(const IPoint &mouse_pos, Game::Square *sq);
		void Editor_CutToClipboard(IRect part);
		bool Editor_PasteFromClipboard(IPoint pos);
	};

}//Gadgets

#endif //_GROUND_MAKER_