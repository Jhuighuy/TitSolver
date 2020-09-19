cmake_minimum_required(VERSION 3.17)
project(test_sph)

set(CMAKE_CXX_STANDARD 17)

include_directories(.)

add_executable(test_sph
        sph.cpp
        TitArtificialViscosity.hpp
        TitParticle.hpp
        TitRootFinder.hpp
        TitSmoothingKernels.hpp)
