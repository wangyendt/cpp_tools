#pragma once

#include <string>
#include <fstream>
#include <filesystem>
#include <libgen.h>
#include <ctime>
#include <chrono>

namespace cw {
	class SaveFileUtils {
	public:
		SaveFileUtils(std::string path, bool new_file) {
			if (std::filesystem::exists(path)) {
				if (std::filesystem::is_regular_file(path)) {
					if (new_file) {
						std::filesystem::remove(path);
					}
					is_available = true;
				} else {
					is_available = false;
				}
			} else {
				auto cur_path = std::filesystem::path(path);
				auto parent_path = cur_path.parent_path();
				if (!std::filesystem::exists(parent_path)) {
					try {
						std::filesystem::create_directories(parent_path);
						is_available = true;
					} catch (std::exception) {
						is_available = false;
					}
				} else {
					is_available = true;
				}
			}
			if (is_available) {
				ofstream = std::ofstream(path, std::ios::out | std::ios::app);
			}
		}

		bool isOpen() {
			return is_available;
		}

		template<typename T>
		void writeItem(T item, std::string delimiter = ",") {
			if (ofstream.is_open()) {
				if constexpr (std::is_same_v<T, std::string>) {
					ofstream << item << delimiter;
				} else{
					ofstream << std::to_string(item) << delimiter;
				}
			}
		}

		void writeLines(std::string line) {
			if (ofstream.is_open()) {
				ofstream << line << "\n";
			}
		}

		void save() {
			if (ofstream.is_open()) {
				ofstream.flush();
			}
		}

		void close() {
			if (ofstream.is_open()) {
				ofstream.close();
			}
		}

		static void makeDir(std::string path) {
			if (!std::filesystem::exists(path)) {
				std::filesystem::create_directories(path);
			}
		}

		static void clearDir(std::string dirName) {
			if (std::filesystem::exists(dirName) && std::filesystem::is_directory(dirName)) {
				for (auto file: std::filesystem::recursive_directory_iterator(dirName)) {
					if (std::filesystem::is_regular_file(file)) {
						std::filesystem::remove(file);
					}
				}
			}
		}

	private:
		std::ofstream ofstream;
		bool is_available;
	};
}
