#ifndef _CHIP_INFO_
#define _CHIP_INFO_
#include "BaseEditorMaker.h"
#include "GameColor.h"
#include <map>

class GameField;

namespace Gadgets
{

	struct InfoC
	{
		int act_count;
		int time_bomb;
		int star;
		int treasure;
		int lock;
		bool adapt;
		Game::ChipColor::Type type;
		int pre_chip;
		bool pre_chameleon;
		bool kill_diamond;
		Game::Hang level_hang;

		InfoC()
			: star(0)
			, treasure(0)
			, act_count(0)
			, time_bomb(0)
			, lock(0)
			, adapt(false)
			, pre_chip(-1)
			, pre_chameleon(false)
			, kill_diamond(false)
			, type(Game::ChipColor::CHIP)
		{
		}

		bool IsEmpty() const
		{
			if(act_count > 0){
				return false;
			}
			if(time_bomb > 0){
				return false;
			}
			if(lock > 0){
				return false;
			}
			if(type != Game::ChipColor::CHIP) {
				return false;
			}
			if(adapt){
				return false;
			}
			if(!level_hang.IsEmpty()){
				return false;
			}
			if(pre_chip != -1){
				return false;
			}
			if(pre_chameleon){
				return false;
			}
			return true;
		}
	};

	class ChipsInfo
		: public BaseEditorMaker
	{
	private:
		int _current_pre_chip;
		int _chip_arrows_dir;
		FPoint _chip_arrows_center;
		int _chip_arrows_length;
		Game::FieldAddress _chip_arrows_addres;
		std::map<Game::FieldAddress, InfoC> _info;
		std::map<Game::FieldAddress, InfoC> _clipboard;
		Game::Square *_prev_square;

		void Editor_ClearFieldPart(IRect part);
		void Editor_CopyToClipboard(IRect part);
	public:
		void Init(GameField *field, const bool &with_editor);
		void LoadLevel(rapidxml::xml_node<> *root);
		void SaveLevel(Xml::TiXmlElement *root);
		void Apply();
		void Clear();
		void Release();
		bool Editor_LeftMouseDown(const IPoint &mouse_pos, Game::Square *sq);
		bool Editor_RightMouseDown(const IPoint &mouse_pos, Game::Square *sq);
		bool Editor_MouseMove(const IPoint &mouse_pos, Game::Square *sq);
		bool Editor_MouseWheel(int delta, Game::Square *sq);
		void DrawEdit();
		bool AcceptMessage(const Message &message);
		
		void Editor_CutToClipboard(IRect part);
		bool Editor_PasteFromClipboard(IPoint pos);
	};
}//Gadgets

#endif //_CHIP_INFO_