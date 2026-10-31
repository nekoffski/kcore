#include <kcore/serialization/Json.hh>

namespace kc {

Result<Json> parseJson(const Str& str) {
    try {
        return Json::parse(str);
    } catch (const Json::exception& e) {
        return Error::unexpected(
            ErrorCode::jsonParsingError, "Could not parse json: {}", e.what()
        );
    }
}

}  // namespace kc
