#include <windows.h>
#include <strsafe.h>

#include <algorithm>

#include "Packets.h"
#include "CImg.h"

#define PRINTF(...) { \
	WCHAR wcFileInfo[512]; \
	StringCchPrintf(wcFileInfo, 512, __VA_ARGS__); \
	OutputDebugString(wcFileInfo); \
}
#define SCENE_SIZE 50.0f

#include "maths_impl.h"
#include "packets_utils.h"

using namespace maths;

void Packets::readMap() {
	cimg_library::CImg<unsigned char> img("TestIsland.bmp");
	groundWidth = img.width();
	groundHeight = img.height();
	ground.resize(groundWidth, groundHeight);
	bound.resize(groundWidth, groundHeight);
	for (int y = 0; y < groundHeight; y++)
		for (int x = 0; x < groundWidth; x++) {
			ground.at(x, y) = img.atXYZC(x, y, 0, 2) / 255.0f;
			const auto v = img.atXYZC(x, y, 0, 1) / 255.0f;
			bound.at(x, y) = v > 11.1f / 255.0f ? 1.0f : 0.0f;
		}
}

void Packets::boundaryDistanceTransform() {
	// boundary texture distance transform 
	// init helper distance map (pMap)
	PRINTF(L"Computing boundary distance transform..");

	map<int> pMap(groundWidth, groundHeight);
	map<int> pMap2(groundWidth, groundHeight);
	boundDist.resize(groundWidth, groundHeight);

#pragma omp parallel for
	for (int y = 0; y < groundHeight; y++)
		for (int x = 0; x < groundWidth; x++)
		{
			// if we are at the boundary, intialize the distance function with 0, otherwise with maximum value
			pMap.at(x, y) = inBounds(x, y) ? 0 : groundWidth * groundWidth;
		}

#pragma omp parallel for
	for (int y = 0; y < groundHeight; y++)   // horizontal scan forward
	{
		int lastBoundX = -groundWidth;
		for (int x = 0; x < groundWidth; x++)
		{
			if (pMap.at(x, y) == 0)
				lastBoundX = x;
			pMap.at(x, y) = min(pMap.at(x, y), (x - lastBoundX) * (x - lastBoundX));
		}
	}

#pragma omp parallel for
	for (int y = 0; y < groundHeight; y++)  // horizontal scan backward
	{
		int lastBoundX = 2 * groundWidth;
		for (int x = groundWidth - 1; x >= 0; x--)
		{
			if (pMap.at(x, y) == 0)
				lastBoundX = x;
			pMap.at(x, y) = min(pMap.at(x, y), (lastBoundX - x) * (lastBoundX - x));
		}
	}

#pragma omp parallel for
	for (int x = 0; x < groundWidth; x++)  // vertical scan forward and backward
		for (int y = 0; y < groundHeight; y++)
		{
			int minDist = pMap.at(x, y);
			for (int yd = 1; yd + y <= groundHeight - 1; yd++)
			{
				minDist = min(minDist, yd * yd + pMap.at(x, y));
				if (minDist < yd * yd)
					break;
			}
			for (int yd = -1; yd + y >= 0; yd--)
			{
				minDist = min(minDist, yd * yd + pMap.at(x, y));
				if (minDist < yd * yd)
					break;
			}
			pMap2.at(x, y) = (float)(minDist);
		}

	// boundDist now contains the _squared_ euklidean distance to closest label boundary, so take the sqroot. And sign the distance
#pragma omp parallel for
	for (int y = 0; y < groundHeight; y++)
		for (int x = 0; x < groundWidth; x++)
		{
			auto dist = sqrt(pMap2.at(x, y));
			if (bound.at(x, y) > 0.5f) dist *= -1;		// negative distance INSIDE a boundary regions
			dist *= SCENE_SIZE / groundWidth;
			boundDist.at(x, y) = dist;
		}

	PRINTF(L"done!\n");
}

