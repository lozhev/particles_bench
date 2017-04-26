cmake_minimum_required(VERSION 2.6)
project(particles_bench)

set(TEMP_SRC
	pch.c
    main.c
    glad.c
    test1.c
    test2.c
    test3.c
    test4.c
    test5.c
)

add_executable(particles_bench ${TEMP_SRC})

target_link_libraries(particles_bench glut m dl)
