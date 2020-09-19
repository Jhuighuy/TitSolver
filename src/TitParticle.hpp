/** Copyright (C) 2020 Oleg Butakov. */
#pragma once
#ifndef TIT_PARTICLES_HPP
#define TIT_PARTICLES_HPP

#include <algorithm>
#include <execution>

#include "TitVector.hpp"
#include "TitSmoothingKernels.hpp"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

template<typename real_t, int nDim=1>
struct TParticle {
    TVector<real_t, nDim> Position;
    TVector<real_t, nDim> Velocity;
    TVector<real_t, nDim> Acceleration;
    real_t ThermalEnergy;
    real_t Heating;

    real_t Mass;
    real_t Density;
    real_t KernelWidth, DensityWidthDerivative;
    real_t Pressure;
    real_t SoundSpeed;

    TParticle() = default;
};  // struct TParticle

template<typename real_t, int nDim>
inline TVector<real_t, nDim> DeltaPosition(
    const TParticle<real_t, nDim>& aParticle, 
    const TParticle<real_t, nDim>& bParticle
) noexcept {
    return aParticle.Position - bParticle.Position;
}   // DeltaPosition
template<typename real_t, int nDim>
inline TVector<real_t, nDim> DeltaVelocity(
    const TParticle<real_t, nDim>& aParticle, 
    const TParticle<real_t, nDim>& bParticle
) noexcept {
    return aParticle.Velocity - bParticle.Velocity;
}   // DeltaVelocity

typedef TParticle<double> Particle;

template<typename real_t, int nDim>
struct TParticleArray {
    TParticle<real_t>* Particles;
    size_t NumParticles;

    void SortParticles() {
        std::sort(std::execution::par_unseq,
                  Particles, Particles+NumParticles, [](const Particle& aParticle, 
                                                        const Particle& bParticle) {
            return Less(aParticle.Position, bParticle.Position);
        });
    }

    template<typename func_t>
    void ForEach(const func_t& func) {
        std::for_each(std::execution::par_unseq,
                      Particles, Particles+NumParticles, func);
    }
    template<typename func_t>
    void ForEachNeighbour(TParticle<real_t, nDim>& aParticle, 
                          real_t searchWidth, const func_t& func) {
        func(aParticle);
        real_t searchWidthSquare = std::pow(searchWidth, 2);
        size_t aParticleIndex = &aParticle - Particles;
        for (size_t bParticleIndex = aParticleIndex+1; 
                    bParticleIndex < NumParticles; ++bParticleIndex) {
            TParticle<real_t, nDim>& bParticle = Particles[bParticleIndex];
            TVector<real_t, nDim> deltaPosisiton = aParticle.Position - bParticle.Position;
            if (Dot(deltaPosisiton, deltaPosisiton) > searchWidthSquare) {
                break;
            }
            func(bParticle);
        }
        for (size_t bParticleIndex = aParticleIndex-1; 
                    bParticleIndex != SIZE_MAX; --bParticleIndex) {
            TParticle<real_t, nDim>& bParticle = Particles[bParticleIndex];
            TVector<real_t, nDim> deltaPosisiton = aParticle.Position - bParticle.Position;
            if (Dot(deltaPosisiton, deltaPosisiton) > searchWidthSquare) {
                break;
            }
            func(bParticle);
        }
    }
};  // struct TParticleArray

template<typename func_t, typename real_t, int nDim>
void ForEach(
    TParticleArray<real_t, nDim>& particles, const func_t& func) {
    particles.ForEach(func);
}

template<typename func_t, typename real_t, int nDim>
void ForEachNeighbour(
    TParticleArray<real_t, nDim>& particles, 
    TParticle<real_t, nDim>& aParticle, real_t searchWidth, 
    const func_t& func) {
    particles.ForEachNeighbour(aParticle, searchWidth, func);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

#endif  // ifndef TIT_PARTICLES_HPP
