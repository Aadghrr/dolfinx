// Copyright (C) 2020 Michal Habera
//
// This file is part of DOLFINX (https://www.fenicsproject.org)
//
// SPDX-License-Identifier:    LGPL-3.0-or-later

#pragma once

#include "pugixml.hpp"
#include "xdmf_mesh.h"
#include "xdmf_utils.h"
#include <dolfinx/common/MPI.h>
#include <dolfinx/mesh/MeshTags.h>
#include <hdf5.h>
#include <string>
#include <vector>

namespace dolfinx
{
namespace io
{
namespace xdmf_meshtags
{

/// Add mesh tags to XDMF file
template <typename T>
void add_meshtags(MPI_Comm comm, const mesh::MeshTags<T>& meshtags,
                  pugi::xml_node& xml_node, const hid_t h5_id,
                  const std::string name)
{
  // Get mesh
  assert(meshtags.mesh());
  std::shared_ptr<const mesh::Mesh> mesh = meshtags.mesh();
  const int dim = meshtags.dim();
  const std::int32_t num_local_entities
      = mesh->topology().index_map(dim)->size_local()
        * mesh->topology().index_map(dim)->block_size();
  const std::vector<std::int32_t>& active_entities = meshtags.indices();
  const std::vector<T>& values = meshtags.values();

  // Find number of tagged entities in local range
  const int num_active_entities
      = std::lower_bound(active_entities.begin(), active_entities.end(),
                         num_local_entities)
        - active_entities.begin();

  const std::vector<std::int32_t> local_entities(
      active_entities.begin(), active_entities.begin() + num_active_entities);
  const std::vector<T> local_values(values.begin(),
                                    values.begin() + num_active_entities);
  const std::string path_prefix = "/MeshTags/" + name;
  xdmf_mesh::add_topology_data(comm, xml_node, h5_id, path_prefix,
                               mesh->topology(), mesh->geometry(), dim,
                               local_entities);

  // Add attribute node with values
  pugi::xml_node attribute_node = xml_node.append_child("Attribute");
  assert(attribute_node);
  attribute_node.append_attribute("Name") = name.c_str();
  attribute_node.append_attribute("AttributeType") = "Scalar";
  attribute_node.append_attribute("Center") = "Cell";

  std::int64_t global_num_values = 0;
  const std::int64_t local_num_values = local_entities.size();
  MPI_Allreduce(&local_num_values, &global_num_values, 1, MPI_INT64_T, MPI_SUM,
                comm);
  const std::int64_t offset
      = dolfinx::MPI::global_offset(comm, local_entities.size(), true);
  const bool use_mpi_io = (dolfinx::MPI::size(comm) > 1);
  xdmf_utils::add_data_item(attribute_node, h5_id, path_prefix + "/Values",
                            local_values, offset, {global_num_values, 1}, "",
                            use_mpi_io);
}

} // namespace xdmf_meshtags
} // namespace io
} // namespace dolfinx