#pragma once

#include <yaml-cpp/yaml.h>

#include <kcore/io/FileSystem.hh>

namespace kc {

Result<YAML::Node> loadYaml(
    const Path& path, const FileSystem& fs = FileSystem{}
);

template <typename T>
Result<T> yamlField(const YAML::Node& node, const Str& key) {
    try {
        if (node.IsDefined() and node.IsMap()) {
            const auto field = node[key];
            T value;
            if (field.IsDefined() and YAML::convert<T>::decode(field, value)) {
                return value;
            }
        }
    } catch (const YAML::Exception&) {
    }
    return Error::unexpected(
        ErrorCode::invalidConfig, "Missing or invalid field: {}", key
    );
}

}  // namespace kc
