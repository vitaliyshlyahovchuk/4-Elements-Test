#include "stdafx.h"
#include "BaseEditorMaker.h"

namespace Gadgets
{
	BaseEditorMaker::BaseEditorMaker()
		: _field(NULL)
	{
	}

	void BaseEditorMaker::Init(GameField *field, const bool &with_editor)
	{
		_field = field;
	}

}//namespace Gadgets

