#pragma once

#include <vector>


using namespace Eigen;

#define SCENE_SIZE 50.0f

template<typename T>
class map {
public:
	vector<T> storage;
	int width, height;

	map(int _width = 0, int _height = 0) : width(_width), height(_height), storage(_width* _height) {}

	void resize(int _width, int _height) {
		width = _width;
		height = _height;
		storage.resize(width * height);
	}

private:
	inline int index(int x, int y) const noexcept {
		return y * width + x;
	}

	inline int index_safe(int x, int y) const noexcept {
		return index(clamp<int>(x, 0, width - 1), clamp<float>(y, 0, height - 1));
	}

public:
	inline T& at(int x, int y) noexcept {
		return storage[index_safe(x, y)];
	}

	inline T& at(float x, float y) noexcept {
		return at(int(x), int(y));
	}

	inline T& at(const Vector2f& v) noexcept {
		return at(v.x(), v.y());
	}

	inline const T& at(int x, int y) const noexcept {
		return storage[index_safe(x, y)];
	}

	inline const T& at(float x, float y) const noexcept {
		return at(static_cast<int>(x), static_cast<int>(y));
	}

	inline const T& at(const Vector2f& v) const noexcept {
		return at(v.x(), v.y());
	}

	inline T interp2(float xf, float yf) const {
		float x, y;
		const auto xOffs = modf(xf, &x);
		const auto yOffs = modf(yf, &y);
		const auto& val1 = at(0 + x, 0 + y);
		const auto& val2 = at(1 + x, 0 + y);
		const auto& val3 = at(0 + x, 1 + y);
		const auto& val4 = at(1 + x, 1 + y);
		const auto valH1 = interp<T>(val1, val2, xOffs);
		const auto valH2 = interp<T>(val3, val4, xOffs);
		return interp<T>(valH1, valH2, yOffs);
	}

	inline T interp2(const Vector2f& v) const {
		return interp2(v.x(), v.y());
	}

private:
	inline Vector2f world2tex(const Vector2f& p) const {
		return Vector2f(
			(p.x() / SCENE_SIZE + 0.5f) * width,
			(p.y() / SCENE_SIZE + 0.5f) * height
		);
	}

public:
	inline T at_world(const Vector2f& p) const {
		return interp2(world2tex(p));
	}

	inline Vector2f deriv(int x, int y) const {
		return Vector2f(
			at(x + 1, y) - at(x - 1, y),
			at(x, y + 1) - at(x, y - 1)
		);
	}
};
