# SPRINT 1 NOTES 
2.28.2020
## Discussed at code review:

### Community development
- GitHub repo up at NOAA-OWP/ngen
- Models, Kernels and stateless defined
- Issues and PR templates being used
- Google Test, GCC, CMake adopted
- Testing .cpp are in 

### Feedback
- Some concerns about C++ but bindings for other languages are the fix
- C++ good for allowing for polymorphism and templates 
- Compile-time checking of capabilities

### TO-DOs from Feedback:

- More descriptive variable names: (double soil_m = state.storage - FS;)
- State and Flux definitions need adding
- State access to model needs documenting (how does a model 'get' state)
- Need definitions of HY_FEATURES translated or referenced in the code
- Catchment, realization and state/model relationsips should be defined 
- Need small unit --> nexus coupling outlines
- Existance of dynamic timesteps, and async calculation of state does it keep the new state or t+1 state for past state recalculation
- Parallelization of realizations and benefits/drawbacks need discussed
- Flow of one realization into another via nexus needs described (could just better describe PointHydroNexus.cpp)
- Non-Linear Reservoir PDF needs added to docs

### Topics for thought for next sprint:
- Model template, even psudocode
- Error Handling Standard 

