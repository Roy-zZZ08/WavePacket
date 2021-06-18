#pragma once

#include <Eigen/Dense>
#include "maths.h"

using namespace Eigen;
using namespace maths;

class PacketVertex {
public:
	PacketVertex(const PacketVertex&) = default;
	PacketVertex() = default;

	bool bounced = false;	// indicates if this vertex bounced in this timestep

	Vector2f pos;			// 2D position
	Vector2f pOld;			// position in last timestep (needed to handle bouncing)
	Vector2f dir;			// current movement direction
	Vector2f dOld;			// direction in last timestep (needed to handle bouncing)
	float speed = 0.0f;		// speed of the particle
	float sOld = 0.0f;		// speed in last timestep (needed to handle bouncing)

	void setOld(const PacketVertex& v) {
		pOld = v.pos;
		dOld = v.dir;
		sOld = v.speed;
	}

	void setNew(const PacketVertex& v) {
		pos = v.pos;
		dir = v.dir;
		speed = v.speed;
	}

	PacketVertex getOld() const {
		PacketVertex v;
		v.pos = pOld;
		v.dir = dOld;
		v.speed = sOld;
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
		middle.dir = (0.5f * (a.pos + b.pos) - 0.5f * (a.pOld + b.pOld));
		return middle.normalized();
	}

	bool going_out() const {
		const auto& v = *this;
		const auto dir = v.pos - v.pOld;
		return ((v.pos.x() < -0.5f * SCENE_EXTENT) && (dir.x() < 0.0))
			|| ((v.pos.x() > +0.5f * SCENE_EXTENT) && (dir.x() > 0.0))
			|| ((v.pos.y() < -0.5f * SCENE_EXTENT) && (dir.y() < 0.0))
			|| ((v.pos.y() > +0.5f * SCENE_EXTENT) && (dir.y() > 0.0));
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
