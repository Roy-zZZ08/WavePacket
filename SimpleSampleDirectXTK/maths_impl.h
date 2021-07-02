#pragma once

#include "Packets.h"

#define MIN_WATER_DEPTH 0.1f
#define MAX_WATER_DEPTH 5.0f

using namespace maths;

inline constexpr float maths::rational_tanh(float x) noexcept //估算tan
{
	if (x < -3.0f)
		return -1.0f;
	else if (x > 3.0f)
		return 1.0f;
	else
		return x * (27.0f + x * x) / (27.0f + 9.0f * x * x);
}

WaveParameters maths::calc_wp(float waterDepth, float w_0, float k) noexcept
{
	float dk = 1.0f;
	int it = 0;
	while ((dk > 1.0e-04) && (it < 6))
	{
		const auto kOld = k;
		k = w_0 / sqrt((GRAVITY / k + k * SIGMA / DENSITY) * rational_tanh(k * waterDepth));		// 使用相速度计算波数 this includes surface tension / capillary waves
		dk = abs(k - kOld);
		it++; //控制步长间隔收敛迭代
	}
	const float t = rational_tanh(k * waterDepth);
	constexpr float c = SIGMA / DENSITY;
	const auto speed = ((c * k * k + GRAVITY) * (t + waterDepth * k * (1.0f - t * t)) + 2.0f * c * k * k * t)
		/ (2.0f * calc_w0(waterDepth, k));   // this is group speed as dw/dk
	return { k, speed };
}

inline constexpr float maths::calc_phase_speed(float w_0, float kIn)
{
	return w_0 / kIn;
}

// area = surface area of the wave packet, E = Energy, k = wavenumber
// computing the amplitude from energy flux for a wave packet
float maths::calc_amp(float area, float E, float k) noexcept
{
	return sqrt(abs(E) / (abs(area) * 0.5f * (DENSITY * GRAVITY + SIGMA * k * k)));
}

float maths::calc_E(float amp, float area, float k) noexcept {
	return amp * amp * (area * 0.5f * (DENSITY * GRAVITY + SIGMA * k * k));
}

template<typename T> inline T maths::interp(const T& a, const T& b, float t) {
	return a * (1.0f - t) + b * t;
}

//返回每个波数所对应的断裂的陡度阈值
inline constexpr float maths::calc_max_amp(float k) {
	return MAX_SPEEDNESS * 2.0f * float(M_PI) / k;
}

inline Vector2f maths::normalize(const Vector2f& v, Vector2f fallback) {
	if (abs(v.x()) + abs(v.y()) > 0.0f) return v.normalized();
	else return fallback;
}

//两条射线相交的距离

float maths::intersection_dist(const Vector2f pos1, const Vector2f dir1, const Vector2f pos2, const Vector2f dir2) {
	ParametrizedLine<float, 2> line1(pos1, dir1); //点和方向定义的一条射线
	Hyperplane<float, 2> line2 = Hyperplane<float, 2>::Through(pos2, pos2 + dir2); //维数小于当前空间的超平面
	float intPointDist = line1.intersectionParameter(line2);
	if (abs(intPointDist) > 10000.0f)
		intPointDist = 10000.0f;
	return intPointDist;
}

inline float maths::calc_w0(float wd, float k) noexcept { //计算角频率
	return sqrt((GRAVITY + k * k * SIGMA / DENSITY) * k * rational_tanh(k * wd));
}

template<typename T> inline T maths::avg(const T& a, const T& b) {
	return interp<T>(a, b, 0.5f);
}

//按比例分配
template<typename T> inline void maths::distribute(T& a, T& b, float t) noexcept {
	const auto sum = a + b;
	a = sum * t;
	b = sum - a;
}

template<typename T> inline void maths::distribute(T& a, T& b, float ta, float tb) noexcept {
	distribute(a, b, ta / (ta + tb));
}

template<typename T> inline void maths::distribute(T& a, T& b, const T& sum, float ta, float tb) noexcept {
	const auto t = ta / (ta + tb);
	a = sum * t;
	b = sum - a;
}

//计算世界坐标系下p点的水深

inline float maths::calc_water_depth(const Packets& packets, const Vector2f& p) {
	const auto v = 1.0f - packets.ground.at_world(p);
	return (MIN_WATER_DEPTH + (MAX_WATER_DEPTH - MIN_WATER_DEPTH) * v * v * v * v);
}

inline void maths::calc_refraction(const Packets& packets, const WavePacket& packet, PacketVertex& out) {
	const auto w0 = packet.w0;
	const auto kIn = packet.k;

	out.dir = out.lastDir;

	const auto speed1 = calc_wp(calc_water_depth(packets, out.lastPos), w0, kIn).speed;
	out.speed = speed1;

	const auto nDir = packets.groundNormal.at_world(out.lastPos);
	if (abs(nDir.x()) + abs(nDir.y()) > 0.1f) {

		// compute new direction and speed based on snells law (depending on water depth)
		const auto vDir1 = out.lastDir;
		const auto pNext = out.lastPos + packets.m_elapsedTime * speed1 * vDir1;
		const auto speed2 = calc_wp(calc_water_depth(packets, pNext), w0, kIn).speed;

		// suppose nDir forms an obtuse angle with vDir1, so make cos1 positive
		// if not so, negate cos2 so that both cos are negative
		const auto cos1 = nDir.dot(-vDir1);
		// sq = square
		const auto sin1sq = 1.0f - cos1 * cos1;
		// snell's law
		const auto sin2sq = (speed2 * speed2) / (speed1 * speed1) * sin1sq;
		const auto cos2 = sqrt(max(0.0f, 1.0f - sin2sq)) * (cos1 < 0.0f ? -1.0f : 1.0f);

		// the sine values are scaled by (speed1 / sin1) to minimize error
		const auto sin1s = speed1;
		const auto sin2s = speed2;
		// a3 = a1 - a2
		const auto sin3s = (sin1s * cos2 - cos1 * sin2s);
		// compose vDir2 using vDir1 and nDir, using law of sines
		const auto vDir2 = (sin2s * vDir1 - sin3s * (nDir)) / speed1;

		if (vDir2.norm() > 0.000001f) out.dir = vDir2.normalized();
	}

	out.pos = out.lastPos + packets.m_elapsedTime * out.speed * out.dir;  // advect wave vertex position
}

inline void maths::calc_reflection(const Packets& packets, PacketVertex& out) {
	// step back until we are outside the boundary, using binary search
	auto pNew = out.lastPos;
	Vector2f pDelta = out.dir * packets.m_elapsedTime * out.speed;
	for (int j = 0; j < 16; j++)
	{
		const auto pNext = pNew + pDelta;
		if (packets.boundDist.at_world(pNext) > 0.0f)
			pNew = pNext;
		pDelta *= 0.5f;
	}
	const auto travelled = (pNew - out.lastPos).norm();

	// compute the traveling direction after the bounce
	const auto nDir = packets.borderNormal.at_world(out.pos);
	const auto vDir = out.dir;
	// nDir and vDir obtuse angle
	const auto cos = nDir.dot(-vDir);
	out.dir = (nDir * cos * 2.0f + vDir).normalized();
	out.pos = pNew + (packets.m_elapsedTime * out.speed - travelled) * out.dir;
}
