#ifndef _FIELD_STYLES_
#define _FIELD_STYLES_

#include "Match3Background.h"

namespace FieldStyles
{
	struct ParallaxScene
	{
		Render::Texture *texture;
		int width;
		int height;
		float speed;
		float scale;
	};

	typedef struct{
		ParallaxScene base;
		std::vector<ParallaxScene> backgrounds;
	} ParallaxBackground;

	struct Style
	{	
		std::string name;	// Имя cтиля в формате "ElementCard"

		std::string bg_top;		// имя текcтуры верхнего уровня заполнения клетки
		std::string bg_middle;		// cреднего
		std::string bg_bottom;		// нижнего
		std::string bg_empty;		// пуcтая клеткаs
		std::string border;		//id текстуры границы по умолчанию LevelBorder

		Color bgTopColor;
		Color bgMiddleColor;
		Color bgBottomColor;
		Color bgEmptyColor;

		Match3Background::FonDrawer fonDrawer;

		Render::Texture *fon_static_texture;
		struct
		{
			bool isUsing;
			ParallaxBackground parallaxBackground;
		} parallax;
		
		Style();
		void Upload();
		void Release();
	};

	typedef std::map <std::string, Style> Styles;

	void Load(rapidxml::xml_node<>* xml_elem);
	void SetStyle(const std::string &name);
	void LoadLevel(rapidxml::xml_node<> *xml_elem);
	void FillCombobox();

	extern Styles styles;
	extern Style* current;

}

#endif//_FIELD_STYLES_