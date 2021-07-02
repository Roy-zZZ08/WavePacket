#pragma once

#include <Eigen/Dense>
#include <array>
#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include <cmath>

// simulation parameters
#define PACKET_SPLIT_ANGLE 0.95105f			// direction angle variation threshold: 0.95105=18 degree
#define PACKET_SPLIT_DISPERSION 0.3f		// if the fastest wave in a packet traveled PACKET_SPLIT_DISPERSION*Envelopesize ahead, or the slowest by the same amount behind, subdivide this packet into two wavelength intervals
#define PACKET_KILL_AMPLITUDE_DERIV 0.0001f	// waves below this maximum amplitude derivative gets killed
#define PACKET_BLEND_TRAVEL_FACTOR 1.0f		// in order to be fully blended (appear or disappear), any wave must travel PACKET_BLEND_TRAVEL_FACTOR times "envelope size" in space (1.0 is standard)
#define PACKET_ENVELOPE_SIZE_FACTOR 3.0f	// size of the envelope relative to wavelength (determines how many "bumps" appear)
#define PACKET_ENVELOPE_MINSIZE 0.02f		// minimum envelope size in meters (smallest expected feature)
#define PACKET_ENVELOPE_MAXSIZE 10.0f		// maximum envelope size in meters (largest expected feature)
#define PACKET_BOUNCE_FREQSPLIT true 		// (boolean) should a wave packet produce smaller waves at a bounce/reflection (->widen the wavelength interval of this packet)?
#define PACKET_BOUNCE_FREQSPLIT_K 31.4f		// if k_L is smaller than this value (lambda = 20cm), the wave is (potentially) split after a bounce
#define MAX_SPEEDNESS 0.07f					// all wave amplitudes a are limited to a <= MAX_SPEEDNESS*2.0*M_PI/k

// physical parameters
#define SIGMA 0.074f						// surface tension N/m at 20 grad celsius
#define GRAVITY 9.81f						// GRAVITY m/s^2
#define DENSITY 998.2071f					// water density at 20 degree celsius
#define KINEMATIC_VISCOSITY 0.000109f		// kinematic viscosity
#define PACKET_SLOWAVE_K 143.1405792f		// k of the slowest possible wave packet
#define PACKET_SLOWAVE_W0 40.2646141f		// w_0 of the slowest possible wave packet

// memory management
#define PACKET_BUFFER_DELTA 500000			// initial number of vertices, packet memory will be increased on demand by this stepsize

#include "pool.h"
#include "map.h"
#include "maths.h"
#include "PacketVertex.h"

using namespace Eigen;
using namespace std;

class Packets;

class WavePacket
{
	std::reference_wrapper<Packets> packets;

public:
	WavePacket(Packets& _packets) : packets(_packets) {}
	WavePacket(const WavePacket&) = default;
	WavePacket& operator=(const WavePacket& that) = default;

	std::array<PacketVertex, 3> vertices;

	Vector2f	midPos;						// middle position (tracked each timestep, used for rendering)
	Vector2f	travelDir;					// travel direction (tracked each timestep, used for rendering)
	float		bending;					// point used for circular arc bending of the wave function inside envelope

											// bouncing and sliding
	bool		sliding3;					// indicates if the 3rd vertex is "sliding" (used for diffraction)
	bool		use3rd;						// indicates if the third vertex is present (it marks a (potential) sliding point)

	// wave function related
	float		phase;						// phase of the representative wave inside the envelope, phase speed vs. group speed
	float		phOld;						// old phase
	float		E;							// wave energy flux for this packet (determines amplitude)
	float		envelope;					// envelope size for this packet
	float		k, w0;						// w0 = angular frequency, k = current wavenumber
	float		k_L, w0_L, k_H, w0_H;			// w0 = angular frequency, k = current wavenumber,  L/H are for lower/upper boundary
	float		d_L, d_H;					// d = travel distance to reference wave (gets accumulated over time),  L/H are for lower/upper boundary
	float		ampOld;						// amplitude from last timestep, will be smoothly adjusted in each timestep to meet current desired amplitude
	float		dAmp;						// amplitude change in each timestep (depends on desired waveheight so all waves (dis)appear with same speed)

	// serial deletion step variable
	bool		toDelete;					// used internally for parallel deletion criterion computation
public:
	EIGEN_MAKE_ALIGNED_OPERATOR_NEW;

	void blend_in();
	void update_envelope();
};

struct GhostPacket
{
	Vector2f	pos;					// 2D position
	Vector2f	dir;					// current movement direction
	float		speed;					// speed of the packet
	float		envelope;				// envelope size for this packet
	float		k;						// k = current (representative) wavenumber(s)
	float		phase;					// phase of the representative wave inside the envelope
	float		dPhase;					// phase speed relative to group speed inside the envelope
	float		ampOld;					// amplitude from last timestep, will be smoothly adjusted in each timestep to meet current desired amplitude
	float		dAmp;					// change in amplitude in each timestep (waves travel PACKET_BLEND_TRAVEL_FACTOR*envelopesize in space until they disappear)

	float		bending;				// point used for circular arc bending of the wave function inside envelope
public:
	EIGEN_MAKE_ALIGNED_OPERATOR_NEW

	void set_from_wave(const Packets& packets, const WavePacket& p, const PacketVertex& v);
};

class Packets
{
	// scene
public:
	int			groundWidth, groundHeight;	// pixel size of the ground texture
	map<float>		ground;						// texture containing the water depth and land (0.95)
	map<Vector2f>	groundNormal;
	map<float>		bound;
	map<float>		boundDist;						// distance map of the boundary map
	map<Vector2f>	borderNormal;

	// packet managing
public:
	int			m_packetBudget;					// this can be changed any time (soft budget)
	float		m_softDampFactor;
	pool<WavePacket>	packets;	// wave packet data 
	pool<GhostPacket>	ghosts;		// ghost packet data 

	// simulation
public:
	float		m_time;
	float		m_elapsedTime;

	// initialization
public:
	EIGEN_MAKE_ALIGNED_OPERATOR_NEW;

	Packets(int packetBudget);

private:
	void readMap();
	void boundaryDistanceTransform();
	bool inBounds(int x, int y) const;
	void boundaryDerivatives();
	void groundDerivatives();

public:
	void Reset();

	// calc or query data
private:
	PacketVertex FindBouncePoint(const WavePacket& packet, const PacketVertex& a, const PacketVertex& b);
	void SplitArm(WavePacket& packet, int idx);
	void UpdateWavenumber(WavePacket& packet, float wd) const;
	PacketVertex AdvectPacketVertex(const WavePacket& packet, const PacketVertex& vertex);
	Vector2f projectToBoundary(Vector2f pos) const;
	WaveParameters Packets::recalcWavenumber(WavePacket& packet, const Vector2f& v, bool updateL = true, bool updateH = false) const;

	// create packets
public:
	void CreatePacket(float pos1x, float pos1y, float pos2x, float pos2y, float dir1x, float dir1y, float dir2x, float dir2y, float k_L, float k_H, float E);
	void CreateCircularWavefront(float xPos, float yPos, float radius, float lambda_L, float lambda_H, float E);

	// per frame
public:
	void AdvectWavePackets(float dTime);

private:
	void ExpandWavePacketMemory(int targetNum);
	void UpdatePacketVertices();
	void UpdateBounces();
	void WavenumberSubdivision();
	void RegularSubdivision();
	void SlidingSubdivision();
	void DeleteOutwardsPackets();
	void UpdateAmp();
	void UpdateDisplayVars();
	void UpdateGhosts();
};
