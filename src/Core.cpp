#include <kcore/Core.hh>
#include <kcore/Random.hh>

namespace kc {

Tag<Str> Tag<Str>::fromUuid() { return Tag<Str>{RandomEngine::get().uuid()}; }

}  // namespace kc
