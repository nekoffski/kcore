#include <kcore/Handle.hh>

namespace kc {

void HandleKey::invalidate() {
    id = InvalidHandle;
    generation = 0u;
}

bool HandleKey::valid() const { return id != InvalidHandle; }

}  // namespace kc
