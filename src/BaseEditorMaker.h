#ifndef _BASE_EDITOR_MAKER_H_
#define _BASE_EDITOR_MAKER_H_

#include "GameOrder.h"

class GameField;
namespace Game
{
	struct Square;
}

namespace Gadgets
{
	class BaseEditorMaker //Абстрактный класс для всех редакторов 
	{
	protected:
		GameField *_field;
	public:
		BaseEditorMaker();
		virtual ~BaseEditorMaker() {};
		virtual void Init(GameField *field, const bool &with_editor);
		virtual void LoadLevel(rapidxml::xml_node<> *root){};
		virtual void SaveLevel(Xml::TiXmlElement *root){};
		virtual void Clear(){};
		virtual void Release(){};
		virtual bool Editor_LeftMouseDown(const IPoint &mouse_pos, Game::Square *sq) = 0;
		virtual bool Editor_RightMouseDown(const IPoint &mouse_pos, Game::Square *sq){return false;}
		virtual bool Editor_MouseUp(const IPoint &mouse_pos, Game::Square *sq){return false;}
		virtual bool Editor_MouseMove(const IPoint &mouse_pos, Game::Square *sq){ return false;}
		virtual bool Editor_MouseWheel(int delta, Game::Square *sq){return false;}
		virtual void DrawEdit(){};
		virtual bool AcceptMessage(const Message &message){return false;}

		virtual void Editor_CutToClipboard(IRect part) {}
		virtual bool Editor_PasteFromClipboard(IPoint pos) { return true;}

		virtual Game::Order::HardPtr* Editor_SelectOrder(IPoint mouse_pos, Game::Square *sq) { return NULL; }
	};

}//Gadgets

#endif //_BASE_EDITOR_MAKER_H_