#ifndef _UNDER_GROUND_PRISES_H_
#define _UNDER_GROUND_PRISES_H_
#include "BaseEditorMaker.h"
#include <map>

class GameField;

namespace Gadgets
{
	class UgPriseInfo
	{
	protected:
		bool _activated;
		float _time_activate, _time_activate_scale;
		Render::Texture *_texture;
		std::string _type;
		FPoint _pos;
	public:
		IPoint _size;
		Game::FieldAddress _address;

		UgPriseInfo(const std::string &type, const IPoint &size = IPoint(2, 2));
		virtual ~UgPriseInfo() {}

		void SetAddress(const Game::FieldAddress &address);

		virtual void Draw() = 0;
		virtual bool Update(float dt);
		virtual void LoadLevel(rapidxml::xml_node<> *xml_element);
		virtual void SaveLevel(Xml::TiXmlElement *xml_element);

		//bool CheckStand(const Game::FieldAddress &address)
		//{
		//	if(!address.IsValid())
		//	{
		//		return false;
		//	}
		//	for(size_t x = 0; x < _size.x; x++)
		//	{
		//		for(size_t y = 0; y < _size.y; y++)
		//		{
		//			Game::FieldAddress a = address + Game::FieldAddress(x, y);
		//			if(!Game::isVisible(a))
		//			{
		//				return false; //Нельзя сюда ни чего поставить
		//			}
		//		}
		//	}
		//	return true;
		//}
		bool Contains(const Game::FieldAddress &a) const
		{
			return	_address.GetCol() <= a.GetCol() &&
					a.GetCol() < _address.GetCol() + _size.x && 

					_address.GetRow() <= a.GetRow() &&
					a.GetRow() < _address.GetRow() + _size.y;
		}

		IRect GetRect() const
		{
			return IRect(_address.GetCol(), _address.GetRow(), _size.x, _size.y);
		}

		typedef boost::shared_ptr<UgPriseInfo> HardPtr;
	};

	struct UgPriseBomb
		:public UgPriseInfo
	{

	public:
		UgPriseBomb();
		bool Update(float dt);
		void Draw();
	};

	struct UgPriseSun
		:public UgPriseInfo
	{

	public:
		UgPriseSun();
		bool Update(float dt);
		void Draw();
	};


	class UgPriseMaker
		: public BaseEditorMaker
	{
	private:
		//int _under_mouse_element_id;
		UgPriseInfo::HardPtr current_element;
		Game::FieldAddress _offset_down;
	public:
		friend class UgPriseInfo;
		//std::map<Game::FieldAddress, UgPriseInfo> _info;
		std::vector<UgPriseInfo::HardPtr> _elements;
		std::vector<UgPriseInfo::HardPtr> _clipboard;
		std::string _current_type;
	public:
		void Init(GameField *field, const bool &with_editor);
		void LoadLevel(rapidxml::xml_node<> *root);
		void SaveLevel(Xml::TiXmlElement *root);
		void DrawUnderWalls();
		void Apply();
		void Clear();
		void Release();
		bool Editor_LeftMouseDown(const IPoint &mouse_pos, Game::Square *sq);
		bool Editor_RightMouseDown(const IPoint &mouse_pos, Game::Square *sq);
		bool Editor_MouseUp(const IPoint &mouse_pos, Game::Square *sq);
		bool Editor_MouseMove(const IPoint &mouse_pos, Game::Square *sq);
		bool Editor_MouseWheel(int delta, Game::Square *sq);
		void Editor_CutToClipboard(IRect part);
		bool Editor_PasteFromClipboard(IPoint pos);
		void DrawEdit();
		bool AcceptMessage(const Message &message);

		void Update(float dt);
	private:
		UgPriseInfo::HardPtr FindElement(const Game::FieldAddress &need_address);
		static UgPriseInfo::HardPtr CreateElement(const std::string &type);
		bool RemoveElement(UgPriseInfo::HardPtr item);
		bool CheckCrosses(UgPriseInfo::HardPtr cheked_element, const Game::FieldAddress &checked_a);
	};

}//Gadgets

#endif //_UNDER_GROUND_PRISES_H_