#include <iostream>
#include <vector>
#include <cstdint>
#include <stdfloat>

#include <utility>
#include <memory>
#include <type_traits>
#include <cnow.hpp>

inline void syncw_stdio(bool s) {
   std::ios_base::sync_with_stdio(s);
}
inline thread_local int cobalt__try_status__ = 0;
inline int TryStatus() { return cobalt__try_status__; }

#include "W:/Cobalt-Language/bin/lib/encryption/encryption.hpp"
#include "W:/Cobalt-Language/bin/lib/network/network.hpp"
#include <csystem.hpp>
#include <cotype.hpp>
#include <fsys.hpp>
#include <errors.hpp>
#include <runtime.hpp>
#include <inf.hpp>
#include <cstr.hpp>
#include <fstream>
#include <cstdio>



int main();

int main() {
    syncw_stdio(false);
    ServerTitle(unwrap_val("New Server"));
    ServerBody(unwrap_val(csm::toStr("<h1>Hello, World!</h1>")));
    RServer(unwrap_val(8080));
    return 0;
}

