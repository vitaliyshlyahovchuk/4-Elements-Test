#ifndef CARD_RESOURCE_MANAGER_H
#define CARD_RESOURCE_MANAGER_H


namespace Card
{
	class CardResourceManager
	{
		struct CardResource 
		{
			Resource *res;
			float low, high, priority;
			bool used;

			CardResource(Resource *r, float l, float h);
		};
		typedef std::vector<CardResource> Resources;
		Resources _resources;
		float _low, _high; // нижняя и верхняя границы видимой на экране области

		static const float LOAD_MARGIN;

		static bool Compare(const CardResource& r1, const CardResource& r2);
	public:
		~CardResourceManager();
		void Register(Resource *res, const float low_border, const float high_border);
		void Release();
		void Update(float low_level, float high_level, float scroll_speed = 0.0f);
		void WaitForLoading();

		bool IsLoadedArea(float low, float high) const; // загружены ли все текстуры, видимые на экране
		bool IsLoadedAll() const;     // загружены ли все запрошенные текстуры
	};

	extern Card::CardResourceManager cardResourceManager;
};


#endif //CARD_RESOURCE_MANAGER_H