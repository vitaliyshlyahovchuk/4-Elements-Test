#include "stdafx.h"
#include "FieldStyles.h"
#include "Match3Gadgets.h"

namespace FieldStyles
{
	Style* current = NULL;
	Styles styles;

	Style::Style()
		: border("LevelBorder")
	{
		fon_static_texture = 0;
	}

	void Style::Upload()
	{
		fonDrawer.LoadLevel();
	}

	void Style::Release()
	{
		 fonDrawer.ClearLevel();
	}

	void Load(rapidxml::xml_node<>* xml_elem)
	{
		std::string prev_name = "";
		if(xml_elem)
		{
			if(FieldStyles::current)
			{
				prev_name = FieldStyles::current->name;
			}
			// Спиcок cтилей...

			FieldStyles::styles.clear();
			rapidxml::xml_node<>* elem = xml_elem->first_node("Style");
			while(elem)
			{
				std::string name = Xml::GetStringAttribute(elem, "name");
				Style& style = FieldStyles::styles[name];
				style.name = name;
				elem = elem->next_sibling();
			}
		}


		Styles::iterator i = FieldStyles::styles.begin();
		Styles::iterator e = FieldStyles::styles.end();

		for ( ; i != e; ++i)
		{
			Style& style = i -> second;
			// Надоело править в разных меcтах! Лежат в cвоих папках!
			std::string  styleFileName = "textures/Fields/Styles/" + style.name + "/Style.xml";

			Xml::RapidXmlDocument doc_desc(styleFileName);
			rapidxml::xml_node<>* xml_root = doc_desc.first_node();

			rapidxml::xml_node<>* texElem = xml_root->first_node("Textures")->first_node("Texture");
			while (texElem)
			{
				std::string  id = Xml::GetStringAttribute(texElem, "id");
				if (id == "border")
				{
					style.border = Xml::GetStringAttributeOrDef(texElem, "name", "LevelBorder");
				} else if (id == "bg_empty") 
				{
					style.bg_empty = Xml::GetStringAttribute(texElem, "name");
					style.bgEmptyColor = Color(texElem); 
				} else if (id == "bg_bottom") 
				{
					style.bg_bottom = Xml::GetStringAttribute(texElem, "name");
					style.bgBottomColor = Color(texElem);
				} else if (id == "bg_middle")
				{
					style.bg_middle = Xml::GetStringAttribute(texElem, "name");
					style.bgMiddleColor = Color(texElem);
				} else if (id == "bg_top") 
				{
					style.bg_top = Xml::GetStringAttribute(texElem, "name");
					style.bgTopColor = Color(texElem);
				}
				//(Калинин)
				//Цвет подложки пока тоже естественный, из текстуры 
				style.bgEmptyColor = Color::WHITE;
				//ToDo Костыль на "пока не определились"!!! Поменял местами с bg_bottom
				std::swap(style.bgTopColor, style.bgBottomColor);
				//ToDo Костыль !!!  Поскольку решено использвоать преимущественно 1 слой земли. То он будет выводиться без корректировки цветом! 
				style.bgBottomColor = Color::WHITE;

				texElem = texElem->next_sibling();
			}

			rapidxml::xml_node<>* elemParallax = xml_root -> first_node("Parallax");

			if (elemParallax)
			{
				style.parallax.isUsing = true;

				rapidxml::xml_node<>* elem = elemParallax->first_node("Base");
				ParallaxScene& base = style.parallax.parallaxBackground.base;
				base.texture = Core::resourceManager.Get<Render::Texture>(Xml::GetStringAttribute(elem, "texture"));
				base.width = Xml::GetIntAttribute(elem, "width");
				base.height =Xml::GetIntAttribute(elem, "height");
				base.speed = Xml::GetFloatAttribute(elem, "speed");
				rapidxml::xml_node<>* elemBackgrounds = elemParallax->first_node("Backgrounds");
				
				style.parallax.parallaxBackground.backgrounds.clear();

				if(elemBackgrounds){
					elem = elemBackgrounds->first_node("Background");

					while(elem){
						ParallaxScene background;

						background.texture = Core::resourceManager.Get<Render::Texture>(Xml::GetStringAttribute(elem, "texture"));
						background.width = Xml::GetIntAttribute(elem, "width");
						background.height = Xml::GetIntAttribute(elem, "height");
						background.speed = Xml::GetFloatAttribute(elem, "speed");
						background.scale = Xml::GetFloatAttribute(elem, "scale");

						style.parallax.parallaxBackground.backgrounds.push_back(background);

						elem = elem->next_sibling();
					}
				}
			}else{
				style.parallax.isUsing = false;
			}
			style.fon_static_texture = Core::resourceManager.Get<Render::Texture>(style.name + "_Fon");
			style.fonDrawer.Init(xml_root->first_node("Background"));
		}
		if(!prev_name.empty())
		{
			if(FieldStyles::styles.find(prev_name) != FieldStyles::styles.end())
			{
				FieldStyles::current =  &FieldStyles::styles[prev_name];
			}else{
				MyAssert(false);
				FieldStyles::current =  &FieldStyles::styles.begin()->second;	
			}
		}
	}

	void SetStyle(const std::string &name_)
	{
		std::string name = name_;
		if(FieldStyles::styles.find(name) == FieldStyles::styles.end())
		{
			name = FieldStyles::current ? FieldStyles::current->name : FieldStyles::styles.begin()->first;
		}	
		if(FieldStyles::current)
		{
			FieldStyles::current->Release();
		}
		if(!name.empty())
		{
			FieldStyles::current = &FieldStyles::styles[name];
		}
	}

	void LoadLevel(rapidxml::xml_node<> *xml_elem)
	{
		std::string style_id("");
		if(Gadgets::levelSettings.findName("Style")){
			style_id = Gadgets::levelSettings.getString("Style");
		}
		
		if(style_id.empty())
		{
			Styles::iterator iter = styles.begin();
			style_id = iter->first;
		}
		SetStyle(style_id);
	}

	void FillCombobox()
	{
		Styles::iterator it = styles.begin(), it_end = styles.end();
		GUI::Widget *widget = Core::guiManager.getLayer("ECombobox")->getWidget("ECombobox"); 
		for(; it != it_end; it++)
		{
			widget->AcceptMessage(Message("AddItem", it->first));
		}
	}
	
}//FieldStyles