# MPI Remote Nexus

* [Summary](#summary)

## Summary

This describes how to use the first iteration of the Remote Nexus Class with MPI.  

The basic outline of steps needed to run the Remote Nexus Class with MPI is:
  * Install MPI.
  * Create the build directory including the option to activate MPI: 
  
      `cmake -DCMAKE_BUILD_TYPE=Debug -B cmake-build-debug -DNGEN_WITH_MPI:BOOL=ON -DNGEN_WITH_TESTS:BOOL=ON -S .`  
  
  * Unit tests for the Remote Nexus Class can then be built and run from the main directory with the following two commands:
  
      `cmake --build cmake-build-debug --target test_remote_nexus`  <br />
      
      `mpirun -np 2 ./cmake-build-debug/test/test_remote_nexus`

  * `mpirun -np 2` runs the test on 2 processors        
  

