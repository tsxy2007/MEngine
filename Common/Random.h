#pragma once
#include <random>
#include <time.h>
#undef max
#undef min
typedef unsigned int UINT;
class Random
{
private:
	std::mt19937 eng;
	std::uniform_int_distribution<UINT> dist;// (eng.min(), eng.max());
public:
	inline Random() : eng(time(nullptr)), dist(eng.min(), eng.max()) {}
	inline double GetNormFloat()
	{
		return dist(eng) / (double)(eng.max());
	}
	inline UINT GetInt()
	{
		return dist(eng);
	}

	inline double GetRangedFloat(double min, double max)
	{
		double range = max - min;
		return dist(eng) / (((double)eng.max()) / range) + min;
	}
};