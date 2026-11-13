/*
 @licstart  The following is the entire license notice for the JavaScript code in this file.

 The MIT License (MIT)

 Copyright (C) 1997-2020 by Dimitri van Heesch

 Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 and associated documentation files (the "Software"), to deal in the Software without restriction,
 including without limitation the rights to use, copy, modify, merge, publish, distribute,
 sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all copies or
 substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 @licend  The above is the entire license notice for the JavaScript code in this file
*/
var NAVTREE =
[
  [ "NGen", "index.html", [
    [ "Next Gen Water Modeling Framework Prototype", "index.html#autotoc_md60", [
      [ "Structural Diagrams", "index.html#autotoc_md61", null ],
      [ "Dependencies", "index.html#autotoc_md62", null ],
      [ "Installation", "index.html#autotoc_md63", null ],
      [ "Configuration", "index.html#autotoc_md64", null ],
      [ "Usage", "index.html#autotoc_md65", null ],
      [ "How to test the software", "index.html#autotoc_md66", null ],
      [ "How to debug the software", "index.html#autotoc_md67", null ],
      [ "Known issues", "index.html#autotoc_md68", null ],
      [ "Getting help", "index.html#autotoc_md69", null ],
      [ "Getting involved", "index.html#autotoc_md70", null ],
      [ "Open source licensing info", "index.html#autotoc_md72", null ],
      [ "Credits and references", "index.html#autotoc_md74", null ]
    ] ],
    [ "<tt>ngen_serialization</tt> library", "md_include_2utilities_2serialization_2_r_e_a_d_m_e.html", [
      [ "Library version", "md_include_2utilities_2serialization_2_r_e_a_d_m_e.html#autotoc_md5", null ],
      [ "Where the headers live", "md_include_2utilities_2serialization_2_r_e_a_d_m_e.html#autotoc_md6", null ],
      [ "Companion documents", "md_include_2utilities_2serialization_2_r_e_a_d_m_e.html#autotoc_md7", null ],
      [ "What this library does and does not do", "md_include_2utilities_2serialization_2_r_e_a_d_m_e.html#autotoc_md9", null ],
      [ "The core types at a glance", "md_include_2utilities_2serialization_2_r_e_a_d_m_e.html#autotoc_md11", null ],
      [ "Rules every backend must satisfy", "md_include_2utilities_2serialization_2_r_e_a_d_m_e.html#autotoc_md13", [
        [ "3.1 Identity contract", "md_include_2utilities_2serialization_2_r_e_a_d_m_e.html#autotoc_md14", null ],
        [ "3.2 Error categorization", "md_include_2utilities_2serialization_2_r_e_a_d_m_e.html#autotoc_md15", null ],
        [ "3.3 Sub-handle lifetime", "md_include_2utilities_2serialization_2_r_e_a_d_m_e.html#autotoc_md16", [
          [ "Pattern A: Snapshot", "md_include_2utilities_2serialization_2_r_e_a_d_m_e.html#autotoc_md17", null ],
          [ "Pattern B: Shared-state", "md_include_2utilities_2serialization_2_r_e_a_d_m_e.html#autotoc_md18", null ],
          [ "Why both patterns coexist", "md_include_2utilities_2serialization_2_r_e_a_d_m_e.html#autotoc_md19", null ]
        ] ],
        [ "3.4 Single-in-flight Writer rule", "md_include_2utilities_2serialization_2_r_e_a_d_m_e.html#autotoc_md20", null ],
        [ "3.5 Reader scope is fixed at construction", "md_include_2utilities_2serialization_2_r_e_a_d_m_e.html#autotoc_md21", null ],
        [ "3.6 Writer commit semantics", "md_include_2utilities_2serialization_2_r_e_a_d_m_e.html#autotoc_md22", null ],
        [ "3.7 Optional Reader::close()", "md_include_2utilities_2serialization_2_r_e_a_d_m_e.html#autotoc_md23", null ],
        [ "3.8 Concurrent sub-handle access", "md_include_2utilities_2serialization_2_r_e_a_d_m_e.html#autotoc_md24", null ]
      ] ],
      [ "Recommended call patterns", "md_include_2utilities_2serialization_2_r_e_a_d_m_e.html#autotoc_md26", [
        [ "4.1 Scoped wrappers (the default)", "md_include_2utilities_2serialization_2_r_e_a_d_m_e.html#autotoc_md27", null ],
        [ "4.2 Low-level factories (the escape hatch)", "md_include_2utilities_2serialization_2_r_e_a_d_m_e.html#autotoc_md28", null ],
        [ "4.3 Error category branching", "md_include_2utilities_2serialization_2_r_e_a_d_m_e.html#autotoc_md29", null ],
        [ "4.4 Id predicates", "md_include_2utilities_2serialization_2_r_e_a_d_m_e.html#autotoc_md30", null ]
      ] ],
      [ "Writing a new RecordBackend", "md_include_2utilities_2serialization_2_r_e_a_d_m_e.html#autotoc_md32", null ]
    ] ],
    [ "CHANGELOG", "md__c_h_a_n_g_e_l_o_g.html", null ],
    [ "Github Automated Testing", "md_doc_2_a_u_t_o_m_a_t_e_d___t_e_s_t.html", [
      [ "What Are Automated Tests and How to Start Them", "md_doc_2_a_u_t_o_m_a_t_e_d___t_e_s_t.html#autotoc_md83", null ],
      [ "What Code Tests Are Performed", "md_doc_2_a_u_t_o_m_a_t_e_d___t_e_s_t.html#autotoc_md84", null ],
      [ "Local Testing", "md_doc_2_a_u_t_o_m_a_t_e_d___t_e_s_t.html#autotoc_md85", null ],
      [ "What to Do When a Test Fail", "md_doc_2_a_u_t_o_m_a_t_e_d___t_e_s_t.html#autotoc_md86", null ]
    ] ],
    [ "BMI External Models", "md_doc_2_b_m_i___m_o_d_e_l_s.html", [
      [ "Summary", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md88", null ],
      [ "Formulation Config", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md89", [
        [ "Required Parameters", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md90", null ],
        [ "Semi-Optional Parameters", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md92", null ],
        [ "Optional Parameters", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md93", null ]
      ] ],
      [ "BMI Models Written in C", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md94", [
        [ "BMI C Model As Shared Library", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md95", [
          [ "Dynamic Loading", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md96", null ]
        ] ],
        [ "BMI C CFE Example", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md97", null ],
        [ "BMI C Caveats", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md98", [
          [ "BMI C Activate/Deactivation Required in CMake Build", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md99", null ],
          [ "Additional Bootstrapping Function Needed", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md100", [
            [ "Why?", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md101", null ]
          ] ]
        ] ]
      ] ],
      [ "BMI Models Written in C++", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md102", [
        [ "BMI C++ Model As Shared Library", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md103", [
          [ "Dynamic Loading", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md104", null ],
          [ "Additional Bootstrapping Functions Needed", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md105", [
            [ "Why?", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md106", null ]
          ] ]
        ] ],
        [ "BMI C++ Example", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md107", null ]
      ] ],
      [ "BMI Models Written in Python", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md108", [
        [ "Enabling Python Integration", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md109", null ],
        [ "BMI Python Model as Package Class", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md110", null ],
        [ "BMI Python Example", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md111", null ]
      ] ],
      [ "BMI Models Written in Fortran", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md112", [
        [ "Enabling Fortran Integration", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md113", null ],
        [ "ISO C Binding Middleware", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md114", null ],
        [ "A Compiled Shared Library", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md115", [
          [ "Required Additional Fortran Registration Function", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md116", null ]
        ] ]
      ] ],
      [ "Multi-Module BMI Formulations", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md117", [
        [ "Passing Variables Between Nested Formulations", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md118", [
          [ "How Values Are Orchestrated", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md119", null ],
          [ "Look-back and Default Values", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md120", [
            [ "Example Look-Back Config", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md121", null ]
          ] ]
        ] ]
      ] ]
    ] ],
    [ "ngen BMI Serialization Protocol", "md_doc_2_b_m_i___s_e_r_i_a_l_i_z_a_t_i_o_n___p_r_o_t_o_c_o_l.html", [
      [ "TODO or Not TODO", "md_doc_2_b_m_i___s_e_r_i_a_l_i_z_a_t_i_o_n___p_r_o_t_o_c_o_l.html#autotoc_md123", [
        [ "Conformance is use-case dependent", "md_doc_2_b_m_i___s_e_r_i_a_l_i_z_a_t_i_o_n___p_r_o_t_o_c_o_l.html#autotoc_md124", null ]
      ] ],
      [ "Reserved BMI Variables", "md_doc_2_b_m_i___s_e_r_i_a_l_i_z_a_t_i_o_n___p_r_o_t_o_c_o_l.html#autotoc_md126", [
        [ "Item list behavior", "md_doc_2_b_m_i___s_e_r_i_a_l_i_z_a_t_i_o_n___p_r_o_t_o_c_o_l.html#autotoc_md127", null ]
      ] ],
      [ "Unit Conventions for Non-Physical Variables", "md_doc_2_b_m_i___s_e_r_i_a_l_i_z_a_t_i_o_n___p_r_o_t_o_c_o_l.html#autotoc_md129", null ],
      [ "Model Developer Implementation Guide", "md_doc_2_b_m_i___s_e_r_i_a_l_i_z_a_t_i_o_n___p_r_o_t_o_c_o_l.html#autotoc_md131", [
        [ "Width of the size variable", "md_doc_2_b_m_i___s_e_r_i_a_l_i_z_a_t_i_o_n___p_r_o_t_o_c_o_l.html#autotoc_md132", null ],
        [ "Recommended: defensive payload format", "md_doc_2_b_m_i___s_e_r_i_a_l_i_z_a_t_i_o_n___p_r_o_t_o_c_o_l.html#autotoc_md133", null ],
        [ "Constraints and non-goals", "md_doc_2_b_m_i___s_e_r_i_a_l_i_z_a_t_i_o_n___p_r_o_t_o_c_o_l.html#autotoc_md134", null ],
        [ "Optional: GetValuePtr for zero-copy state access", "md_doc_2_b_m_i___s_e_r_i_a_l_i_z_a_t_i_o_n___p_r_o_t_o_c_o_l.html#autotoc_md135", null ],
        [ "Recommended: single field list drives save, restore, and size", "md_doc_2_b_m_i___s_e_r_i_a_l_i_z_a_t_i_o_n___p_r_o_t_o_c_o_l.html#autotoc_md136", null ],
        [ "Reference implementations", "md_doc_2_b_m_i___s_e_r_i_a_l_i_z_a_t_i_o_n___p_r_o_t_o_c_o_l.html#autotoc_md137", null ]
      ] ]
    ] ],
    [ "BMI Conventions", "md_doc_2_b_m_iconventions.html", [
      [ "Compliance with the BMI v2.0 Standard", "md_doc_2_b_m_iconventions.html#autotoc_md139", null ],
      [ "Time Control", "md_doc_2_b_m_iconventions.html#autotoc_md140", [
        [ "Set and Get Values Per Timestep", "md_doc_2_b_m_iconventions.html#autotoc_md141", null ],
        [ "update vs. update_until", "md_doc_2_b_m_iconventions.html#autotoc_md142", null ],
        [ "Time Units", "md_doc_2_b_m_iconventions.html#autotoc_md143", null ],
        [ "Model time begin", "md_doc_2_b_m_iconventions.html#autotoc_md144", null ],
        [ "Model time end", "md_doc_2_b_m_iconventions.html#autotoc_md145", null ]
      ] ],
      [ "Metadata", "md_doc_2_b_m_iconventions.html#autotoc_md146", [
        [ "Variable Units", "md_doc_2_b_m_iconventions.html#autotoc_md147", null ],
        [ "Variable Types and Sizes", "md_doc_2_b_m_iconventions.html#autotoc_md148", null ]
      ] ],
      [ "Input and output published variables", "md_doc_2_b_m_iconventions.html#autotoc_md149", [
        [ "Input/output variables and the time loop", "md_doc_2_b_m_iconventions.html#autotoc_md150", null ],
        [ "Validation of formulations for input variables", "md_doc_2_b_m_iconventions.html#autotoc_md151", null ],
        [ "Validation of formulations for output variables", "md_doc_2_b_m_iconventions.html#autotoc_md152", null ],
        [ "Calling set_value on output variables", "md_doc_2_b_m_iconventions.html#autotoc_md153", null ],
        [ "Calling get_value on input variables", "md_doc_2_b_m_iconventions.html#autotoc_md154", null ],
        [ "Input and output with the same name not permitted", "md_doc_2_b_m_iconventions.html#autotoc_md155", null ]
      ] ],
      [ "Unpublished variables", "md_doc_2_b_m_iconventions.html#autotoc_md156", [
        [ "Unknown/unexpected variables passed to set_value", "md_doc_2_b_m_iconventions.html#autotoc_md157", null ],
        [ "Must support variable metadata functions", "md_doc_2_b_m_iconventions.html#autotoc_md158", null ]
      ] ],
      [ "Array representation", "md_doc_2_b_m_iconventions.html#autotoc_md159", [
        [ "Contiguousness", "md_doc_2_b_m_iconventions.html#autotoc_md160", null ],
        [ "Zero-Indexing", "md_doc_2_b_m_iconventions.html#autotoc_md161", null ],
        [ "Layout and Flattening", "md_doc_2_b_m_iconventions.html#autotoc_md162", [
          [ "C ordering required", "md_doc_2_b_m_iconventions.html#autotoc_md163", null ],
          [ "Proper ordering example", "md_doc_2_b_m_iconventions.html#autotoc_md164", null ],
          [ "When this matters", "md_doc_2_b_m_iconventions.html#autotoc_md165", [
            [ "Fortran models using z,y,x index ordering", "md_doc_2_b_m_iconventions.html#autotoc_md166", null ],
            [ "Clarification for get_value_at_indices and set_value_at_indices", "md_doc_2_b_m_iconventions.html#autotoc_md167", null ],
            [ "Spatial data arrays for structured grids", "md_doc_2_b_m_iconventions.html#autotoc_md168", null ]
          ] ]
        ] ]
      ] ],
      [ "Grid metadata functions", "md_doc_2_b_m_iconventions.html#autotoc_md169", [
        [ "Required metadata functions", "md_doc_2_b_m_iconventions.html#autotoc_md170", null ],
        [ "Ranks and sizes for scalar values", "md_doc_2_b_m_iconventions.html#autotoc_md171", null ]
      ] ],
      [ "Specific ngen uses of unpublished variables", "md_doc_2_b_m_iconventions.html#autotoc_md172", [
        [ "The model_params list", "md_doc_2_b_m_iconventions.html#autotoc_md173", null ],
        [ "Grid sizing parameters", "md_doc_2_b_m_iconventions.html#autotoc_md174", [
          [ "ngen_grid_N_shape", "md_doc_2_b_m_iconventions.html#autotoc_md175", null ],
          [ "ngen_grid_N_spacing", "md_doc_2_b_m_iconventions.html#autotoc_md176", null ],
          [ "ngen_grid_N_origin", "md_doc_2_b_m_iconventions.html#autotoc_md177", null ]
        ] ]
      ] ]
    ] ],
    [ "Project Builds", "md_doc_2_b_u_i_l_d_s___a_n_d___c_m_a_k_e.html", [
      [ "The TL;DR", "md_doc_2_b_u_i_l_d_s___a_n_d___c_m_a_k_e.html#autotoc_md179", null ],
      [ "Generating a Build System", "md_doc_2_b_u_i_l_d_s___a_n_d___c_m_a_k_e.html#autotoc_md180", [
        [ "Out-of-Source Building", "md_doc_2_b_u_i_l_d_s___a_n_d___c_m_a_k_e.html#autotoc_md181", null ],
        [ "Ignoring the Build Directory", "md_doc_2_b_u_i_l_d_s___a_n_d___c_m_a_k_e.html#autotoc_md182", null ],
        [ "Generation Commands", "md_doc_2_b_u_i_l_d_s___a_n_d___c_m_a_k_e.html#autotoc_md183", [
          [ "Generating from Build Directory", "md_doc_2_b_u_i_l_d_s___a_n_d___c_m_a_k_e.html#autotoc_md184", null ],
          [ "Generating from Project Root", "md_doc_2_b_u_i_l_d_s___a_n_d___c_m_a_k_e.html#autotoc_md185", null ],
          [ "Optional: Specifying Project Build Config Type", "md_doc_2_b_u_i_l_d_s___a_n_d___c_m_a_k_e.html#autotoc_md186", null ]
        ] ],
        [ "Regenerating", "md_doc_2_b_u_i_l_d_s___a_n_d___c_m_a_k_e.html#autotoc_md187", null ]
      ] ],
      [ "Building Targets", "md_doc_2_b_u_i_l_d_s___a_n_d___c_m_a_k_e.html#autotoc_md188", [
        [ "Building From Build Directory", "md_doc_2_b_u_i_l_d_s___a_n_d___c_m_a_k_e.html#autotoc_md189", null ],
        [ "Building From Source Directory", "md_doc_2_b_u_i_l_d_s___a_n_d___c_m_a_k_e.html#autotoc_md190", null ]
      ] ],
      [ "Dependencies", "md_doc_2_b_u_i_l_d_s___a_n_d___c_m_a_k_e.html#autotoc_md191", [
        [ "Build System Regeneration", "md_doc_2_b_u_i_l_d_s___a_n_d___c_m_a_k_e.html#autotoc_md192", null ],
        [ "Boost ENV Variable", "md_doc_2_b_u_i_l_d_s___a_n_d___c_m_a_k_e.html#autotoc_md193", null ]
      ] ],
      [ "Build Design", "md_doc_2_b_u_i_l_d_s___a_n_d___c_m_a_k_e.html#autotoc_md194", [
        [ "Library-Based Structure", "md_doc_2_b_u_i_l_d_s___a_n_d___c_m_a_k_e.html#autotoc_md195", null ],
        [ "Handling Changes", "md_doc_2_b_u_i_l_d_s___a_n_d___c_m_a_k_e.html#autotoc_md196", [
          [ "Adding New Subdirectories", "md_doc_2_b_u_i_l_d_s___a_n_d___c_m_a_k_e.html#autotoc_md197", null ]
        ] ]
      ] ]
    ] ],
    [ "Project Dependencies", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html", [
      [ "Summary", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md199", null ],
      [ "Details", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md200", [
        [ "Google Test", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md201", [
          [ "Setup", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md202", null ],
          [ "Version Requirements", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md203", null ]
        ] ],
        [ "C and C++ Compiler", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md204", [
          [ "Setup", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md205", null ],
          [ "Version Requirements", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md206", [
            [ "GCC", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md207", null ],
            [ "Clang", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md208", null ]
          ] ]
        ] ],
        [ "CMake", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md209", [
          [ "Setup", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md210", null ],
          [ "Version Requirements", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md211", null ]
        ] ],
        [ "Boost (Headers Only)", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md212", [
          [ "Setup", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md213", [
            [ "Setting BOOST_ROOT", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md214", null ]
          ] ],
          [ "Version Requirements", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md215", null ]
        ] ],
        [ "Udunits", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md216", [
          [ "Version Requirements", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md219", null ]
        ] ],
        [ "Python 3 Libraries", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md220", [
          [ "Setup", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md222", null ],
          [ "Version Requirements", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md223", null ]
        ] ],
        [ "pybind11", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md224", [
          [ "Setup", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md226", null ],
          [ "Version Requirements", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md227", null ]
        ] ],
        [ "The dmod.subsetservice Package", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md228", [
          [ "Setup", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md229", null ],
          [ "Version Requirements", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md230", null ]
        ] ],
        [ "t-route", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md231", [
          [ "Setup", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md232", null ]
        ] ],
        [ "NetCDF Libraries", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md233", [
          [ "Setup", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md234", null ],
          [ "Parallel NetCDF", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md235", null ]
        ] ]
      ] ]
    ] ],
    [ "Distributed Processing", "md_doc_2_d_i_s_t_r_i_b_u_t_e_d___p_r_o_c_e_s_s_i_n_g.html", [
      [ "Overview", "md_doc_2_d_i_s_t_r_i_b_u_t_e_d___p_r_o_c_e_s_s_i_n_g.html#autotoc_md237", null ],
      [ "Building the MPI-Enabled Nextgen Driver", "md_doc_2_d_i_s_t_r_i_b_u_t_e_d___p_r_o_c_e_s_s_i_n_g.html#autotoc_md238", null ],
      [ "Running the MPI-Enabled Nextgen Driver", "md_doc_2_d_i_s_t_r_i_b_u_t_e_d___p_r_o_c_e_s_s_i_n_g.html#autotoc_md239", [
        [ "Subdivided Hydrofabric", "md_doc_2_d_i_s_t_r_i_b_u_t_e_d___p_r_o_c_e_s_s_i_n_g.html#autotoc_md240", [
          [ "Driver Runtime Differences", "md_doc_2_d_i_s_t_r_i_b_u_t_e_d___p_r_o_c_e_s_s_i_n_g.html#autotoc_md241", null ],
          [ "File Names", "md_doc_2_d_i_s_t_r_i_b_u_t_e_d___p_r_o_c_e_s_s_i_n_g.html#autotoc_md242", null ],
          [ "On-the-fly Generation", "md_doc_2_d_i_s_t_r_i_b_u_t_e_d___p_r_o_c_e_s_s_i_n_g.html#autotoc_md243", null ]
        ] ],
        [ "Examples", "md_doc_2_d_i_s_t_r_i_b_u_t_e_d___p_r_o_c_e_s_s_i_n_g.html#autotoc_md244", [
          [ "Example 1 - Full Hydrofabric", "md_doc_2_d_i_s_t_r_i_b_u_t_e_d___p_r_o_c_e_s_s_i_n_g.html#autotoc_md245", null ],
          [ "Example 2 - Subdivided Hydrofabric", "md_doc_2_d_i_s_t_r_i_b_u_t_e_d___p_r_o_c_e_s_s_i_n_g.html#autotoc_md246", null ]
        ] ]
      ] ],
      [ "Partitioning Config Generator", "md_doc_2_d_i_s_t_r_i_b_u_t_e_d___p_r_o_c_e_s_s_i_n_g.html#autotoc_md247", null ]
    ] ],
    [ "Git Strategy", "md_doc_2_g_i_t___u_s_a_g_e.html", [
      [ "Branching Model", "md_doc_2_g_i_t___u_s_a_g_e.html#autotoc_md249", [
        [ "Feature Branches from master", "md_doc_2_g_i_t___u_s_a_g_e.html#autotoc_md250", null ],
        [ "Relating production, master, and Release Branches", "md_doc_2_g_i_t___u_s_a_g_e.html#autotoc_md251", null ]
      ] ],
      [ "Contributing", "md_doc_2_g_i_t___u_s_a_g_e.html#autotoc_md252", null ],
      [ "Optional: Setting Up Hook Scripts", "md_doc_2_g_i_t___u_s_a_g_e.html#autotoc_md253", null ]
    ] ],
    [ "Definitions", "md_doc_2glossary.html", [
      [ "References", "md_doc_2glossary.html#autotoc_md255", null ]
    ] ],
    [ "MPI Remote Nexus", "md_doc_2_m_p_i___r_e_m_o_t_e___n_e_x_u_s.html", [
      [ "Summary", "md_doc_2_m_p_i___r_e_m_o_t_e___n_e_x_u_s.html#autotoc_md257", null ]
    ] ],
    [ "NextGen on CONUS", "md_doc_2_next_gen___o_n___c_o_n_u_s.html", [
      [ "Summary", "md_doc_2_next_gen___o_n___c_o_n_u_s.html#autotoc_md259", null ],
      [ "Download the Codes", "md_doc_2_next_gen___o_n___c_o_n_u_s.html#autotoc_md260", null ],
      [ "Setting up the Environment", "md_doc_2_next_gen___o_n___c_o_n_u_s.html#autotoc_md261", null ],
      [ "Build the Executable", "md_doc_2_next_gen___o_n___c_o_n_u_s.html#autotoc_md262", null ],
      [ "CONUS Hydrofabric", "md_doc_2_next_gen___o_n___c_o_n_u_s.html#autotoc_md263", null ],
      [ "Generate Partition For Parallel Computation", "md_doc_2_next_gen___o_n___c_o_n_u_s.html#autotoc_md264", null ],
      [ "Prepare the Input Data", "md_doc_2_next_gen___o_n___c_o_n_u_s.html#autotoc_md265", null ],
      [ "Build the Realization Configurations", "md_doc_2_next_gen___o_n___c_o_n_u_s.html#autotoc_md266", null ],
      [ "Run Computations with Submodules", "md_doc_2_next_gen___o_n___c_o_n_u_s.html#autotoc_md267", null ],
      [ "Resource Usage", "md_doc_2_next_gen___o_n___c_o_n_u_s.html#autotoc_md268", null ],
      [ "Run Computation with Topmodel", "md_doc_2_next_gen___o_n___c_o_n_u_s.html#autotoc_md269", null ],
      [ "Run Computation with Routing", "md_doc_2_next_gen___o_n___c_o_n_u_s.html#autotoc_md270", null ]
    ] ],
    [ "ngen Serialization", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html", [
      [ "Engine behavior with non-conforming models", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md273", null ],
      [ "Wire Format", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md275", [
        [ "Two timestamps, not one", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md276", null ],
        [ "Wire-format version", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md277", null ],
        [ "Key properties", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md278", null ]
      ] ],
      [ "Save Protocol: NgenSerializationProtocol", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md280", [
        [ "Configuration", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md281", null ],
        [ "Support detection", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md282", null ],
        [ "Operation", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md283", null ],
        [ "Size variable width — how the engine composes with narrower models", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md284", null ]
      ] ],
      [ "Restore Protocol: NgenDeserializationProtocol", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md286", [
        [ "Configuration", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md287", null ],
        [ "Split save/restore paths", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md288", null ],
        [ "Support detection", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md289", null ],
        [ "Operation", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md290", null ]
      ] ],
      [ "Combined save + restore example", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md292", null ],
      [ "Restart semantics: hot start and resume", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md294", [
        [ "Hot start", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md295", null ],
        [ "Resume", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md296", null ],
        [ "Guaranteed consistent restore", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md297", null ],
        [ "Modules with no state", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md298", null ],
        [ "\"latest\"", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md299", null ],
        [ "Not supported yet", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md300", null ]
      ] ],
      [ "Identity Convention: compound_id()", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md302", [
        [ "How the compound gets set", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md303", null ]
      ] ],
      [ "Engine Integration", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md305", [
        [ "Save — per-timestep from Layer", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md306", null ],
        [ "Restore — once at the end of create_model", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md307", null ],
        [ "Lifecycle summary", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md308", null ]
      ] ],
      [ "Worked Example — multi-BMI formulation with save + restore", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md310", null ],
      [ "Realization-Level (Global) Configuration", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md312", [
        [ "Top-level placement", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md313", null ],
        [ "Precedence", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md314", null ],
        [ "Absent block", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md315", null ],
        [ "Path templating", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md316", null ]
      ] ],
      [ "Scaling Considerations", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md318", [
        [ "On-disk format", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md319", null ],
        [ "Save cost model", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md320", null ],
        [ "Restore cost model", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md321", null ],
        [ "id_subset for memory-bounded restores", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md322", null ],
        [ "Thread and process safety", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md323", null ],
        [ "When the protocol is not the right tool", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md324", null ]
      ] ]
    ] ],
    [ "Precision guidelines for validation of NGEN code translation and upgrades", "md_doc_2_precision___guidelines__for___ngen___code___validation.html", null ],
    [ "Standards for Programming", "md_doc_2programming__standards.html", [
      [ "Source Documentation", "md_doc_2programming__standards.html#autotoc_md327", null ],
      [ "Naming", "md_doc_2programming__standards.html#autotoc_md328", null ],
      [ "Other", "md_doc_2programming__standards.html#autotoc_md331", [
        [ "Headers", "md_doc_2programming__standards.html#autotoc_md332", null ],
        [ "Spaces vs Tabs", "md_doc_2programming__standards.html#autotoc_md333", null ],
        [ "Braces", "md_doc_2programming__standards.html#autotoc_md334", null ],
        [ "Line Width", "md_doc_2programming__standards.html#autotoc_md335", null ]
      ] ]
    ] ],
    [ "Python Routing", "md_doc_2_p_y_t_h_o_n___r_o_u_t_i_n_g.html", [
      [ "Summary", "md_doc_2_p_y_t_h_o_n___r_o_u_t_i_n_g.html#autotoc_md337", null ],
      [ "Setup Virtual Environment", "md_doc_2_p_y_t_h_o_n___r_o_u_t_i_n_g.html#autotoc_md338", null ],
      [ "Install t-route", "md_doc_2_p_y_t_h_o_n___r_o_u_t_i_n_g.html#autotoc_md339", null ],
      [ "Installation Caveats", "md_doc_2_p_y_t_h_o_n___r_o_u_t_i_n_g.html#autotoc_md340", [
        [ "Compilers and Libraries", "md_doc_2_p_y_t_h_o_n___r_o_u_t_i_n_g.html#autotoc_md341", null ],
        [ "Default installation is in development mode (impacts macOS)", "md_doc_2_p_y_t_h_o_n___r_o_u_t_i_n_g.html#autotoc_md342", null ],
        [ "Some tips on installation if you run into issues", "md_doc_2_p_y_t_h_o_n___r_o_u_t_i_n_g.html#autotoc_md343", null ]
      ] ],
      [ "Using t-route with ngen", "md_doc_2_p_y_t_h_o_n___r_o_u_t_i_n_g.html#autotoc_md344", [
        [ "Realization Config", "md_doc_2_p_y_t_h_o_n___r_o_u_t_i_n_g.html#autotoc_md345", null ],
        [ "Routing Config", "md_doc_2_p_y_t_h_o_n___r_o_u_t_i_n_g.html#autotoc_md346", null ],
        [ "Configuration considerations for t-route with ngen", "md_doc_2_p_y_t_h_o_n___r_o_u_t_i_n_g.html#autotoc_md347", null ]
      ] ],
      [ "Running t-route separately with ngen output", "md_doc_2_p_y_t_h_o_n___r_o_u_t_i_n_g.html#autotoc_md348", null ],
      [ "Known issues", "md_doc_2_p_y_t_h_o_n___r_o_u_t_i_n_g.html#autotoc_md349", [
        [ "Cleanup of *.parquet files required", "md_doc_2_p_y_t_h_o_n___r_o_u_t_i_n_g.html#autotoc_md350", null ],
        [ "Bug in multiprocessing on macOS", "md_doc_2_p_y_t_h_o_n___r_o_u_t_i_n_g.html#autotoc_md351", null ]
      ] ]
    ] ],
    [ "Realization Configuration", "md_doc_2_r_e_a_l_i_z_a_t_i_o_n___c_o_n_f_i_g_u_r_a_t_i_o_n.html", [
      [ "Description and Top-Level Structure", "md_doc_2_r_e_a_l_i_z_a_t_i_o_n___c_o_n_f_i_g_u_r_a_t_i_o_n.html#autotoc_md353", [
        [ "Required Top-Level Keys", "md_doc_2_r_e_a_l_i_z_a_t_i_o_n___c_o_n_f_i_g_u_r_a_t_i_o_n.html#autotoc_md354", null ],
        [ "Optional Top-Level Keys", "md_doc_2_r_e_a_l_i_z_a_t_i_o_n___c_o_n_f_i_g_u_r_a_t_i_o_n.html#autotoc_md355", [
          [ "output", "md_doc_2_r_e_a_l_i_z_a_t_i_o_n___c_o_n_f_i_g_u_r_a_t_i_o_n.html#autotoc_md356", null ],
          [ "output_root", "md_doc_2_r_e_a_l_i_z_a_t_i_o_n___c_o_n_f_i_g_u_r_a_t_i_o_n.html#autotoc_md357", null ],
          [ "disable_catchment_output", "md_doc_2_r_e_a_l_i_z_a_t_i_o_n___c_o_n_f_i_g_u_r_a_t_i_o_n.html#autotoc_md358", null ],
          [ "per_formulation_nexus_files", "md_doc_2_r_e_a_l_i_z_a_t_i_o_n___c_o_n_f_i_g_u_r_a_t_i_o_n.html#autotoc_md359", null ],
          [ "catchments", "md_doc_2_r_e_a_l_i_z_a_t_i_o_n___c_o_n_f_i_g_u_r_a_t_i_o_n.html#autotoc_md360", null ],
          [ "routing", "md_doc_2_r_e_a_l_i_z_a_t_i_o_n___c_o_n_f_i_g_u_r_a_t_i_o_n.html#autotoc_md361", null ]
        ] ],
        [ "Examples of Top-Level Structure", "md_doc_2_r_e_a_l_i_z_a_t_i_o_n___c_o_n_f_i_g_u_r_a_t_i_o_n.html#autotoc_md362", null ]
      ] ],
      [ "The Global Section", "md_doc_2_r_e_a_l_i_z_a_t_i_o_n___c_o_n_f_i_g_u_r_a_t_i_o_n.html#autotoc_md363", null ],
      [ "The Time Section", "md_doc_2_r_e_a_l_i_z_a_t_i_o_n___c_o_n_f_i_g_u_r_a_t_i_o_n.html#autotoc_md364", null ],
      [ "Individual Catchments", "md_doc_2_r_e_a_l_i_z_a_t_i_o_n___c_o_n_f_i_g_u_r_a_t_i_o_n.html#autotoc_md365", null ],
      [ "The Routing Section", "md_doc_2_r_e_a_l_i_z_a_t_i_o_n___c_o_n_f_i_g_u_r_a_t_i_o_n.html#autotoc_md366", null ],
      [ "A Full Example", "md_doc_2_r_e_a_l_i_z_a_t_i_o_n___c_o_n_f_i_g_u_r_a_t_i_o_n.html#autotoc_md367", null ],
      [ "A Note on BMI Models", "md_doc_2_r_e_a_l_i_z_a_t_i_o_n___c_o_n_f_i_g_u_r_a_t_i_o_n.html#autotoc_md368", null ]
    ] ],
    [ "<a href=\"https://docs.opengeospatial.org/is/14-111r6/14-111r6.html\" >HY_Features</a>", "md_doc_2references.html", [
      [ "BMI", "md_doc_2references.html#autotoc_md370", null ],
      [ "Formulations", "md_doc_2references.html#autotoc_md371", null ],
      [ "HYMOD", "md_doc_2references.html#autotoc_md372", null ],
      [ "T-shirt", "md_doc_2references.html#autotoc_md373", null ],
      [ "GIUH", "md_doc_2references.html#autotoc_md374", [
        [ "Modeling References", "md_doc_2references.html#autotoc_md375", null ]
      ] ]
    ] ],
    [ "Release Management", "md_doc_2_r_e_l_e_a_s_e___m_a_n_a_g_e_m_e_n_t.html", [
      [ "The Release Process", "md_doc_2_r_e_l_e_a_s_e___m_a_n_a_g_e_m_e_n_t.html#autotoc_md377", [
        [ "TL;DR", "md_doc_2_r_e_l_e_a_s_e___m_a_n_a_g_e_m_e_n_t.html#autotoc_md378", null ],
        [ "Process Steps", "md_doc_2_r_e_l_e_a_s_e___m_a_n_a_g_e_m_e_n_t.html#autotoc_md379", null ]
      ] ]
    ] ],
    [ "Installation instructions", "md__i_n_s_t_a_l_l.html", [
      [ "Building and running with docker:", "md__i_n_s_t_a_l_l.html#autotoc_md382", null ],
      [ "Building manually:", "md__i_n_s_t_a_l_l.html#autotoc_md383", null ]
    ] ],
    [ "Guidance on How to Contribute", "md__c_o_n_t_r_i_b_u_t_i_n_g.html", [
      [ "Using the Issue Tracker", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md386", null ],
      [ "Changing the Code-Base", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md387", [
        [ "Summary", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md388", null ],
        [ "Getting Started", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md389", [
          [ "Optional: Git Hooks", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md390", null ]
        ] ],
        [ "Developing Changes", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md391", [
          [ "Work in a Dedicated Branch", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md392", null ],
          [ "Pushing Incremental Commits", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md393", null ],
          [ "Submitting Pull Requests", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md394", [
            [ "Guidelines for Pull Requests", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md395", null ],
            [ "PR Review and Requested Revisions", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md396", null ]
          ] ]
        ] ],
        [ "Keeping Forks Up to Date", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md397", [
          [ "A Rebase Strategy", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md398", null ],
          [ "Getting Remote Changes", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md399", [
            [ "For production Too", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md400", null ]
          ] ],
          [ "Rebasing Development Branches", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md401", [
            [ "Interactive Rebasing", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md402", null ]
          ] ],
          [ "Fixing Diverging Development Branches", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md403", null ]
        ] ]
      ] ]
    ] ],
    [ "Disclaimer", "md__t_e_r_m_s.html", [
      [ "Exceptions", "md__t_e_r_m_s.html#autotoc_md406", null ]
    ] ],
    [ "Testing", "md_test_2_r_e_a_d_m_e.html", [
      [ "Testing Frameworks", "md_test_2_r_e_a_d_m_e.html#autotoc_md408", [
        [ "Google Test", "md_test_2_r_e_a_d_m_e.html#autotoc_md409", [
          [ "(Re)generating CMake Project Buildsystem", "md_test_2_r_e_a_d_m_e.html#autotoc_md410", null ]
        ] ]
      ] ],
      [ "Executing Automated Tests", "md_test_2_r_e_a_d_m_e.html#autotoc_md411", [
        [ "C++ Tests", "md_test_2_r_e_a_d_m_e.html#autotoc_md412", [
          [ "Test Targets and Executables", "md_test_2_r_e_a_d_m_e.html#autotoc_md413", null ]
        ] ]
      ] ],
      [ "Creating New Automated Tests", "md_test_2_r_e_a_d_m_e.html#autotoc_md414", [
        [ "Adding Tests to CMake Builds", "md_test_2_r_e_a_d_m_e.html#autotoc_md415", null ],
        [ "Test Creation Rules of Thumb", "md_test_2_r_e_a_d_m_e.html#autotoc_md416", null ]
      ] ]
    ] ],
    [ "Todo List", "todo.html", null ],
    [ "Namespaces", "namespaces.html", [
      [ "Namespace List", "namespaces.html", "namespaces_dup" ],
      [ "Namespace Members", "namespacemembers.html", [
        [ "All", "namespacemembers.html", "namespacemembers_dup" ],
        [ "Functions", "namespacemembers_func.html", null ],
        [ "Variables", "namespacemembers_vars.html", null ],
        [ "Typedefs", "namespacemembers_type.html", null ],
        [ "Enumerations", "namespacemembers_enum.html", null ],
        [ "Enumerator", "namespacemembers_eval.html", null ]
      ] ]
    ] ],
    [ "Classes", "annotated.html", [
      [ "Class List", "annotated.html", "annotated_dup" ],
      [ "Class Index", "classes.html", null ],
      [ "Class Hierarchy", "hierarchy.html", "hierarchy" ],
      [ "Class Members", "functions.html", [
        [ "All", "functions.html", "functions_dup" ],
        [ "Functions", "functions_func.html", "functions_func" ],
        [ "Variables", "functions_vars.html", "functions_vars" ],
        [ "Typedefs", "functions_type.html", null ],
        [ "Enumerations", "functions_enum.html", null ],
        [ "Enumerator", "functions_eval.html", null ],
        [ "Related Symbols", "functions_rela.html", null ]
      ] ]
    ] ]
  ] ]
];

var NAVTREEINDEX =
[
"annotated.html",
"class_h_y___dendritic_catchment.html#a3a03fa4da4dd40512e8893f945809e0b",
"class_ngen_simulation.html#a92db5bb0d32bf8ca3e65fb50c1203283",
"classdata__access_1_1_data_provider.html#a676578fae3696947cfa0269a2135a338",
"classgeojson_1_1_collection_feature.html#a73fbfffd228851000ceb117620a575c8",
"classgeojson_1_1_feature_collection.html#a82e227fca8517d1475daa27da1dfe7f9",
"classgeojson_1_1_line_string_feature.html#ad821415b00bf3b84764ad4c39e978d6d",
"classgeojson_1_1_multi_point_feature.html#adbebd1a45b3feeeb6d2d1d49b9027709",
"classgeojson_1_1_point_feature.html#ae37036f3abfe71ca484d4f6885624020",
"classgeojson_1_1_sentinel_feature.html#ae7fa0192036f7406ebb0af6715d6c3b4",
"classmodels_1_1bmi_1_1_bmi___adapter.html#a20a614920ee1fda51670923736670a83",
"classmodels_1_1bmi_1_1_bmi___c___adapter.html#ad3cc9355829342868c29f7f565f7a639",
"classmodels_1_1bmi_1_1protocols_1_1_ngen_deserialization_protocol.html#a56b9df3eac23cc22710398fb23af45ac",
"classngen_1_1_layer.html#a1d1dbf9050ac23f56ac06e9e3dffc176",
"classngen_1_1serialization_1_1_file_backend.html",
"classrealization_1_1_bmi___c___formulation.html#a5c9d1261dbbaf14773579ca59d0d0a69",
"classrealization_1_1_bmi___cpp___formulation.html#a9265b07222e97669c28fc24454ad7350",
"classrealization_1_1_bmi___module___formulation.html#a1f79424f52607974a649761eec144acf",
"classrealization_1_1_bmi___multi___formulation.html#a54b472f0a470ee51ef336c24c879e55a",
"classrealization_1_1_formulation___manager.html#a6678c68c2fdca087cd1efb7532bd80d2",
"index.html#autotoc_md69",
"md_doc_2_next_gen___o_n___c_o_n_u_s.html#autotoc_md270",
"namespacemembers_enum.html",
"namespacenonstd.html",
"structhy__features_1_1attributes_1_1area__from__geometry__visitor.html#ae8ffe9ba9dc13c3b5dc869bd8a4cb4d8",
"structngen_1_1serialization_1_1detail_1_1_record_t.html#ad1539d8050906bf18c139e014ac55c3c",
"structutils_1_1_output_field.html#a2bbb68c85a6b472645f2d522e3abbccb"
];

var SYNCONMSG = 'click to disable panel synchronisation';
var SYNCOFFMSG = 'click to enable panel synchronisation';