void Packets::boundaryDerivatives() {
	// derivative (2D normal) of the boundary texture
	PRINTF(L"Computing boundary derivatives..");

	borderNormal.resize(groundWidth, groundHeight);
	map<Vector2f> m_bndDerivH(groundWidth, groundHeight);

#pragma omp parallel for
	for (int y = 0; y < groundHeight; y++)
		for (int x = 0; x < groundWidth; x++)
			m_bndDerivH.at(x, y) = normalize(boundDist.deriv(x, y), Vector2f(0, 0));

	PRINTF(L"done!\n");

	//smooth the derivative to avoid staricase artifacts of the texture
	PRINTF(L"Smoothing boundary derivatives..");

	constexpr float weights[][3] = {
		{ 1, 2, 1 },
		{ 2, 4, 2 },
		{ 1, 2, 1 },
	};

	for (int i = 0; i < 2; i++)  //15
	{
#pragma omp parallel for
		for (int y = 0; y < groundHeight; y++)
			for (int x = 0; x < groundWidth; x++)
			{
				auto dV = Vector2f(0.0f, 0.0f);
				for (int dy = -1; dy <= 1; dy++)
					for (int dx = -1; dx <= 1; dx++)
					{
						const auto w = weights[dy + 1][dx + 1] / 16.0f;
						dV += w * m_bndDerivH.at(x + dx, y + dy);
					}
				borderNormal.at(x, y) = normalize(dV, Vector2f(0, 0));
			}
		
		// copy the result back to derivative map
		std::copy(borderNormal.storage.begin(), borderNormal.storage.end(), m_bndDerivH.storage.begin());
	}

	PRINTF(L"done!\n");
}

void Packets::groundDerivatives() {
	// derivative (2D normal) of the ground texture
	PRINTF(L"Computing ground derivatives..");

	groundNormal.resize(groundWidth, groundHeight);

#pragma omp parallel for
	for (int y = 0; y < groundHeight; y++)
		for (int x = 0; x < groundWidth; x++)
			groundNormal.at(x, y) = normalize(ground.deriv(x, y), Vector2f(0, 0));

	PRINTF(L"done!\n");
}

Packets::Packets(int packetBudget) : 
	m_packetBudget(packetBudget), 
	packets(0, WavePacket(*this)),
	ghosts(0)
{
	readMap();

	boundaryDistanceTransform();
	boundaryDerivatives();
	groundDerivatives();
	
	// init variables
	ExpandWavePacketMemory(0);
	Reset();

	PRINTF(L"Initialized.\n");
}

// initialize all packets as "unused"
void Packets::Reset()
{
	ghosts.reset();
	packets.reset();
	m_time = 0.0f;
}

// increase the available packet memory
// single threaded
void Packets::ExpandWavePacketMemory(int threshold)
{
	if (threshold < packets.size()) return; // don't shrink
	const auto targetNum = threshold + PACKET_BUFFER_DELTA;

	PRINTF(L"xpanding packet memory from %i to %i packets (%i MB).\n", 
		packets.size(),
		targetNum, 
		(targetNum) * (sizeof(WavePacket) + sizeof(GhostPacket) + 4 * sizeof(int)) / (1024 * 1024));

	packets.resize(targetNum);
	ghosts.resize(targetNum);
};

