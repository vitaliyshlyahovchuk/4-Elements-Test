#pragma once

#include "MapItem.h"

class UserAvatar;
struct FriendInfo;
// todo: разделить на два файла: CardUserAvatar + CardAvatarFriends
namespace Card 
{
	class UpdateLevelItem;
	struct Image {
		FPoint position;
		Render::Texture* texture;

		virtual void Draw() {
			texture->Draw(position);
		}
		virtual void Draw(const FPoint &p) {
			texture->Draw(position + p);
		}
	};
	
	typedef std::list<Image> ListImage;
	typedef ListImage::iterator ListImageIter;

	// иконка, отображающая друга в карте
	class FriendAvatar;
	// объект, состоящий из иконок друзей, прикрепленных к уровню (или к шлюзу)
	class CardAvatarFriends: public MapItem {
	public:
		CardAvatarFriends();
		virtual void Draw(const FPoint& shift);
		virtual void Update(float dt);
		virtual bool isDrawUp() const;
		virtual void MouseDown(const FPoint& mouse_position, bool &capture);
		virtual void MouseMove(const FPoint& mouse_position, bool &capture);
		virtual void MouseUp(const FPoint& mouse_position, bool& capture);
		virtual bool needDraw(const IRect &draw_rect) const;
		void attachToMarker(UpdateLevelItem* marker);
		void AddFriend(FriendInfo* info);
		void ClearFriends();
	private:
		// иконки друзей
		FriendAvatar* icons[3];
		// сколько всего иконок
		int iconsAmount;
		// когда иконок больше одной, иконки сжаты к друг другу, чтобы занимало места
		// при клике, иконки разъежаются, чтобы можно было увидеть всех друзей (максимум три)
		bool openAvatars;
		// таймер открытия/закрытия иконок
		float openAvatarTimer;
		// время откртыия/закрытия иконок
		float openAvatarTime;
		// cмещения иконок при открытии/закрытии
		FPoint openOffset[3];
		// слайны смещения иконок при открытии/закрытии
		SplinePath<FPoint> openOffsetPath[3];
		// состяние открыты/закрыты
		bool stateIn;

		bool pressed;
		FPoint mouse_pressed;

		void correctIconsPosition();
		// смещение аватарок относительно маркера уровня/шлюза
		static FPoint marker_offset;
	};
	
	// иконка игрока (большая рамка)
	class CardUserAvatar: public MapItem
	{
	public:
		CardUserAvatar(UserAvatar *avatar);
		void attachToMarker(UpdateLevelItem* marker);
		void goToMarker(UpdateLevelItem* marker);
		void Draw(const FPoint& shift);
		void Update(float dt);
		virtual bool isDrawUp() const;
	private:
		// аватар игрока
		UserAvatar* avatar;
		// внутренне смещение avatar-а относительно position
		FPoint avatar_offset;
		// рамка аватарки
		Render::Texture* frame;
		// тексты You/Вы
		Render::Text* titleBossText;
		// внутреннее смещение текста You/Вы относительно position
		FPoint text_offset;
		// смещение всего аватара относительно маркера уровня/шлюза
		FPoint marker_offset;

		UpdateLevelItem* currentMarker;
		SplinePath<FPoint> path;
		float moveTime;
		float moveTimer;
		bool move;
	};
} // namespace Card