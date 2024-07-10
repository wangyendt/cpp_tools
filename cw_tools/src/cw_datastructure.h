#pragma once

#include <unordered_map>
#include <utility> // for std::move

namespace cw {
	
	template<typename Key, typename Value, typename Hash = std::hash<Key>>
	class DefaultDict {
	public:
		// Default constructor
		DefaultDict() : default_value(Value()) {}

		// Constructor with default value
		DefaultDict(const Value& default_value) : default_value(default_value) {}

		// Copy constructor
		DefaultDict(const DefaultDict& other) = default;

		// Move constructor
		DefaultDict(DefaultDict&& other) noexcept = default;

		// Copy assignment
		DefaultDict& operator=(const DefaultDict& other) = default;

		// Move assignment
		DefaultDict& operator=(DefaultDict&& other) noexcept = default;

		// Operator []
		Value& operator[](const Key& key) {
			return data[key];
		}

		const Value& operator[](const Key& key) const {
			auto it = data.find(key);
			if (it != data.end()) {
				return it->second;
			} else {
				return default_value;
			}
		}

		std::size_t size() const {
			return data.size();
		}

		void clear() {
			data.clear();
		}

		// Iterator support
		auto begin() { return data.begin(); }
		auto end() { return data.end(); }
		auto begin() const { return data.begin(); }
		auto end() const { return data.end(); }

	private:
		std::unordered_map<Key, Value, Hash> data;
		Value default_value;
	};
}
