cmake_minimum_required(VERSION 2.6)
project(particles_bench)

set(TEMP_SRC
    main.c
    glad.c
)

add_executable(particles_bench ${TEMP_SRC})

target_link_libraries(particles_bench glut m dl)
