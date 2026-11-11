#include <kcore/Random.hh>

namespace kc {

bool isValidUuid(std::string_view value) noexcept {
    return uuids::uuid::is_valid_uuid(value);
}

Opt<Uuid> parseUuid(std::string_view value) {
    const auto uuid = uuids::uuid::from_string(value);
    if (not uuid) {
        return {};
    }
    return uuids::to_string(*uuid);
}

RandomEngine::RandomEngine()
    : m_engine(std::random_device{}()), m_uuidGen(m_engine) {}

Str RandomEngine::uuid() {
    std::lock_guard lock{m_mutex};
    return uuids::to_string(m_uuidGen());
}

WithUuid::WithUuid() : m_uuid(RandomEngine::get().uuid()) {}

const Uuid& WithUuid::uuid() const { return m_uuid; }

}  // namespace kc
