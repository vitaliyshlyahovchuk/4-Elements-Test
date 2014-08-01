#ifndef CARD_PARALAX_CLOUD_H
#define CARD_PARALAX_CLOUD_H
#include "SpriteCardItem.h"

namespace Card
{
	/**************************/
	// ParalaxCloud
	/**************************/
	class ParalaxCloud
		:public MapItem
	{
		Render::Texture *_cloudTexture;
		float _time, _timeScale;
		float _timeOffset;
		bool _right;
		float _alpha;
	public:
		static float SCROLL_POS;
	public:
		ParalaxCloud();
		virtual void init(Container* parent, rapidxml::xml_node<>* info);
		virtual void Draw(const FPoint &pos);
		virtual void Update(float dt);
		virtual bool needDraw(const IRect &draw_rect) const;
		virtual bool needUpdate(const IRect &update_rect) const;
	};

	/**************************/
	// ParalaxItemDynamic
	/**************************/

	class ParalaxItemDynamic
		:public SpriteCardItem
	{
	public:
		ParalaxItemDynamic();
		~ParalaxItemDynamic();
		virtual void init(Container* parent, rapidxml::xml_node<>* info);
		virtual void Draw(const FPoint &pos);
		virtual void Update(float dt);
		virtual bool needDraw(const IRect &draw_rect) const;
		virtual bool needUpdate(const IRect &update_rect) const;
		virtual bool isDrawUp() const;
	private:
		Render::Text* text;
		float shift_text_y;
	};

}//namespace Card

#endif //CARD_PARALAX_CLOUD_H