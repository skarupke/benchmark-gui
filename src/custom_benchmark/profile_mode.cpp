#include "custom_benchmark/profile_mode.hpp"
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>

using namespace boost::interprocess;

namespace skb
{
struct ProfileModeArgs {
    int benchmark_index = -1;
    int argument = -1;
};

shared_memory_object & InitProfileMode(boost::interprocess::mode_t mode) {
    static shared_memory_object shm(open_or_create, "benchmark_gui_profile_mode", mode);
    if (mode == read_write) {
        shm.truncate(sizeof(ProfileModeArgs));
    }
    return shm;
}

void EnableProfileMode(int benchmark_index, int argument) {
    shared_memory_object & shm = InitProfileMode(read_write);
    mapped_region region(shm, read_write);
    ProfileModeArgs* running = static_cast<ProfileModeArgs*>(region.get_address());
    *running = ProfileModeArgs{benchmark_index, argument};
}
void DisableProfileMode() {
    EnableProfileMode(-1, -1);
}

bool IsProfileMode(int benchmark_index, int argument) {
    shared_memory_object & shm = InitProfileMode(read_only);
    mapped_region region(shm, read_only);
    ProfileModeArgs args = *static_cast<const ProfileModeArgs*>(region.get_address());
    return args.benchmark_index == benchmark_index && args.argument == argument;
}
}