// adds a new packet at given positions, directions and wavenumber interval k_L and k_H
void Packets::CreatePacket(float pos1x, float pos1y, float pos2x, float pos2y, float dir1x, float dir1y, float dir2x, float dir2y, float k_L, float k_H, float E)
{
	// make sure we have enough memory
	ExpandWavePacketMemory(max(packets.nused(), ghosts.nused()));

	auto& first = packets.claim(*this);

	first.vertices[0].lastPos = first.vertices[0].pos = Vector2f(pos1x, pos1y);
	first.vertices[1].lastPos = first.vertices[1].pos = Vector2f(pos2x, pos2y);
	first.vertices[0].lastDir = first.vertices[0].dir = Vector2f(dir1x, dir1y);
	first.vertices[1].lastDir = first.vertices[1].dir = Vector2f(dir2x, dir2y);
	
	first.vertices[0].bounced = false;
	first.vertices[1].bounced = false;
	first.vertices[2].bounced = false;
	first.sliding3 = false;
	first.use3rd = false;

	first.phase = 0.0;
	first.phOld = 0.0;
	first.E = E;

	auto wd = calc_water_depth(*this, first.vertices[0].pos);
	first.k_L = k_L;
	first.k_H = k_H;
	first.k = avg(first.k_L, first.k_H); // set the representative wave as average of interval boundaries
	first.w0 = calc_w0(wd, first.k);	// this takes surface tension into account
	first.w0_L = calc_w0(wd, first.k_L);	// this take surface tension into account
	first.w0_H = calc_w0(wd, first.k_H);	// this take surface tension into account
	first.d_L = 0.0;
	first.d_H = 0.0;
	
	first.vertices[0].lastSpeed = first.vertices[0].speed = calc_wp(calc_water_depth(*this, first.vertices[0].pos), first.w0, first.k).speed;
	first.vertices[1].lastSpeed = first.vertices[1].speed = calc_wp(calc_water_depth(*this, first.vertices[1].pos), first.w0, first.k).speed;
	first.update_envelope();
	first.blend_in();

	// Test for wave number splitting -> if the packet interval crosses the slowest waves, divide so that each part has a monotonic speed function (assumed for travel spread/error calculation)
	if ((first.w0_L > PACKET_SLOWAVE_W0) && (first.w0_H < PACKET_SLOWAVE_W0))
	{
		// copy the entiry wave packet information
		auto& second = packets.claim(first);
		// set new interval boundaries to the slowest wave and update all influenced parameters
		second.k_L = PACKET_SLOWAVE_K;
		second.w0_L = PACKET_SLOWAVE_W0;
		second.w0 = avg(second.w0_L, second.w0_H);
		const auto wp1 = recalcWavenumber(second, first.vertices[0].pos, true, false);
		second.vertices[1].speed = second.vertices[0].speed = wp1.speed;
		second.blend_in();

		// also adjust freq. interval and envelope size of existing wave
		first.k_H = PACKET_SLOWAVE_K;
		first.w0_H = PACKET_SLOWAVE_W0;
		first.w0 = avg(first.w0_L, first.w0_H);
		const auto wp2 = recalcWavenumber(first, first.vertices[0].pos, false, true);
		first.vertices[1].speed = first.vertices[0].speed = wp2.speed;
		first.blend_in();
	}
}


// adds a new circular wave at normalized position x,y with wavelength boundaries lambda_L and lambda_H (in meters)
void Packets::CreateCircularWavefront(float xPos, float yPos, float radius, float lambda_L, float lambda_H, float E)
{
	// adapt initial packet crestlength to impact radius and wavelength ¶¨ÒåPacket·¶Î§
	float dAng = min(24.0f, 360.0f * ((0.5f * lambda_L + 0.5f * lambda_H) * 3.0f) / (2.0f * M_PI * radius));

	for (float i = 0; i < 360.0f; i += dAng)
		CreatePacket(
			xPos + radius * sin(i * M_PI / 180.0f), yPos + radius * cos(i * M_PI / 180.0f),
			xPos + radius * sin((i + dAng) * M_PI / 180.0f), yPos + radius * cos((i + dAng) * M_PI / 180.0f),
			sin(i * M_PI / 180.0f), cos(i * M_PI / 180.0f),
			sin((i + dAng) * M_PI / 180.0), cos((i + dAng) * M_PI / 180.0f),
			2.0f * M_PI / lambda_L, 2.0f * M_PI / lambda_H, E);
}

