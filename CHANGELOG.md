# dune-uggrid 2.9 (unreleased)

- The `dune-uggrid` module does not set the preprocessor flag `HAVE_UG` anymore.
  Use `HAVE_DUNE_UGGRID` instead.

# dune-uggrid 2.8 (2021-09-06)

* Added support for All_All communication on facets.
* Removes support for `_2` and `_3` macros.
* Remove support for XDR.

# dune-uggrid 2.7.0 (2019-12-20)

* Multiple grids are now also allowed in the parallel implementation
  [!96]: https://gitlab.dune-project.org/staging/dune-uggrid/merge_requests/96

*   The `TET_RULESET` compile-time option has been renamed to
    `DUNE_UGGRID_TET_RULESET` and is enabled by default.
    It is no longer necessary to provide the `RefRules.data` file.
    [!134](https://gitlab.dune-project.org/staging/dune-uggrid/merge_requests/134)

* Header files moved below dune/uggrid. Headers are installed in the same
  sub-directory and no longer into ug/. This breaks compatibility to UG 3.13.
  [!137](https://gitlab.dune-project.org/staging/dune-uggrid/merge_requests/137)

# dune-uggrid 2.6.0 (2018-04-03)

* Transfer of element data during load balancing is supported.
  See [!55][] and [dune-grid!172][]
* `dune-uggrid` has gained a new build-time switch `TET_RULESET` (default is off).
  When set, `UGGrid` will use a better algorithm to compute green closures for red--green
  grid refinement.  This better algorithm, which involves a complete rule set
  for tetrahedral elements, leads to refined grids with smaller closures.
  It also fixes at least one [bug](https://gitlab.dune-project.org/core/dune-grid/issues/27).
  *Beware:* when `TET_RULESET` is set, UG wants to read a table from the file `RefRule.data`
  (in `uggrid/lib/ugdata`).  This file needs to be in the current directory, otherwise `UGGrid`
  will abort. The long-term plan is to compile the data file into the binary, and make
  the new closure algorithm the default.
* Many now unused parts of UG have been removed from the source.
* The memory management of UG got a major overhaul:
  - As malloc/free memory management was used for a long time, we removed
    the manual heap management of UG.
  - `DYNAMIC_MEMORY_ALLOCMODEL` was used by default, now we removed the
    switch and cleaned up all `#ifdef`s
  - The `FROM_BOTTOM`/`FROM_TOP` flags during memory allocation didn't have
    any influence, as we use the system heap. The enums were deprecated
    and the interface updated.
  - unified the different memory allocation methods
  - removed user data from the `MULTIGRID` structure
  - removed virtual heap management
* The `InsertElement` algorithm has been completely rewritten. Up to now the user
  had to ensure that enough memory was allocated in order to create a
  lookup for the `InsertElement` neighbor search. This was necessary to
  ensure an O(n) complexity. Otherwise loading large meshes was
  prohibitively slow. The new implementation uses modern C++ data
  structures. To speed up the neighbor search we maintain a hash-map
  (i.e. `unordered_map`) from a face to an element. The new
  implementation is as fast as the old one, can dynamically adapt to
  the mesh size and uses less memory.

See the list of all [dune-uggrid 2.6 merge requests][] for minor
changes not mentioned here.

  [!55]: https://gitlab.dune-project.org/staging/dune-uggrid/merge_requests/55
  [dune-grid!172]: https://gitlab.dune-project.org/core/dune-grid/merge_requests/172
  [dune-uggrid 2.6 merge requests]: https://gitlab.dune-project.org/staging/dune-uggrid/merge_requests?milestone_title=Dune+2.6.0&scope=all&state=all

# dune-uggrid 2.5.0 (2017-12-18)

* dune-uggrid is now a DUNE module that will only contain UG's grid
  manager in the future.
* Switch to CMake build system.


# UG Release 3.13.0 (01-03-2016)

* Rename macros `_2` and `_3` to `UG_DIM_2` and `UG_DIM_3` respectively,
  to avoid clashes with symbols of the same name in libc++.
* Add file `.gitlab-ci.yml` to drive the Gitlab continuous integration features
* Remove an obsolete XCode project file, and an obsolete mystery file 'conf.h'
* Fix an out-of-bounds array access reported by the gcc sanitizer



# UG Release 3.12.1 (11-05-2015)

* Fix bug in `Makefile.am` which prevented building of tarballs.



# UG Release 3.12.0 (11-05-2015)

The major change in this release is the official transformation of the
entire code base to C++.  While pretty much everybody (read: all Dune users)
has compiled UG as C++ for many years now, the code itself was still
officially C.  In the 3.12.0 release all .c files have now been renamed
to .cc, and UG will hence build as C++ by default.
Thanks to Christoph Grüninger.

* Remove code that redefines standard types on specific architectures
  (Fixes Debian build problems of dune-grid)
* Fix various problems when compiling with clang
* Fix the return type of a few methods that don't actually return anything.
* Removal of dead and redundant code
* Minor cleanup and beautifications in ddd



# UG Release 3.11.1 (2-12-2014)

This is mainly a bugfix release.  As the only behavioral change, memory
management now defaults to the operating system heap, rather than the
built-in heap.  This eases debugging and appears to speed up the code.

* Make method `CreateLine` return `void` instead of `INT`.
  This fixes a segfault when compiling with clang.
  Thanks to Carsten Gräser.
* More build system and other cleanup



# UG Release 3.11.0 (12-06-2014)

This release contains several bugfixes related to dynamic load balancing.
It also contains many important cleanup patches (kudos to Ansgar Burchardt).

* Properly set `SideVector` `VCOUNT` fields after load balancing
  (Dune FlySpray 810: https://dune-project.org/flyspray/index.php?do=details&task_id=810 )
* Bugfix: Always identify MIDNODEs to the proclist of the father edge
* Bugfix: Initialize EDIDENT for edges for 3d _and 2d_ before refinement
* Bugfix: Call the `RestrictPartitioning` method more often before adaptive refinement
* Remove the Fortran interface to the DDD library
* Remove left-overs from the Chaco load balancer which used to be contained
  in the UG source tree
* Remove lots of code related to obsolete architectures and the old build system
* Remove old, non-free version of netgen
* Constification in the DDD subsystem
* Various build-system improvements
