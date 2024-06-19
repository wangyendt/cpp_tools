#pragma once

#include <iostream>
#include <fstream>
#include <yaml-cpp/yaml.h>
#include <Eigen/Dense>
#include <string>
#include <map>
#include <cw_basic.h>
#include <cw_string.h>


namespace cw {
    namespace {
        void deepMergeDicts(YAML::Node& original, const YAML::Node& updater) {
            for (YAML::const_iterator it = updater.begin(); it != updater.end(); ++it) {
                const std::string& key = it->first.as<std::string>();
                const YAML::Node& value = it->second;

                if (value.IsMap() && original[key]) {
                    YAML::Node originalNode = original[key];
                    deepMergeDicts(originalNode, value);
                    original[key] = originalNode;
                } else {
                    original[key] = value;
                }
            }
        }
    }

    inline void write_yaml_file(const std::string& configYamlFile, const YAML::Node& config, bool update = false) {
        YAML::Node existingConfig;

        if (update) {
            try {
                existingConfig = YAML::LoadFile(configYamlFile);
            } catch (const YAML::BadFile& e) {
                print("Config file does not exist, creating new one.", "red");
            }
        }

        if (update) {
            deepMergeDicts(existingConfig, config);
        } else {
            existingConfig = config;
        }

        std::ofstream fout(configYamlFile);
        fout << existingConfig;
    }

    inline YAML::Node read_yaml_file(const std::string& configYamlFile) {
        try {
            return YAML::LoadFile(configYamlFile);
        } catch (const YAML::BadFile& e) {
            print(format("Failed to open config file: {}", e.what()), "red");
            return YAML::Node();
        }
    }
}

namespace YAML {
    template<typename T, int Rows, int Cols>
    struct convert<Eigen::Matrix<T, Rows, Cols>> {
        static Node encode(const Eigen::Matrix<T, Rows, Cols>& mat) {
            Node node;
            for (int i = 0; i < mat.rows(); ++i) {
                std::string rowStr;
                for (int j = 0; j < mat.cols(); ++j) {
                    rowStr += std::to_string(mat(i, j));
                    if (j != mat.cols() - 1) {
                        rowStr += ",";
                    }
                }
                node.push_back(rowStr);
            }
            return node;
        }

        static bool decode(const Node& node, Eigen::Matrix<T, Rows, Cols>& mat) {
            if (!node.IsSequence() || node.size() == 0) {
                cw::print("Node is not a sequence or is empty.", "red");
                return false;
            }

            size_t rows = node.size();
            size_t cols = cw::split(node[0].as<std::string>(), ',').size();

            if ((Rows != Eigen::Dynamic && rows != static_cast<size_t>(Rows)) || 
                (Cols != Eigen::Dynamic && cols != static_cast<size_t>(Cols))) {
                cw::print(cw::format("Matrix dimensions do not match. Rows: {} Cols: {}", rows, cols), "red");
                return false;
            }

            mat.resize(rows, cols);

            for (size_t i = 0; i < rows; ++i) {
                std::string rowStr = node[i].as<std::string>();
                auto rowItems = cw::split(rowStr, ',');
                for (size_t j = 0; j < cols; ++j) {
                    mat(i, j) = std::stod(rowItems[j]);
                }
            }
            return true;
        }
    };
}