void Packets::UpdatePacketVertices() {
	// compute the new packet vertex positions, directions and speeds based on snells law
	packets.for_each([&](auto& packet, auto it) {
		packet.vertices[0] = AdvectPacketVertex(packet, packet.vertices[0]);
		packet.vertices[1] = AdvectPacketVertex(packet, packet.vertices[1]);

		if (packet.use3rd)
			packet.vertices[2] = AdvectPacketVertex(packet, packet.vertices[2]);

		// measure new wave k at phase speed at wave packet center, advect all representative waves inside
		packet.phOld = packet.phase;

		const float packetSpeed = 0.5f * (packet.vertices[0].speed + packet.vertices[1].speed);
		packet.phase += m_elapsedTime * (calc_phase_speed(packet.w0, packet.k) - packetSpeed) * packet.k;	// advect phase with difference between group speed and wave speed
	});
}

// how to update bounces?
// packets move forwards, until at lease one vertex of two is bounced at, say, frame #0.
// 
// <<FRAME #0>>
// if both vertices are bounced, we simply let it go on, towards the new direction, no 3rd vertex involved.
// otherwise, a 3rd vertex is inserted between them at the border, that 3rd vertex is called to be in "waiting for bounce" mode.
// then, the packet is hidden and replaced by a ghost.
// 
// <<FRAME #1>>
// vertices move forwards, including the 3rd one.
// if 3rd is not bounced, it enters sliding mode, that happens most of the time
// otherwise, we have two bouncing 

// read: use3rd, vertices, 
// write: set_from_wave, ampOld, dAmp
void Packets::UpdateBounces() {
	// first contact to a boundary -> sent a ghost packet, make packet invisible for now, add 3rd vertex
	ExpandWavePacketMemory(ghosts.nused() + packets.nused());

	packets.for_each([&](auto& packet, auto it) {
		// not using 3rd, at least one of vertices bounced
		if (!packet.use3rd)
		{
			if (packet.vertices[0].bounced || packet.vertices[1].bounced) {
				auto& ghost = ghosts.claim();

				// the new position is wrong after the reflection, so use the old direction instead
				ghost.set_from_wave(*this, packet, PacketVertex::middle1(packet.vertices[0], packet.vertices[1]));

				// hide this packet from display
				packet.ampOld = 0.0;
				packet.dAmp = 0.0;

				// emit all (higher-)frequency waves after a bounce
				if ((PACKET_BOUNCE_FREQSPLIT) && (packet.k_L < PACKET_BOUNCE_FREQSPLIT_K))  // split the frequency range if smallest wave is > 20cm
				{
					packet.k_L = PACKET_BOUNCE_FREQSPLIT_K;
					packet.w0_L = sqrt(GRAVITY / packet.k_L) * packet.k_L;  // initial guess for angular frequency
					packet.w0 = avg(packet.w0_L, packet.w0_H);

					// distribute the error according to current speed difference
					recalcWavenumber(packet, avg(packet.vertices[0].pos, packet.vertices[1].pos));
				}

				//if both vertices bounced, the reflected wave needs to smoothly reappear
				if (packet.vertices[0].bounced == packet.vertices[1].bounced)
				{
					packet.blend_in();
				}
				else	  // only one vertex bounced -> insert 3rd "wait for bounce" vertex and reorder such that 1st is waiting for bounce
				{
					// if the first bounced and the second did not -> exchange the two points, as we assume that the second bounced already and the 3rd will be "ahead" of the first.. 
					if (packet.vertices[0].bounced) std::swap(packet.vertices[0], packet.vertices[1]);

					packet.vertices[2] = FindBouncePoint(packet, packet.vertices[0], packet.vertices[1]);
					packet.vertices[2].bounced = false;

					packet.use3rd = true;
					packet.sliding3 = false;
				}
			}
		}
		else // 3rd vertex already present
		{
			auto& v3 = packet.vertices[2];
			if (!v3.bounced)	// advect 3rd as sliding vertex (from now on it is)
			{
				const auto nDir = borderNormal.at_world(v3.pos); // get sliding direction
				v3.dir = Vector2f(-nDir.y(), nDir.x());
				if (v3.dir.dot(v3.lastDir) < 0) v3.dir *= -1.0f;

				v3.pos = projectToBoundary(v3.pos);

				packet.sliding3 = true;
			}
			else {
				if (packet.sliding3) {// if sliding 3rd point bounced, release it
					packet.use3rd = false;
					packet.blend_in();
				}
				// 3rd point was in "waiting for bounce" state
				else {
					if ((packet.vertices[0].bounced) || (packet.vertices[1].bounced)) {// case: 3rd "has to bounce" and one other point bounced as well -> release 3rd vertex
						PRINTF(L"O");
						packet.use3rd = false;
						// if we released this wave from a boundary (3rd vertex released) -> blend it smoothly in again
						packet.blend_in();
					}
					else { // "has to bounce"-3rd vertex -> find new "has to bounce" point if no other vertex bounced
						PRINTF(L"X");
						packet.vertices[2] = FindBouncePoint(packet, packet.vertices[0], packet.vertices[2]);
						packet.vertices[2].bounced = true;
					}
				}
			}
		}
	});
}

