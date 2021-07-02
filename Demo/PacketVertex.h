#pragma once

#include <Eigen/Dense>
#include "maths.h"

using namespace Eigen;
using namespace maths;

#define SCENE_SIZE 50.0f

class PacketVertex {
public:
	PacketVertex() = default;
	PacketVertex(const PacketVertex&) = default;

	bool bounced = false;

	Vector2f pos;
	Vector2f lastPos;
	Vector2f dir;
	Vector2f lastDir;
	float speed = 0.0f;
	float lastSpeed = 0.0f;

	void setOld(const PacketVertex& v) {
		lastPos = v.pos;
		lastDir = v.dir;
		lastSpeed = v.speed;
	}

	void setNew(const PacketVertex& v) {
		pos = v.pos;
		dir = v.dir;
		speed = v.speed;
	}

	PacketVertex getOld() const {
		PacketVertex v;
		v.pos = lastPos;
		v.dir = lastDir;
		v.speed = lastSpeed;
		return v;
	}

	PacketVertex normalized() const {
		PacketVertex v = *this;
		v.dir = v.dir.normalized();
		return v;
	}

	static PacketVertex middle1(const PacketVertex& a, const PacketVertex& b) {
		return maths::interp(a.getOld(), b.getOld(), 0.5f).normalized();
	}

	static PacketVertex middle2(const PacketVertex& a, const PacketVertex& b) {
		auto middle = maths::interp(a.getOld(), b.getOld(), 0.5f);
		middle.dir = (0.5f * (a.pos + b.pos) - 0.5f * (a.lastPos + b.lastPos));
		return middle.normalized();
	}

	bool going_out() const {
		const auto& v = *this;
		const auto dir = v.pos - v.lastPos;
		return ((v.pos.x() < -0.5f * SCENE_SIZE) && (dir.x() < 0.0))
			|| ((v.pos.x() > +0.5f * SCENE_SIZE) && (dir.x() > 0.0))
			|| ((v.pos.y() < -0.5f * SCENE_SIZE) && (dir.y() < 0.0))
			|| ((v.pos.y() > +0.5f * SCENE_SIZE) && (dir.y() > 0.0));
	}

	inline float dist_to(const PacketVertex& b) {
		return (pos - b.pos).norm();
	}
};

inline PacketVertex make_vertex(const PacketVertex& old) {
	PacketVertex p;
	p.setOld(old);
	return p;
}

template<> inline PacketVertex maths::interp<PacketVertex>(const PacketVertex& a, const PacketVertex& b, float t) {
	PacketVertex v;
	v.pos = interp(a.pos, b.pos, t);
	v.dir = interp(a.dir, b.dir, t);
	v.speed = interp(a.speed, b.speed, t);
	return v;
}
