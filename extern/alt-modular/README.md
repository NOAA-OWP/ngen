# Alt-Modular Submodule

## About

This directory wraps the *alt-modular* Git submodule repo, which contains a number of independent model libraries each implementing BMI.  From here, shared library files for several of the *alt-modular* modules can be built for use in NGen.  These are configured with the [CMakeLists.txt](CMakeLists.txt) and other files in this outer directory.

#### Extra Outer Directory

Currently there are two directory layers beneath the top-level *extern/* directory.  This was done so that certain things used by NGen (i.e., a *CMakeLists.txt* file for building shared library files) can be placed alongside, but not within, the submodule.

## Working with the Submodule

Some simple explanations of several commands are included below.  To better understand what these things are doing, consult the [Git Submodule documentation](https://git-scm.com/book/en/v2/Git-Tools-Submodules). 

### Getting the Latest Changes

There are two steps to getting upstream submodule changes fully 
  1. fetching and locally checking out the changes from the remote
  2. committing the new checkout revision for the submodule

To fetch and check out the latest revision (for the [currently used branch](#viewing-the-current-branch)):

    git submodule update --remote extern/alt-modular/alt-modular

To commit the current submodule checkout revision to the NGen repo:

    git add extern/alt-modular/alt-modular
    git commit

### Viewing the Current Branch

The configured branch for the submodule will control exactly what revision gets checked out when new changes are retrieved, and as such may need to be viewed or [changed](#changing-the-branch).  

The submodule's status, which includes the current branch, can be viewed with `git submodule status`:

    git submodule status -- extern/alt-modular/alt-modular/

This will show the **commit**, **submodule local path**, and **submodule branch**.  It can also be run without the extra arguments to show this for all NGen repo submodules.

### Changing the Branch

To change the branch for everyone, run the following:

    git config -f .gitmodules "submodule.extern/alt-modular/alt-modular.branch" main

# Usage

## Building Libraries


## Adding New Library Configuration