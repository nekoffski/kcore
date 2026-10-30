#pragma once

#include <kcore/Core.hh>
#include <kcore/Error.hh>
#include <nlohmann/json.hpp>

namespace kc {

using namespace nlohmann;

using Json = nlohmann::json;

Result<Json> parseJson(const Str& str);

}  // namespace kc
