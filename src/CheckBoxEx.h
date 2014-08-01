#ifndef GUI_CHECKBOXEX_ONCE
#define GUI_CHECKBOXEX_ONCE

namespace GUI
{

class CheckBoxEx
	: public Widget
{
	IRect _rect;
	IPoint _offsetCaption;
	TextAlign _hor_align;
	std::string _caption;
	int _inner_inflate;
	bool _send_message;
public:
	CheckBoxEx(const std::string& name, rapidxml::xml_node<>* elem);

	void Update(float dt);

	virtual void Draw();

	virtual void AcceptMessage(const Message& message);

	virtual Message QueryState(const Message& message) const;

	bool MouseDown(const IPoint &mouse_pos);

protected:

	bool _checked;
	int _checkSquareSize;
	
	std::string _font;
    Color _fontColor;
	
};

}

#endif
