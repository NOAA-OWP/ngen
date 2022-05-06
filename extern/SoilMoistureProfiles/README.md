# SMP Submodule

## About

This directory wraps the *smp* Git submodule repo, which contains a clone of the repo for the OWP SMP module library that implements BMI.  From here, the shared library file for the SMP module can be built for use in NGen.  This is configured with the [CMakeLists.txt](CMakeLists.txt) and other files in this outer directory.

#### Extra Outer Directory

Currently there are two directory layers beneath the top-level *extern/* directory. While this module has its CMakeLists.txt within the submodule, at present this extra directory exists primarily to be consistent with other modules which have their CMakeLists.txt files in the interstitial directory (this may change in the future).

## Working with the Submodule

Some simple explanations of several command actions are included below.  To better understand what these things are doing, consult the [Git Submodule documentation](https://git-scm.com/book/en/v2/Git-Tools-Submodules). 

### Getting the Latest Changes

There are two steps to getting upstream submodule changes fully 
  1. fetching and locally checking out the changes from the remote
  2. committing the new checkout revision for the submodule

To fetch and check out the latest revision (for the [currently used branch](#viewing-the-current-branch)):

    git submodule update --init -- extern/SoilMoistureProfiles

To commit the current submodule checkout revision to the NGen repo:

    git add extern/SoilMoistureProfiles
    git commit

### Viewing the Commit Hash

Git submodule configurations include the specific commit to be checked out (or an implicit default).  The current commit can be view with `git submodule status`:

    git submodule status -- extern/SoilMoistureProfiles/

This will show the **commit**, **submodule local path**, and the git description for the **commit**.  The specific configuration, including the configured branch, is set in the _.gitmodules_ file in the NGen project root.

### Changing the Commit Branch

The latest commit in the configured branch can be brought in as described here.  If it is ever necessary to change to a different branch, the following will do so:

    git config -f .gitmodules "submodule.extern/SoilMoistureProfiles/SoilMoistureProfiles.branch" <branchName>

Note that this will be done in the NGen repo configuration, so it can then be committed and push to remotes.  It is also possible to do something similar in just the local clone of a repo, by configuring `.git/config` instead of `.gitmodules`.  See the Git documentation for more on how that works if needed.

# Usage

## Building Libraries

The instructions on how to build the submodule libraries can be found in extern/SoilMoistureProfiles/INSTALL.md after the submodule has been downloaded.
