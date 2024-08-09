<!--
SPDX-FileCopyrightText: Copyright © DUNE Project contributors, see file LICENSE.md in module root
SPDX-License-Identifier: LGPL-2.1-or-later
-->

# dune-uggrid

This is a fork of the old [UG](https://doi.org/10.1007/s007910050003)
finite element software, wrapped as a Dune module, and stripped of everything
but the grid data structure. You need this module if you want to use the `UGGrid`
grid implementation from the `dune-grid` module.

To use this module, simply have it available when building `dune-grid`.
The Dune build system will then pick it up when building `dune-grid`,
and the `UGGrid` grid implementation becomes available.

## Features

The `UGGrid` grid data structure is very powerful and flexible.  Its main features are:

* Unstructured two- and three-dimensional grids
* Grids with more than one element type: triangles and quadrilaterals in 2d,
  tetrahedra, hexahedra, prisms and pyramids in 3d
* Adaptive red-green refinement, and nonconforming refinement
* Anisotropic refinement
* Distributed grids on large numbers of processes, connected by MPI
* Free-form domain boundaries:  Individual grids always have polyhedral
  boundaries, but refining the grids will approximate the actual
  (free-form) boundary better and better.

The features of `UGGrid` are described in more detail in the
[Dune book](https://link.springer.com/book/10.1007/978-3-030-59702-3).

## Planned features

The following features are natural next steps in the development
of `dune-uggrid`.

* Checkpointing: Save the entire grid hierarchy to a file,
  and read it back in again.
* Direct construction of distributed grids (instead of constructing
  the grid on rank 0 and then distributing)
* Two-dimensional grids in a three-dimensional world

## Releases

Although technically not a "Dune core module",
`dune-uggrid` is released together with the Dune core modules.
Starting with version 2.10 you can find `dune-uggrid` release
tarballs at the [same site](https://dune-project.org/releases/)
as the Dune core module releases.

Older release tarballs can be found at the following links:

<table>
<tr>
  <th>version</th>
  <th>source</th>
  <th>signature</th>
</tr>
<tr>
  <td>2.9.1</td>
  <td><a href="https://dune-project.org/download/2.9.1/dune-uggrid-2.9.1.tar.gz" download>dune-uggrid-2.9.1.tar.gz</a></td>
  <td><a href="https://dune-project.org/download/2.9.1/dune-uggrid-2.9.1.tar.gz.asc" download>dune-uggrid-2.9.1.tar.gz.asc</a></td>
</tr>
<tr>
  <td>2.8.0</td>
  <td><a href="https://dune-project.org/download/2.8.0/dune-uggrid-2.8.0.tar.gz" download>dune-uggrid-2.8.0.tar.gz</a></td>
  <td><a href="https://dune-project.org/download/2.8.0/dune-uggrid-2.8.0.tar.gz.asc" download>dune-uggrid-2.8.0.tar.gz.asc</a></td>
</tr>
</table>

## Maintainer

The `dune-uggrid` module is currently maintained by [Oliver Sander](https://tu-dresden.de/mn/math/numerik/sander).
