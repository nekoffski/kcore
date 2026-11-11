#pragma once

#include <uuid.h>

#include <kcore/Core.hh>
#include <kcore/Singleton.hh>
#include <mutex>
#include <random>
#include <string_view>

namespace kc {

bool isValidUuid(std::string_view value) noexcept;
Opt<Uuid> parseUuid(std::string_view value);

class RandomEngine : public Singleton<RandomEngine> {
   public:
    explicit RandomEngine();

    Str uuid();

   private:
    std::mutex m_mutex;
    std::mt19937 m_engine;
    uuids::uuid_random_generator m_uuidGen;
};

class WithUuid : public virtual NonCopyable, public virtual NonMovable {
   public:
    explicit WithUuid();

    const Uuid& uuid() const;

   private:
    Uuid m_uuid;
};

}  // namespace kc