// read: use3rd, vertices, w0*, k*, d*, envelope, E
// write: d*, k*, set_from_wave, E, phase, ampOld, dAmp
void Packets::WavenumberSubdivision() {
	// wavenumber interval subdivision if travel distance between fastest and slowest wave packets differ more than PACKET_SPLIT_DISPERSION x envelope size
	ExpandWavePacketMemory(max(ghosts.nused() + packets.nused(), 2 * packets.nused()));

	packets.for_each([&](auto& packet, auto it) {
		if (!packet.use3rd)
		{
			Vector2f pos = avg(packet.vertices[0].pos, packet.vertices[1].pos);
			float wd = calc_water_depth(*this, pos);
			float dist_Ref = m_elapsedTime * calc_wp(wd, packet.w0, packet.k).speed;

			auto wp1 = calc_wp(wd, packet.w0_L, packet.k_L);
			packet.k_L = wp1.k;
			packet.d_L += fabs(m_elapsedTime * wp1.speed - dist_Ref);  // taking the abs augments any errors caused by waterdepth independent slowest wave assumption..
			auto wp2 = calc_wp(wd, packet.w0_H, packet.k_H);
			packet.k_H = wp2.k;
			packet.d_H += fabs(m_elapsedTime * wp2.speed - dist_Ref);

			if (packet.d_L + packet.d_H > PACKET_SPLIT_DISPERSION * packet.envelope)  // if fastest/slowest waves in this packet diverged too much -> subdivide
			{
				// first create a ghost for the old packet
				auto& ghost = ghosts.claim();
				auto middle = interp(packet.vertices[0].getOld(), packet.vertices[1].getOld(), 0.5f);
				middle.dir = avg(packet.vertices[0].pos, packet.vertices[1].pos) - avg(packet.vertices[0].lastPos, packet.vertices[1].lastPos);
				ghost.set_from_wave(*this, packet, middle.normalized());

				auto k_M = packet.k;
				auto w0_M = packet.w0;

				// create new packet and copy ALL parameters
				auto& second = packets.claim(packet);
				// split the frequency range
				second.k_H = k_M;
				second.w0_H = w0_M;
				UpdateWavenumber(second, wd);

				// set the new upper freq. boundary and representative freq.
				packet.k_L = k_M;
				packet.w0_L = w0_M;
				UpdateWavenumber(packet, wd);

				// distribute the error according to E_MAXSIZE, max(PACKET_ENVELOPE_MINSIZE, PACKET_ENVELOPE_SIZE_FACTOR * 2.0f * M_PI / packet.k));
				// distribute the energy such that both max. wave gradients are equal -> both get the same wave shape
				second.E = 0;
				distribute(
					packet.E, second.E,
					packet.envelope * second.k * second.k
						* (DENSITY * GRAVITY + SIGMA * packet.k * packet.k),
					second.envelope * packet.k * packet.k
						* (DENSITY * GRAVITY + SIGMA * second.k * second.k));

				// smoothly ramp the new waves
				packet.phase = 0;
				packet.blend_in();
				second.phase = 0.0f;
				second.blend_in();
			}
		}
	});
}

