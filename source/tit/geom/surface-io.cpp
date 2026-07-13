/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <algorithm>
#include <array>
#include <cstddef>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <utility>

#include <assimp/Exporter.hpp>
#include <assimp/Importer.hpp>
#include <assimp/cexport.h>
#include <assimp/defs.h>
#include <assimp/material.h>
#include <assimp/mesh.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/types.h>
#include <assimp/vector3.h>

#include "tit/core/assert.hpp"
#include "tit/core/exception.hpp"
#include "tit/core/float.hpp"
#include "tit/core/math.hpp"
#include "tit/core/utils.hpp"
#include "tit/core/vec.hpp"
#include "tit/geom/surface.hpp"

namespace tit::geom {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Assimp post-processing flags.
constexpr auto process_flags = aiProcess_JoinIdenticalVertices | //
                               aiProcess_Triangulate |           //
                               aiProcess_SortByPType |
                               aiProcess_PreTransformVertices |
                               aiProcess_ValidateDataStructure;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Construct a surface from an Assimp scene.
template<std::size_t Dim>
auto surface_from_scene(const aiScene& scene) -> IOSurface<Dim> {
  IOSurface<Dim> result;

  // Append the meshes of the scene to the surface.
  for (const auto& mesh : std::span{scene.mMeshes, scene.mNumMeshes}) {
    // Check that the mesh has correct primitive type.
    TIT_ALWAYS_ASSERT(mesh != nullptr, "Mesh is null!");
    if constexpr (Dim == 2) {
      TIT_ENSURE(mesh->mPrimitiveTypes == aiPrimitiveType_LINE,
                 "Mesh is not 2D.");
    } else if constexpr (Dim == 3) {
      TIT_ENSURE(mesh->mPrimitiveTypes == aiPrimitiveType_TRIANGLE,
                 "Mesh is not 3D.");
    } else {
      static_assert(false);
    }

    // Skip empty meshes.
    if (mesh->mNumVertices == 0) continue;
    if (mesh->mNumFaces == 0) continue;

    // Append the vertices of the mesh to the surface.
    const auto vert_offset = result.num_verts();
    TIT_ALWAYS_ASSERT(mesh->mVertices != nullptr, "Mesh has no vertices!");
    for (const auto& vert : std::span{mesh->mVertices, mesh->mNumVertices}) {
      if constexpr (Dim == 2) {
        TIT_ENSURE(is_tiny(vert.z), "2D mesh does not lie in the `z=0` plane.");
        result.append_vert(vec_cast<float32_t>(Vec{vert.x, vert.y}));
      } else if constexpr (Dim == 3) {
        result.append_vert(vec_cast<float32_t>(Vec{vert.x, vert.y, vert.z}));
      } else {
        static_assert(false);
      }
    }

    // Append the faces of the mesh to the surface.
    TIT_ALWAYS_ASSERT(mesh->mFaces != nullptr, "Mesh has no faces!");
    for (const auto& face : std::span{mesh->mFaces, mesh->mNumFaces}) {
      TIT_ALWAYS_ASSERT(face.mIndices != nullptr, "Face has no indices!");
      TIT_ALWAYS_ASSERT(face.mNumIndices == Dim,
                        "Face has an incorrect number of vertices!");
      std::array<std::size_t, Dim> face_verts{};
      for (std::size_t i = 0; i < Dim; ++i) {
        TIT_ALWAYS_ASSERT(face.mIndices[i] < mesh->mNumVertices,
                          "Face has an invalid vertex index.");
        face_verts[i] = vert_offset + face.mIndices[i];
      }
      result.append_face(face_verts);
    }
  }

  return result;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Construct an Assimp scene from a surface.
template<std::size_t Dim>
auto scene_to_surface(const IOSurface<Dim>& surf) -> std::unique_ptr<aiScene> {
  // Construct the scene.
  auto scene = std::make_unique<aiScene>();

  // Construct the material and attach it to the scene.
  // Assimp requires some material to be present.
  scene->mNumMaterials = 1;
  scene->mMaterials = make_array<aiMaterial*>(scene->mNumMaterials);
  scene->mMaterials[0] = make<aiMaterial>();

  // Construct the mesh and attach it to the scene.
  scene->mNumMeshes = 1;
  scene->mMeshes = make_array<aiMesh*>(scene->mNumMeshes);
  scene->mMeshes[0] = make<aiMesh>();
  auto* const mesh = scene->mMeshes[0];
  mesh->mMaterialIndex = 0;
  mesh->mPrimitiveTypes = [] {
    if constexpr (Dim == 2) return aiPrimitiveType_LINE;
    else if constexpr (Dim == 3) return aiPrimitiveType_TRIANGLE;
    else static_assert(false);
  }();

  // Construct the root node and attach it to the scene and the mesh.
  auto* const root_node = make<aiNode>();
  scene->mRootNode = root_node;
  scene->mNumMeshes = 1;
  root_node->mName = "BlueTit Surface";
  root_node->mNumMeshes = 1;
  root_node->mMeshes = make_array<unsigned int>(root_node->mNumMeshes);
  root_node->mMeshes[0] = 0;

  // Populate the mesh vertices.
  TIT_ENSURE(std::in_range<unsigned int>(surf.num_verts()),
             "Surface vertices count is out of unsigned integer range.");
  mesh->mNumVertices = static_cast<unsigned int>(surf.num_verts());
  mesh->mVertices = make_array<aiVector3D>(mesh->mNumVertices);
  std::ranges::transform( //
      surf.verts(),
      mesh->mVertices,
      [](const auto& vert) -> aiVector3D {
        if constexpr (Dim == 2) {
          return {static_cast<ai_real>(vert[0]),
                  static_cast<ai_real>(vert[1]),
                  ai_real{0}};
        } else if constexpr (Dim == 3) {
          return {static_cast<ai_real>(vert[0]),
                  static_cast<ai_real>(vert[1]),
                  static_cast<ai_real>(vert[2])};
        } else {
          static_assert(false);
        }
      });

  // Populate the mesh faces.
  TIT_ENSURE(std::in_range<unsigned int>(surf.num_faces()),
             "Surface faces count is out of unsigned integer range.");
  mesh->mNumFaces = static_cast<unsigned int>(surf.num_faces());
  mesh->mFaces = make_array<aiFace>(mesh->mNumFaces);
  std::ranges::transform(
      surf.face_verts(),
      mesh->mFaces,
      [](const auto& face_verts) {
        aiFace face;
        face.mNumIndices = Dim;
        face.mIndices = make_array<unsigned int>(face.mNumIndices);
        std::ranges::transform(
            face_verts,
            face.mIndices,
            [](std::size_t index) {
              TIT_ENSURE(std::in_range<unsigned int>(index),
                         "Face vertex index is out of unsigned integer range.");
              return static_cast<unsigned int>(index);
            });
        return face;
      });

  return scene;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace

template<std::size_t Dim>
auto surface_from_string(std::string_view str) -> IOSurface<Dim> {
  // Import the scene.
  Assimp::Importer importer;
  const auto* const scene =
      importer.ReadFileFromMemory(str.data(), str.size(), process_flags);
  TIT_ENSURE(scene != nullptr, "Import failed: {}.", importer.GetErrorString());

  // Convert the scene to a surface.
  return surface_from_scene<Dim>(*scene);
}

template auto surface_from_string<2>(std::string_view str) -> IOSurface<2>;
template auto surface_from_string<3>(std::string_view str) -> IOSurface<3>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

template<std::size_t Dim>
auto surface_dump_string(const IOSurface<Dim>& surf, const std::string& format)
    -> std::string {
  // Convert the surface to an Assimp scene.
  const auto scene = scene_to_surface(surf);

  // Export the mesh.
  Assimp::Exporter exporter;
  const auto* const blob = exporter.ExportToBlob(scene.get(), format);
  TIT_ENSURE(blob != nullptr, "Export failed: {}.", exporter.GetErrorString());
  TIT_ENSURE(blob->next == nullptr,
             "Export format '{}' produces multiple files.",
             format);

  // Return the blob as a string.
  return {static_cast<const char*>(blob->data),
          static_cast<std::size_t>(blob->size)};
}

template auto surface_dump_string<2>(const IOSurface<2>& surf,
                                     const std::string& format) -> std::string;
template auto surface_dump_string<3>(const IOSurface<3>& surf,
                                     const std::string& format) -> std::string;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::geom
