#include <iostream>
#include <vector>
#include <cstdint>
#include <stdfloat>

#include <utility>
#include <memory>
inline void syncw_stdio(bool s) {
   std::ios_base::sync_with_stdio(s);
}
inline thread_local int cobalt__try_status__ = 0;
inline int TryStatus() { return cobalt__try_status__; }
#include <csystem.hpp>
#include <cotype.hpp>
#include <fsys.hpp>
#include <errors.hpp>
#include <runtime.hpp>
#include <inf.hpp>
#include <cstr.hpp>
#include <fstream>
#include <cstdio>

class UserList {
public:
    c_string getName() {
        return name;
    }
private:
    std::unique_ptr<c_string> name = std::make_unique<c_string>("");
    std::unique_ptr<int> age = std::make_unique<int>(0);
};
UserList UserList;


int main();

int main() {
    syncw_stdio(false);
    println_c(UserList.getName());
}

