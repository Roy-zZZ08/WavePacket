#pragma once

#include <Eigen/Dense>

class Packets;
class WavePacket;
class PacketVertex;

using namespace Eigen;

struct WaveParameters {
	float k;
	float speed;
};

namespace maths {
	constexpr float reflect_threshold() noexcept {
		return 4.5f / 180.0f * M_PI;
	}

	inline constexpr float rational_tanh(float) noexcept;

	WaveParameters calc_wp(float waterDepth, float w_0, float k) noexcept;
	inline float calc_w0(float wd, float k) noexcept;
	inline constexpr float calc_phase_speed(float w_0, float kIn);

	float calc_amp(float area, float E, float k) noexcept;
	inline constexpr float calc_max_amp(float k);
	float calc_E(float amp, float area, float k) noexcept;

	template<typename T> inline T interp(const T& a, const T& b, float t);
	template<> inline PacketVertex interp<PacketVertex>(const PacketVertex& a, const PacketVertex& b, float t);
	template<typename T> inline T avg(const T& a, const T& b);

	inline Vector2f normalize(const Vector2f& v, Vector2f fallback);
	float intersection_dist(const Vector2f pos1, const Vector2f dir1, const Vector2f pos2, const Vector2f dir2);

	template<typename T> inline void distribute(T& a, T& b, float t) noexcept;
	template<typename T> inline void distribute(T& a, T& b, float ta, float tb) noexcept;
	template<typename T> inline void distribute(T& a, T& b, const T& total, float ta, float tb) noexcept;

	inline float calc_water_depth(const Packets& packets, const Vector2f& p);

	inline void calc_refraction(const Packets& packets, const WavePacket& packet, PacketVertex& out);
	inline void calc_reflection(const Packets& packets, PacketVertex& out);
};
