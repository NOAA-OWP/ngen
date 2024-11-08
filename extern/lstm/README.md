# LSTM Submodule

## About
This submodule is linked in from: https://github.com/CIROH-UA/lstm, which is a fork of https://github.com/NOAA-OWP/lstm. This directory follows the template for linking submodules from https://github.com/NOAA-OWP/ngen/edit/master/extern/cfe/.

#### Extra Outer Directory

Currently there are two directory layers beneath the top-level *extern/* directory.  This was done so that certain things used by NGen (i.e., a *CMakeLists.txt* file for building shared library files) can be placed alongside, but not within, the submodule.

## Working with the Submodule

Some simple explanations of several command actions are included below.  To better understand what these things are doing, consult the [Git Submodule documentation](https://git-scm.com/book/en/v2/Git-Tools-Submodules).

### Getting the Latest Changes

There are two steps to getting upstream submodule changes fully 
  1. fetching and locally checking out the changes from the remote
  2. committing the new checkout revision for the submodule

To fetch and check out the latest revision (for the [currently used branch](#viewing-the-current-branch)):

    git submodule update --init -- extern/lstm/lstm

To commit the current submodule checkout revision to the CIROH UA NGen repo:

    git add extern/lstm/lstm
    git commit
