#ifndef _GUI_EDITBOX_EX_H_
#define _GUI_EDITBOX_EX_H_

namespace GUI
{

class EditBoxEx
	: public Widget
{
	IRect _rect;
	bool _canEdit;
	std::string _caption;
	IPoint _caption_offset;
public:
	EditBoxEx(const std::string& name, rapidxml::xml_node<>* elem);

	void Update(float dt);

	virtual void Draw();

	virtual void AcceptMessage(const Message& message);

	virtual Message QueryState(const Message& message) const;

	bool MouseDown(const IPoint &mouse_pos);

protected:
	bool _active;

	std::string _text;
	
	size_t _ibeamPos;

	std::string _font;
	
    Color _fontColor;
    
	int _limit;

	float _timer;
	
	std::set<int> _denied;
};

}

#endif //_GUI_EDITBOX_EX_H_
