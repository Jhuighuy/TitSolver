/** Copyright (C) Oleg Butakov 2020. */
#pragma once
#ifndef TIT_PARTICLE_TREE_
#define TIT_PARTICLE_TREE_

#include <array>
#include <memory>
#include <algorithm>
//#include <execution>

#include "TitParticle.hpp"

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/********************************************************************
 ** Barnes-Hut tree node.
 ********************************************************************/
template<typename real_t, int nDim>
struct TParticleTreeNode {
  TVector<real_t, nDim> LowerLeftCorner;
  TVector<real_t, nDim> UpperRightCorner;
  TParticleTreeNode* RootNode;
  std::array<std::unique_ptr<TParticleTreeNode>, (1 << nDim)> LeafNodes;
};  // struct TParticleTreeNode



/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#endif  // TIT_PARTICLE_TREE_
