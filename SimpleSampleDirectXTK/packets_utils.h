#pragma once

#include "Packets.h"

inline bool Packets::inBounds(int x, int y) const {
	// four directions
	return
		(bound.at(x + 0, y + 1) <= 0.5f) ||
		(bound.at(x + 1, y + 0) <= 0.5f) ||
		(bound.at(x + 0, y - 1) <= 0.5f) ||
		(bound.at(x - 1, y + 0) <= 0.5f);
}

Vector2f Packets::projectToBoundary(Vector2f pos) const {
	for (int i = 0; i < 16; i++)
		pos += -0.5f * boundDist.at_world(pos) * borderNormal.at_world(pos);
	return pos;
}


WaveParameters Packets::recalcWavenumber(WavePacket& packet, const Vector2f& pos, bool updateL, bool updateH) const {
	const auto wd = calc_water_depth(*this, pos);
	if (updateL)
		packet.k_L = calc_wp(wd, packet.w0_L, packet.k_L).k;
	if (updateH)
		packet.k_H = calc_wp(wd, packet.w0_H, packet.k_H).k;
	const auto wp = calc_wp(wd, packet.w0, avg(packet.k_L, packet.k_H));
	packet.k = wp.k;
	packet.d_L = 0.0;
	packet.d_H = 0.0; // reset the internally tracked error
	packet.update_envelope();
	return wp;
}

PacketVertex Packets::AdvectPacketVertex(const WavePacket& packet, const PacketVertex& v) {
	PacketVertex out;
	out.setOld(v);
	calc_refraction(*this, packet, out);

	// if we ran into a boundary -> step back and bounce off
	if (boundDist.at_world(out.pos) < 0)
	{
		const float a = borderNormal.at_world(out.pos).dot(out.dir);
		if (a <= reflect_threshold())  // a wave reflects if it travels with >4.5 degrees towards a surface. Otherwise, it gets diffracted
		{
			out.bounced = true;
			calc_reflection(*this, out);
		}
	}

	// if we got trapped in a boundary (again), just project onto the nearest surface (this approximates multiple bounces)
	if (boundDist.at_world(out.pos) < 0.0)
		out.pos = projectToBoundary(out.pos);

	return out;
}

// a is not bouncing and b is, find bouncing point
PacketVertex Packets::FindBouncePoint(const WavePacket& packet, const PacketVertex& a, const PacketVertex& b) {
	float s = 0.0f;
	float sD = 0.5f;

	PacketVertex p;
	p.setOld(a.normalized());

	for (int j = 0; j < 16; j++)
	{
		const auto v = interp(a, b, s + sD).normalized();
		const auto vNew = AdvectPacketVertex(packet, v);

		if (!vNew.bounced)
		{
			s += sD;
			p = vNew;
		}
		sD *= 0.5f;
	}
	return p;
}

void Packets::UpdateWavenumber(WavePacket& packet, float wd) const {
	packet.w0 = avg(packet.w0_L, packet.w0_H);

	// distribute the error according to current speed difference
	const auto wp_L = calc_wp(wd, packet.w0_L, packet.k_L);
	packet.k_L = wp_L.k;
	const auto wp_H = calc_wp(wd, packet.w0_H, packet.k_H);
	packet.k_H = wp_H.k;
	const auto wp_M = calc_wp(wd, packet.w0, avg(packet.k_L, packet.k_H));
	packet.k = wp_M.k;

	float dSL = abs(abs(wp_L.speed) - abs(wp_M.speed));
	float dSH = abs(abs(wp_H.speed) - abs(wp_M.speed));
	distribute(packet.d_L, packet.d_H, packet.d_L, dSL, dSH);
	packet.update_envelope();
}

void WavePacket::blend_in() {
	const auto& packets = static_cast<Packets&>((*this).packets);
	const float amp = min(
		calc_max_amp(k),
		calc_amp(envelope * (vertices[0].pos - vertices[1].pos).norm(), E, k)
	);
	ampOld = 0.0f;
	dAmp = amp
		* avg(vertices[0].speed, vertices[1].speed)
		* packets.m_elapsedTime
		/ (PACKET_BLEND_TRAVEL_FACTOR * envelope);
}

void WavePacket::update_envelope() {
	envelope = clamp(PACKET_ENVELOPE_SIZE_FACTOR * 2.0f * (float)M_PI / k, PACKET_ENVELOPE_MINSIZE, PACKET_ENVELOPE_MAXSIZE); // adjust envelope size to represented wavelength
}

void GhostPacket::set_from_wave(const Packets& packets, const WavePacket& p, const PacketVertex& v) {
	auto& ghost = *this;

	ghost.pos = v.pos;
	ghost.dir = v.dir;
	ghost.speed = v.speed;

	ghost.envelope = p.envelope;
	ghost.ampOld = p.ampOld;
	ghost.dAmp = ghost.ampOld * ghost.speed * packets.m_elapsedTime / (PACKET_BLEND_TRAVEL_FACTOR * ghost.envelope);
	ghost.k = p.k;
	ghost.phase = p.phOld;
	ghost.dPhase = p.phase - p.phOld;
	ghost.bending = intersection_dist(ghost.pos, ghost.dir, p.vertices[0].pOld, p.vertices[0].dOld);
}