// read: use3rd, vertices, envelope
// write: new, vertices, E, ampOld, dAmp, set_from_wave
void Packets::RegularSubdivision() {
	// crest-refinement of packets of regular packet (not at any boundary, i.e. having no 3rd vertex)
	ExpandWavePacketMemory(max(ghosts.nused() + packets.nused(), 2 * packets.nused()));

	packets.for_each([&](auto& packet, auto it) {
		if (!packet.use3rd)
		{
			float sDiff = (packet.vertices[1].pos - packet.vertices[0].pos).norm();
			float aDiff = packet.vertices[0].dir.dot(packet.vertices[1].dir);
			if ((sDiff > packet.envelope) || (aDiff <= PACKET_SPLIT_ANGLE))  // if the two vertices move towards each other, do not subdivide
			{
				auto& ghost = ghosts.claim();
				const auto middle = interp(packet.vertices[0].getOld(), packet.vertices[1].getOld(), 0.5f).normalized();
				ghost.set_from_wave(*this, packet, middle.normalized());

				auto middleNew = make_vertex(middle);
				middleNew.setNew(interp(packet.vertices[0], packet.vertices[1], 0.5f).normalized());

				// create new vertex between existing packet vertices
				auto& second = packets.claim(packet);
				second.vertices[0] = middleNew;
				packet.vertices[1] = middleNew;

				second.E = 0;
				distribute(packet.E, second.E, 0.5f);

				packet.blend_in();
				second.blend_in();
			}
		}
	});
}

void Packets::SplitArm(WavePacket& packet, int idx1) {
	const auto idx2 = !idx1; // { idx1, idx2 } = { 0, 1 }

	const auto sDiff = (packet.vertices[idx1].pos - packet.vertices[2].pos).norm();
	const auto aDiff = packet.vertices[idx1].dir.dot(packet.vertices[2].dir);
	if ((sDiff >= packet.envelope)) // || (aDiff <= PACKET_SPLIT_ANGLE))  // angle criterion is removed here because this would prevent diffraction
	{
		PacketVertex middleNew;
		middleNew.setOld(avg(packet.vertices[idx1].getOld(), packet.vertices[2].getOld()).normalized());
		middleNew.setNew(avg(packet.vertices[idx1], packet.vertices[2]).normalized());

		// first vertex becomes first in new wave packet, third one becomes second
		auto& second = packets.claim(packet);
		second.use3rd = false;
		second.sliding3 = false;
		second.vertices[idx2] = middleNew;
		packet.vertices[idx1] = middleNew;
		second.blend_in();

		// distribute the energy according to length of the two new packets
		second.E = 0;
		distribute(
			second.E, packet.E, 
			second.vertices[0].dist_to(second.vertices[1]),
			packet.vertices[0].dist_to(packet.vertices[2]) +
			packet.vertices[1].dist_to(packet.vertices[2]));
	}
}

// read: usr3rd, sliding3, vertices, E, envelope, k
// write: new, vertices, ampOld, dAmp, (usr3rd, sliding3, bounced), E
void Packets::SlidingSubdivision() {
	// crest-refinement of packets with a sliding 3rd vertex
	ExpandWavePacketMemory(3 * packets.nused());

	packets.for_each([&](auto& packet, auto it) {
		if ((packet.use3rd) && (packet.sliding3))
		{
			SplitArm(packet, 0);
			SplitArm(packet, 1);
		}
	});
}

// read: usr3rd, vertices
// write: toDelete
void Packets::DeleteOutwardsPackets() {
	// delete packets traveling outside the scene
	packets.for_each([&](auto& packet, auto it) {
		packet.toDelete = false;
		if (!packet.use3rd && packet.vertices[0].going_out() && packet.vertices[1].going_out()) packet.toDelete = true;
	});
}

