#pragma once

#include <uuid.h>

#include <kcore/Core.hh>
#include <kcore/Singleton.hh>
#include <random>

namespace kc {

class RandomEngine : public Singleton<RandomEngine> {
   public:
    explicit RandomEngine();

    Str uuid();

   private:
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
