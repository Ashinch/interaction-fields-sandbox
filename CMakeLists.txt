cmake_minimum_required(VERSION 3.20)
project(run C)

link_libraries("/usr/lib/libseccomp.so")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/output)
set(CMAKE_C_FLAGS "-g -O3 -std=c99 -fPIC")
add_executable(run main.c main.h rules/general.c rules/seccomp_rules.h utils.c utils.h killer.c killer.h child.c child.h runner.c runner.h)

install(FILES output/run
        PERMISSIONS OWNER_EXECUTE OWNER_READ OWNER_WRITE GROUP_EXECUTE GROUP_READ GROUP_WRITE WORLD_EXECUTE WORLD_READ WORLD_WRITE
        DESTINATION /usr/bin)
