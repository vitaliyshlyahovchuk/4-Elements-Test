#ifndef ONCE_COMMON_CONTAINERS
#define ONCE_COMMON_CONTAINERS

template <class T>
class SplinePath;
namespace Render {
	class Texture;
};

typedef std::set <int> IntSet;
typedef std::set <std::string> StringSet;

typedef std::vector <int> IntVector;
typedef std::vector <float> FloatVector;
typedef std::vector <IPoint> IPointVector;
typedef std::vector <FPoint> FPointVector;
typedef std::vector <std::string> StringVector;
typedef std::vector <Render::Texture*> TexVector;

typedef SplinePath <float> SplinePathFloat;
typedef SplinePath <FPoint> SplinePathFPoint;

#endif
