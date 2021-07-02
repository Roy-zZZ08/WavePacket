#pragma once

#include <vector>
#include <functional>

using namespace std;

template<typename T>
class pool
{
	size_t freeCount;
	size_t usedCount;
	vector<size_t> free;
	vector<size_t> used;
	vector<T> storage;
	const T& value;

public:
	pool(size_t size = 0, const T& _value = T()) :
		freeCount(size), free(size),
		usedCount(0), used(size),
		storage(size, _value),
		value(_value) {}

	void resize(size_t n) {
		size_t s = free.size();
		free.resize(n);
		used.resize(n);
		for (int i = 0; i < n - s; i++)
			free[freeCount + i] = s + i;
		freeCount += n - s;
		storage.resize(n, value);
	}

	T& operator[](int pos) noexcept {
		return storage[used[pos]];
	}

private:
	T& _claim() {
		int first(0);
#pragma omp critical
		{
			freeCount--;
			first = used[usedCount] = free[freeCount];
			usedCount++;
		}
		return storage[first];
	}

public:
	T& claim() {
		auto& obj = _claim();
		obj = value;
		return obj;
	}

	template<class ... Args>
	T& claim(Args&& ... args) {
		auto& obj = _claim();
		obj = T(args...);
		return obj;
	}

	void reset() noexcept {
		for (int i = 0; i < free.size(); ++i) free[i] = i;
		usedCount = 0;
		freeCount = free.size();
	}

	size_t size() const noexcept {
		return free.size();
	}

	size_t nused() const noexcept {
		return usedCount;
	}

	int rbegin() {
		return usedCount - 1;
	}

	int rend() {
		return -1;
	}

	void erase(const int& id) {
#pragma omp critical
		{
			free[freeCount] = used[id];
			++freeCount;
			--usedCount;
			used[id] = used[usedCount];
		}
	}

	void for_each(std::function<void(T&)> fn) {
#pragma omp parallel for
		for (auto it = rbegin(); it > rend(); --it) fn((*this)[it]);
	}

	void for_each(std::function<void(T&, int)> fn) {
#pragma omp parallel for
		for (auto it = rbegin(); it > rend(); --it) fn((*this)[it], it);
	}

	void for_each_seq(std::function<void(T&)> fn) {
		for (auto it = rbegin(); it > rend(); --it) fn((*this)[it]);
	}

	void for_each_seq(std::function<void(T&, int)> fn) {
		for (auto it = rbegin(); it > rend(); --it) fn((*this)[it], it);
	}

	void delete_if(std::function<bool(const T&)> fn) {
		for_each_seq([&](auto& packet, auto it) {
			if (fn(packet)) erase(it);
		});
	}
};
