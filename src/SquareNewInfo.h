#ifndef _SQUARE_NEW_INFO_
#define _SQUARE_NEW_INFO_
#include "BaseEditorMaker.h"

class GameField;

namespace Gadgets
{
	struct InfoSquare
	{
		std::string portal;
		bool is_short_square;
		bool forbid_bonus;
		bool wallRemovedWithSeqFromEnergy;
		bool growingWall;
		bool growingWall_fabric;
		bool isNoChainOnStart;
		bool kill_diamonds;
		int treasure; // 0 - нет сокровища, 1-3 уровень сокровища
		int en_source;

		InfoSquare();

		bool empty() const;
	};

	class SquareNewInfo
		: public BaseEditorMaker
	{
	private:
		size_t _current_portal;
		std::vector<std::string> _PORTAL_IDS;
		Game::FieldAddress addres_from;
		Game::Square *_prev_square;
	public:
		std::map<Game::FieldAddress, InfoSquare> _info;
		std::map<Game::FieldAddress, InfoSquare> _clipboard;
		std::string _type;
	private:
		void Editor_ClearFieldPart(IRect part);
		void Editor_CopyToClipboard(IRect part);
		void EnergySource_UpgradeIndexes();
	public:
		void Init(GameField *field, const bool &with_editor);
		void LoadLevel(rapidxml::xml_node<> *root);
		void SaveLevel(Xml::TiXmlElement *root);
		void Apply(const Game::FieldAddress &address);
		void Apply();
		void Clear();
		void Release();
		bool Editor_LeftMouseDown(const IPoint &mouse_pos, Game::Square *sq);
		bool Editor_RightMouseDown(const IPoint &mouse_pos, Game::Square *sq);
		bool Editor_MouseMove(const IPoint &mouse_pos, Game::Square *sq);
		bool Editor_MouseWheel(int delta, Game::Square *sq);
		
		void Editor_CutToClipboard(IRect part);
		bool Editor_PasteFromClipboard(IPoint pos);
		void DrawEdit();
		bool AcceptMessage(const Message &message);

		bool IsEnergySourceSquare(const IPoint &address) const;
		void EnergySource_Start();
		void EnergySource_Get(std::vector<IPoint> &vec);
	};

	extern SquareNewInfo square_new_info;
}//Gadgets

#endif //_SQUARE_NEW_INFO_
