#pragma once

#include <iostream>
#include <sstream>
#include <string>
#include <initializer_list>
#include <stdexcept>
#include <tuple>
#include <utility>
#include <vector>

namespace cw {

    namespace {
        // Overload for initializer_list
		template<typename T>
		std::string join(const std::initializer_list<T>& elements, const std::string& delimiter) {
			return join<std::initializer_list<T>>(elements, delimiter);
		}
	}

    // Template join function to join elements of any container with a delimiter
    template<typename Iterable>
    inline std::string join(const Iterable& elements, const std::string& delimiter) {
        std::ostringstream oss;
        auto iter = elements.begin();
        
        if (iter != elements.end()) {
            oss << *iter++;
        }
        
        while (iter != elements.end()) {
            oss << delimiter << *iter++;
        }
        
        return oss.str();
    }

	

	namespace {
        // Helper function to format a single argument
        template <typename T>
        void format_arg(std::ostringstream& oss, const T& value) {
            oss << value;
        }

        // Recursive function to process format string and arguments
        template <std::size_t Index, typename Tuple>
        void process_format(std::ostringstream& oss, const std::string& fmt, const Tuple& t) {
            std::string::size_type pos = fmt.find('{');
            if (pos == std::string::npos) {
                oss << fmt;
                return;
            }
            oss << fmt.substr(0, pos);
            auto end_brace = fmt.find('}', pos);
            if (end_brace == std::string::npos) {
                throw std::runtime_error("Mismatched braces in format string");
            }

            if constexpr (Index < std::tuple_size_v<Tuple>) {
                format_arg(oss, std::get<Index>(t));
                process_format<Index + 1>(oss, fmt.substr(end_brace + 1), t);
            } else {
                throw std::out_of_range("Argument index out of range");
            }
        }
	}

    // Main format function
    template <typename... Args>
    inline std::string format(const std::string& fmt, Args&&... args) {
        std::ostringstream oss;
        std::tuple<Args...> arg_tuple(std::forward<Args>(args)...);
        process_format<0>(oss, fmt, arg_tuple);
        return oss.str();
    }

    inline std::vector<std::string> split(const std::string& str, char delimiter) {
        std::vector<std::string> tokens;
        std::string token;
        std::istringstream tokenStream(str);
        while (std::getline(tokenStream, token, delimiter)) {
            tokens.emplace_back(token);
        }
        return tokens;
    }
}

