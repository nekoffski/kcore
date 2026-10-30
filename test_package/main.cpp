#include <kcore/Error.hh>
#include <kcore/Functional.hh>
#include <kcore/containers/FlatMap.hh>
#include <kcore/io/FileSystem.hh>
#include <kcore/serialization/Json.hh>

int main() {
    int calls = 0;
    {
        KCORE_DEFER { ++calls; };
        auto lazy = KCORE_LAZY_EVALUATE(++calls);
        if (int(lazy) != 1) {
            return 4;
        }
    }
    if (calls != 2) {
        return 5;
    }
    const kc::Error error{kc::ErrorCode::fileNotFound, "Missing {}", "plan"};
    if (fmt::format("{}", error) != "code: kcore:9, details: 'Missing plan'") {
        return 1;
    }
    auto json = kc::parseJson("{\"ok\":true}");
    if (!json || !json->at("ok").get<bool>()) {
        return 2;
    }
    kc::FlatMap<int, int> values;
    values.insert(1, 42);
    if (!values.find(1) || *values.find(1) != 42) {
        return 3;
    }
    return kc::Path::join("/tmp", "plan").str() != "/tmp/plan";
}
