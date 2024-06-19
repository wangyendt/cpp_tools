#pragma once

namespace cw {
	template<typename... Args>
	inline bool all(Args... args) {
		return (... && args);
	}

	template<typename... Args>
	inline bool any(Args... args) {
		return (... || args);
	}

	template<typename... Args>
	inline bool none(Args... args) {
		return !(... || args);
	}

	template<typename... Args>
	inline bool notall(Args... args) {
		return !(... && args);
	}
}