// read: use3rd, toDelete, k*, w0, E, envelope, vertices, ampOld, dAmp
// write: E, ampOld
void Packets::UpdateAmp() {
	packets.for_each([&](auto& packet, auto it) {
		if (!packet.use3rd && !packet.toDelete)
		{
			const float dampViscosity = -2.0f * packet.k * packet.k * KINEMATIC_VISCOSITY;
			const float dampJunkfilm = -0.5f * packet.k * sqrt(0.5f * KINEMATIC_VISCOSITY * packet.w0);
			packet.E *= exp((dampViscosity + dampJunkfilm) * m_elapsedTime * m_softDampFactor);   // wavelength-dependent damping

			// amplitude computation: lower if too steep, delete if too low
			const float area = packet.envelope * packet.vertices[0].dist_to(packet.vertices[1]);
			if (calc_amp(area, packet.E, packet.k) * packet.k < PACKET_KILL_AMPLITUDE_DERIV) {
				packet.toDelete = true;
				return;
			}

			// get the biggest wave as reference for energy reduction (conservative but important to not remove too much energy in case of large k intervals)
			const auto a_L = calc_amp(area, packet.E, packet.k_L);
			const auto a_H = calc_amp(area, packet.E, packet.k_H);
			const auto b = a_L * packet.k_L < a_H * packet.k_H;
			const auto k = b ? packet.k_L : packet.k_H;
			const auto amp = min(calc_max_amp(k), b ? a_L : a_H);
			packet.E = calc_E(amp, area, k);
			packet.ampOld = min(amp, packet.ampOld + packet.dAmp); // smoothly increase amplitude from last timestep
		}
	});
}

// read: vertices
// write: midPos, traverDir, bending
void Packets::UpdateDisplayVars() {
	packets.for_each([&](auto& packet, auto it) {
		// update variables needed for packet display
		const Vector2f posMidNew = avg(packet.vertices[0].pos, packet.vertices[1].pos);
		const Vector2f posMidOld = avg(packet.vertices[0].lastPos, packet.vertices[1].lastPos);
		const Vector2f dirN = (posMidNew - posMidOld).normalized();				// vector in traveling direction
		packet.midPos = posMidNew;
		packet.travelDir = dirN;
		packet.bending = intersection_dist(posMidNew, dirN, packet.vertices[0].pos, packet.vertices[0].dir);
	});
}

// read: speed, dir, dPhase, ampOld, dAmp, phase
// write: pos, phast, ampOld, delete
void Packets::UpdateGhosts() {
	// advect ghost packets
	ghosts.for_each([&](auto& ghost, auto it) {
		ghost.pos += m_elapsedTime * ghost.speed * ghost.dir;
		ghost.phase += ghost.dPhase;
		ghost.ampOld = max(0.0f, ghost.ampOld - m_softDampFactor * ghost.dAmp);  // decrease amplitude to let this wave disappear (take budget-based softdamping into account)
	});
}

// updates the wavefield using the movin wavefronts and generated an output image from the wavefield
void Packets::AdvectWavePackets(float dTime)
{
	m_time += dTime;					// time stepping
	m_elapsedTime = dTime;
	if (m_elapsedTime <= 0.0)  // if there is no time advancement, do not update anything..
		return;

	UpdatePacketVertices();
	UpdateBounces();

	WavenumberSubdivision();
	RegularSubdivision();
	SlidingSubdivision();

	DeleteOutwardsPackets();

	// damping, insignificant packet removal (if too low amplitude), reduce energy of steep waves with too high gradient
	m_softDampFactor = 1.0f + 100.0f * pow(max(0.0f, (float)(packets.nused()) / (float)(m_packetBudget) - 1.0f), 2.0f);

	UpdateAmp();
	packets.delete_if([](auto& packet) { return packet.toDelete;  });
	UpdateDisplayVars();

	UpdateGhosts();
	ghosts.delete_if([](auto& ghost) { return ghost.ampOld <= 0.0f; });
}
