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
      [ "What Are Automated Tests and How to Start Them", "md_doc_2_a_u_t_o_m_a_t_e_d___t_e_s_t.html#autotoc_md82", null ],
      [ "What Code Tests Are Performed", "md_doc_2_a_u_t_o_m_a_t_e_d___t_e_s_t.html#autotoc_md83", null ],
      [ "Local Testing", "md_doc_2_a_u_t_o_m_a_t_e_d___t_e_s_t.html#autotoc_md84", null ],
      [ "What to Do When a Test Fail", "md_doc_2_a_u_t_o_m_a_t_e_d___t_e_s_t.html#autotoc_md85", null ]
    ] ],
    [ "BMI External Models", "md_doc_2_b_m_i___m_o_d_e_l_s.html", [
      [ "Summary", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md87", null ],
      [ "Formulation Config", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md88", [
        [ "Required Parameters", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md89", null ],
        [ "Semi-Optional Parameters", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md91", null ],
        [ "Optional Parameters", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md92", null ]
      ] ],
      [ "BMI Models Written in C", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md93", [
        [ "BMI C Model As Shared Library", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md94", [
          [ "Dynamic Loading", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md95", null ]
        ] ],
        [ "BMI C CFE Example", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md96", null ],
        [ "BMI C Caveats", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md97", [
          [ "BMI C Activate/Deactivation Required in CMake Build", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md98", null ],
          [ "Additional Bootstrapping Function Needed", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md99", [
            [ "Why?", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md100", null ]
          ] ]
        ] ]
      ] ],
      [ "BMI Models Written in C++", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md101", [
        [ "BMI C++ Model As Shared Library", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md102", [
          [ "Dynamic Loading", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md103", null ],
          [ "Additional Bootstrapping Functions Needed", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md104", [
            [ "Why?", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md105", null ]
          ] ]
        ] ],
        [ "BMI C++ Example", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md106", null ]
      ] ],
      [ "BMI Models Written in Python", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md107", [
        [ "Enabling Python Integration", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md108", null ],
        [ "BMI Python Model as Package Class", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md109", null ],
        [ "BMI Python Example", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md110", null ]
      ] ],
      [ "BMI Models Written in Fortran", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md111", [
        [ "Enabling Fortran Integration", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md112", null ],
        [ "ISO C Binding Middleware", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md113", null ],
        [ "A Compiled Shared Library", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md114", [
          [ "Required Additional Fortran Registration Function", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md115", null ]
        ] ]
      ] ],
      [ "Multi-Module BMI Formulations", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md116", [
        [ "Passing Variables Between Nested Formulations", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md117", [
          [ "How Values Are Orchestrated", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md118", null ],
          [ "Look-back and Default Values", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md119", [
            [ "Example Look-Back Config", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md120", null ]
          ] ]
        ] ]
      ] ]
    ] ],
    [ "ngen BMI Serialization Protocol", "md_doc_2_b_m_i___s_e_r_i_a_l_i_z_a_t_i_o_n___p_r_o_t_o_c_o_l.html", [
      [ "TODO or Not TODO", "md_doc_2_b_m_i___s_e_r_i_a_l_i_z_a_t_i_o_n___p_r_o_t_o_c_o_l.html#autotoc_md122", [
        [ "Conformance is use-case dependent", "md_doc_2_b_m_i___s_e_r_i_a_l_i_z_a_t_i_o_n___p_r_o_t_o_c_o_l.html#autotoc_md123", null ]
      ] ],
      [ "Reserved BMI Variables", "md_doc_2_b_m_i___s_e_r_i_a_l_i_z_a_t_i_o_n___p_r_o_t_o_c_o_l.html#autotoc_md125", [
        [ "Item list behavior", "md_doc_2_b_m_i___s_e_r_i_a_l_i_z_a_t_i_o_n___p_r_o_t_o_c_o_l.html#autotoc_md126", null ]
      ] ],
      [ "Unit Conventions for Non-Physical Variables", "md_doc_2_b_m_i___s_e_r_i_a_l_i_z_a_t_i_o_n___p_r_o_t_o_c_o_l.html#autotoc_md128", null ],
      [ "Model Developer Implementation Guide", "md_doc_2_b_m_i___s_e_r_i_a_l_i_z_a_t_i_o_n___p_r_o_t_o_c_o_l.html#autotoc_md130", [
        [ "Width of the size variable", "md_doc_2_b_m_i___s_e_r_i_a_l_i_z_a_t_i_o_n___p_r_o_t_o_c_o_l.html#autotoc_md131", null ],
        [ "Recommended: defensive payload format", "md_doc_2_b_m_i___s_e_r_i_a_l_i_z_a_t_i_o_n___p_r_o_t_o_c_o_l.html#autotoc_md132", null ],
        [ "Constraints and non-goals", "md_doc_2_b_m_i___s_e_r_i_a_l_i_z_a_t_i_o_n___p_r_o_t_o_c_o_l.html#autotoc_md133", null ],
        [ "Optional: GetValuePtr for zero-copy state access", "md_doc_2_b_m_i___s_e_r_i_a_l_i_z_a_t_i_o_n___p_r_o_t_o_c_o_l.html#autotoc_md134", null ],
        [ "Recommended: single field list drives save, restore, and size", "md_doc_2_b_m_i___s_e_r_i_a_l_i_z_a_t_i_o_n___p_r_o_t_o_c_o_l.html#autotoc_md135", null ],
        [ "Reference implementations", "md_doc_2_b_m_i___s_e_r_i_a_l_i_z_a_t_i_o_n___p_r_o_t_o_c_o_l.html#autotoc_md136", null ]
      ] ]
    ] ],
    [ "BMI Conventions", "md_doc_2_b_m_iconventions.html", [
      [ "Compliance with the BMI v2.0 Standard", "md_doc_2_b_m_iconventions.html#autotoc_md138", null ],
      [ "Time Control", "md_doc_2_b_m_iconventions.html#autotoc_md139", [
        [ "Set and Get Values Per Timestep", "md_doc_2_b_m_iconventions.html#autotoc_md140", null ],
        [ "update vs. update_until", "md_doc_2_b_m_iconventions.html#autotoc_md141", null ],
        [ "Time Units", "md_doc_2_b_m_iconventions.html#autotoc_md142", null ],
        [ "Model time begin", "md_doc_2_b_m_iconventions.html#autotoc_md143", null ],
        [ "Model time end", "md_doc_2_b_m_iconventions.html#autotoc_md144", null ]
      ] ],
      [ "Metadata", "md_doc_2_b_m_iconventions.html#autotoc_md145", [
        [ "Variable Units", "md_doc_2_b_m_iconventions.html#autotoc_md146", null ],
        [ "Variable Types and Sizes", "md_doc_2_b_m_iconventions.html#autotoc_md147", null ]
      ] ],
      [ "Input and output published variables", "md_doc_2_b_m_iconventions.html#autotoc_md148", [
        [ "Input/output variables and the time loop", "md_doc_2_b_m_iconventions.html#autotoc_md149", null ],
        [ "Validation of formulations for input variables", "md_doc_2_b_m_iconventions.html#autotoc_md150", null ],
        [ "Validation of formulations for output variables", "md_doc_2_b_m_iconventions.html#autotoc_md151", null ],
        [ "Calling set_value on output variables", "md_doc_2_b_m_iconventions.html#autotoc_md152", null ],
        [ "Calling get_value on input variables", "md_doc_2_b_m_iconventions.html#autotoc_md153", null ],
        [ "Input and output with the same name not permitted", "md_doc_2_b_m_iconventions.html#autotoc_md154", null ]
      ] ],
      [ "Unpublished variables", "md_doc_2_b_m_iconventions.html#autotoc_md155", [
        [ "Unknown/unexpected variables passed to set_value", "md_doc_2_b_m_iconventions.html#autotoc_md156", null ],
        [ "Must support variable metadata functions", "md_doc_2_b_m_iconventions.html#autotoc_md157", null ]
      ] ],
      [ "Array representation", "md_doc_2_b_m_iconventions.html#autotoc_md158", [
        [ "Contiguousness", "md_doc_2_b_m_iconventions.html#autotoc_md159", null ],
        [ "Zero-Indexing", "md_doc_2_b_m_iconventions.html#autotoc_md160", null ],
        [ "Layout and Flattening", "md_doc_2_b_m_iconventions.html#autotoc_md161", [
          [ "C ordering required", "md_doc_2_b_m_iconventions.html#autotoc_md162", null ],
          [ "Proper ordering example", "md_doc_2_b_m_iconventions.html#autotoc_md163", null ],
          [ "When this matters", "md_doc_2_b_m_iconventions.html#autotoc_md164", [
            [ "Fortran models using z,y,x index ordering", "md_doc_2_b_m_iconventions.html#autotoc_md165", null ],
            [ "Clarification for get_value_at_indices and set_value_at_indices", "md_doc_2_b_m_iconventions.html#autotoc_md166", null ],
            [ "Spatial data arrays for structured grids", "md_doc_2_b_m_iconventions.html#autotoc_md167", null ]
          ] ]
        ] ]
      ] ],
      [ "Grid metadata functions", "md_doc_2_b_m_iconventions.html#autotoc_md168", [
        [ "Required metadata functions", "md_doc_2_b_m_iconventions.html#autotoc_md169", null ],
        [ "Ranks and sizes for scalar values", "md_doc_2_b_m_iconventions.html#autotoc_md170", null ]
      ] ],
      [ "Specific ngen uses of unpublished variables", "md_doc_2_b_m_iconventions.html#autotoc_md171", [
        [ "The model_params list", "md_doc_2_b_m_iconventions.html#autotoc_md172", null ],
        [ "Grid sizing parameters", "md_doc_2_b_m_iconventions.html#autotoc_md173", [
          [ "ngen_grid_N_shape", "md_doc_2_b_m_iconventions.html#autotoc_md174", null ],
          [ "ngen_grid_N_spacing", "md_doc_2_b_m_iconventions.html#autotoc_md175", null ],
          [ "ngen_grid_N_origin", "md_doc_2_b_m_iconventions.html#autotoc_md176", null ]
        ] ]
      ] ]
    ] ],
    [ "Project Builds", "md_doc_2_b_u_i_l_d_s___a_n_d___c_m_a_k_e.html", [
      [ "The TL;DR", "md_doc_2_b_u_i_l_d_s___a_n_d___c_m_a_k_e.html#autotoc_md178", null ],
      [ "Generating a Build System", "md_doc_2_b_u_i_l_d_s___a_n_d___c_m_a_k_e.html#autotoc_md179", [
        [ "Out-of-Source Building", "md_doc_2_b_u_i_l_d_s___a_n_d___c_m_a_k_e.html#autotoc_md180", null ],
        [ "Ignoring the Build Directory", "md_doc_2_b_u_i_l_d_s___a_n_d___c_m_a_k_e.html#autotoc_md181", null ],
        [ "Generation Commands", "md_doc_2_b_u_i_l_d_s___a_n_d___c_m_a_k_e.html#autotoc_md182", [
          [ "Generating from Build Directory", "md_doc_2_b_u_i_l_d_s___a_n_d___c_m_a_k_e.html#autotoc_md183", null ],
          [ "Generating from Project Root", "md_doc_2_b_u_i_l_d_s___a_n_d___c_m_a_k_e.html#autotoc_md184", null ],
          [ "Optional: Specifying Project Build Config Type", "md_doc_2_b_u_i_l_d_s___a_n_d___c_m_a_k_e.html#autotoc_md185", null ]
        ] ],
        [ "Regenerating", "md_doc_2_b_u_i_l_d_s___a_n_d___c_m_a_k_e.html#autotoc_md186", null ]
      ] ],
      [ "Building Targets", "md_doc_2_b_u_i_l_d_s___a_n_d___c_m_a_k_e.html#autotoc_md187", [
        [ "Building From Build Directory", "md_doc_2_b_u_i_l_d_s___a_n_d___c_m_a_k_e.html#autotoc_md188", null ],
        [ "Building From Source Directory", "md_doc_2_b_u_i_l_d_s___a_n_d___c_m_a_k_e.html#autotoc_md189", null ]
      ] ],
      [ "Dependencies", "md_doc_2_b_u_i_l_d_s___a_n_d___c_m_a_k_e.html#autotoc_md190", [
        [ "Build System Regeneration", "md_doc_2_b_u_i_l_d_s___a_n_d___c_m_a_k_e.html#autotoc_md191", null ],
        [ "Boost ENV Variable", "md_doc_2_b_u_i_l_d_s___a_n_d___c_m_a_k_e.html#autotoc_md192", null ]
      ] ],
      [ "Build Design", "md_doc_2_b_u_i_l_d_s___a_n_d___c_m_a_k_e.html#autotoc_md193", [
        [ "Library-Based Structure", "md_doc_2_b_u_i_l_d_s___a_n_d___c_m_a_k_e.html#autotoc_md194", null ],
        [ "Handling Changes", "md_doc_2_b_u_i_l_d_s___a_n_d___c_m_a_k_e.html#autotoc_md195", [
          [ "Adding New Subdirectories", "md_doc_2_b_u_i_l_d_s___a_n_d___c_m_a_k_e.html#autotoc_md196", null ]
        ] ]
      ] ]
    ] ],
    [ "Project Dependencies", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html", [
      [ "Summary", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md198", null ],
      [ "Details", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md199", [
        [ "Google Test", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md200", [
          [ "Setup", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md201", null ],
          [ "Version Requirements", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md202", null ]
        ] ],
        [ "C and C++ Compiler", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md203", [
          [ "Setup", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md204", null ],
          [ "Version Requirements", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md205", [
            [ "GCC", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md206", null ],
            [ "Clang", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md207", null ]
          ] ]
        ] ],
        [ "CMake", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md208", [
          [ "Setup", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md209", null ],
          [ "Version Requirements", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md210", null ]
        ] ],
        [ "Boost (Headers Only)", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md211", [
          [ "Setup", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md212", [
            [ "Setting BOOST_ROOT", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md213", null ]
          ] ],
          [ "Version Requirements", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md214", null ]
        ] ],
        [ "Udunits", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md215", [
          [ "Version Requirements", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md218", null ]
        ] ],
        [ "Python 3 Libraries", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md219", [
          [ "Setup", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md221", null ],
          [ "Version Requirements", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md222", null ]
        ] ],
        [ "pybind11", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md223", [
          [ "Setup", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md225", null ],
          [ "Version Requirements", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md226", null ]
        ] ],
        [ "The dmod.subsetservice Package", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md227", [
          [ "Setup", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md228", null ],
          [ "Version Requirements", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md229", null ]
        ] ],
        [ "t-route", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md230", [
          [ "Setup", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md231", null ]
        ] ],
        [ "NetCDF Libraries", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md232", [
          [ "Setup", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md233", null ],
          [ "Parallel NetCDF", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md234", null ]
        ] ]
      ] ]
    ] ],
    [ "Distributed Processing", "md_doc_2_d_i_s_t_r_i_b_u_t_e_d___p_r_o_c_e_s_s_i_n_g.html", [
      [ "Overview", "md_doc_2_d_i_s_t_r_i_b_u_t_e_d___p_r_o_c_e_s_s_i_n_g.html#autotoc_md236", null ],
      [ "Building the MPI-Enabled Nextgen Driver", "md_doc_2_d_i_s_t_r_i_b_u_t_e_d___p_r_o_c_e_s_s_i_n_g.html#autotoc_md237", null ],
      [ "Running the MPI-Enabled Nextgen Driver", "md_doc_2_d_i_s_t_r_i_b_u_t_e_d___p_r_o_c_e_s_s_i_n_g.html#autotoc_md238", [
        [ "Subdivided Hydrofabric", "md_doc_2_d_i_s_t_r_i_b_u_t_e_d___p_r_o_c_e_s_s_i_n_g.html#autotoc_md239", [
          [ "Driver Runtime Differences", "md_doc_2_d_i_s_t_r_i_b_u_t_e_d___p_r_o_c_e_s_s_i_n_g.html#autotoc_md240", null ],
          [ "File Names", "md_doc_2_d_i_s_t_r_i_b_u_t_e_d___p_r_o_c_e_s_s_i_n_g.html#autotoc_md241", null ],
          [ "On-the-fly Generation", "md_doc_2_d_i_s_t_r_i_b_u_t_e_d___p_r_o_c_e_s_s_i_n_g.html#autotoc_md242", null ]
        ] ],
        [ "Examples", "md_doc_2_d_i_s_t_r_i_b_u_t_e_d___p_r_o_c_e_s_s_i_n_g.html#autotoc_md243", [
          [ "Example 1 - Full Hydrofabric", "md_doc_2_d_i_s_t_r_i_b_u_t_e_d___p_r_o_c_e_s_s_i_n_g.html#autotoc_md244", null ],
          [ "Example 2 - Subdivided Hydrofabric", "md_doc_2_d_i_s_t_r_i_b_u_t_e_d___p_r_o_c_e_s_s_i_n_g.html#autotoc_md245", null ]
        ] ]
      ] ],
      [ "Partitioning Config Generator", "md_doc_2_d_i_s_t_r_i_b_u_t_e_d___p_r_o_c_e_s_s_i_n_g.html#autotoc_md246", null ]
    ] ],
    [ "Git Strategy", "md_doc_2_g_i_t___u_s_a_g_e.html", [
      [ "Branching Model", "md_doc_2_g_i_t___u_s_a_g_e.html#autotoc_md248", [
        [ "Feature Branches from master", "md_doc_2_g_i_t___u_s_a_g_e.html#autotoc_md249", null ],
        [ "Relating production, master, and Release Branches", "md_doc_2_g_i_t___u_s_a_g_e.html#autotoc_md250", null ]
      ] ],
      [ "Contributing", "md_doc_2_g_i_t___u_s_a_g_e.html#autotoc_md251", null ],
      [ "Optional: Setting Up Hook Scripts", "md_doc_2_g_i_t___u_s_a_g_e.html#autotoc_md252", null ]
    ] ],
    [ "Definitions", "md_doc_2glossary.html", [
      [ "References", "md_doc_2glossary.html#autotoc_md254", null ]
    ] ],
    [ "MPI Remote Nexus", "md_doc_2_m_p_i___r_e_m_o_t_e___n_e_x_u_s.html", [
      [ "Summary", "md_doc_2_m_p_i___r_e_m_o_t_e___n_e_x_u_s.html#autotoc_md256", null ]
    ] ],
    [ "NextGen on CONUS", "md_doc_2_next_gen___o_n___c_o_n_u_s.html", [
      [ "Summary", "md_doc_2_next_gen___o_n___c_o_n_u_s.html#autotoc_md258", null ],
      [ "Download the Codes", "md_doc_2_next_gen___o_n___c_o_n_u_s.html#autotoc_md259", null ],
      [ "Setting up the Environment", "md_doc_2_next_gen___o_n___c_o_n_u_s.html#autotoc_md260", null ],
      [ "Build the Executable", "md_doc_2_next_gen___o_n___c_o_n_u_s.html#autotoc_md261", null ],
      [ "CONUS Hydrofabric", "md_doc_2_next_gen___o_n___c_o_n_u_s.html#autotoc_md262", null ],
      [ "Generate Partition For Parallel Computation", "md_doc_2_next_gen___o_n___c_o_n_u_s.html#autotoc_md263", null ],
      [ "Prepare the Input Data", "md_doc_2_next_gen___o_n___c_o_n_u_s.html#autotoc_md264", null ],
      [ "Build the Realization Configurations", "md_doc_2_next_gen___o_n___c_o_n_u_s.html#autotoc_md265", null ],
      [ "Run Computations with Submodules", "md_doc_2_next_gen___o_n___c_o_n_u_s.html#autotoc_md266", null ],
      [ "Resource Usage", "md_doc_2_next_gen___o_n___c_o_n_u_s.html#autotoc_md267", null ],
      [ "Run Computation with Topmodel", "md_doc_2_next_gen___o_n___c_o_n_u_s.html#autotoc_md268", null ],
      [ "Run Computation with Routing", "md_doc_2_next_gen___o_n___c_o_n_u_s.html#autotoc_md269", null ]
    ] ],
    [ "ngen Serialization", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html", [
      [ "Engine behavior with non-conforming models", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md272", null ],
      [ "Wire Format", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md274", [
        [ "Two timestamps, not one", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md275", null ],
        [ "Wire-format version", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md276", null ],
        [ "Key properties", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md277", null ]
      ] ],
      [ "Save Protocol: NgenSerializationProtocol", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md279", [
        [ "Configuration", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md280", null ],
        [ "Support detection", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md281", null ],
        [ "Operation", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md282", null ],
        [ "Size variable width — how the engine composes with narrower models", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md283", null ]
      ] ],
      [ "Restore Protocol: NgenDeserializationProtocol", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md285", [
        [ "Configuration", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md286", null ],
        [ "Split save/restore paths", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md287", null ],
        [ "Support detection", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md288", null ],
        [ "Operation", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md289", null ]
      ] ],
      [ "Combined save + restore example", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md291", null ],
      [ "Restart semantics: hot start and resume", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md293", [
        [ "Hot start", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md294", null ],
        [ "Resume", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md295", null ],
        [ "Guaranteed consistent restore", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md296", null ],
        [ "Modules with no state", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md297", null ],
        [ "\"latest\"", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md298", null ],
        [ "Not supported yet", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md299", null ]
      ] ],
      [ "Identity Convention: compound_id()", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md301", [
        [ "How the compound gets set", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md302", null ]
      ] ],
      [ "Engine Integration", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md304", [
        [ "Save — per-timestep from Layer", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md305", null ],
        [ "Restore — once at the end of create_model", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md306", null ],
        [ "Lifecycle summary", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md307", null ]
      ] ],
      [ "Worked Example — multi-BMI formulation with save + restore", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md309", null ],
      [ "Realization-Level (Global) Configuration", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md311", [
        [ "Top-level placement", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md312", null ],
        [ "Precedence", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md313", null ],
        [ "Absent block", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md314", null ],
        [ "Path templating", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md315", null ]
      ] ],
      [ "Scaling Considerations", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md317", [
        [ "On-disk format", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md318", null ],
        [ "Save cost model", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md319", null ],
        [ "Restore cost model", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md320", null ],
        [ "id_subset for memory-bounded restores", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md321", null ],
        [ "Thread and process safety", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md322", null ],
        [ "When the protocol is not the right tool", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md323", null ]
      ] ]
    ] ],
    [ "Precision guidelines for validation of NGEN code translation and upgrades", "md_doc_2_precision___guidelines__for___ngen___code___validation.html", null ],
    [ "Standards for Programming", "md_doc_2programming__standards.html", [
      [ "Source Documentation", "md_doc_2programming__standards.html#autotoc_md326", null ],
      [ "Naming", "md_doc_2programming__standards.html#autotoc_md327", null ],
      [ "Other", "md_doc_2programming__standards.html#autotoc_md330", [
        [ "Headers", "md_doc_2programming__standards.html#autotoc_md331", null ],
        [ "Spaces vs Tabs", "md_doc_2programming__standards.html#autotoc_md332", null ],
        [ "Braces", "md_doc_2programming__standards.html#autotoc_md333", null ],
        [ "Line Width", "md_doc_2programming__standards.html#autotoc_md334", null ]
      ] ]
    ] ],
    [ "Python Routing", "md_doc_2_p_y_t_h_o_n___r_o_u_t_i_n_g.html", [
      [ "Summary", "md_doc_2_p_y_t_h_o_n___r_o_u_t_i_n_g.html#autotoc_md336", null ],
      [ "Setup Virtual Environment", "md_doc_2_p_y_t_h_o_n___r_o_u_t_i_n_g.html#autotoc_md337", null ],
      [ "Install t-route", "md_doc_2_p_y_t_h_o_n___r_o_u_t_i_n_g.html#autotoc_md338", null ],
      [ "Installation Caveats", "md_doc_2_p_y_t_h_o_n___r_o_u_t_i_n_g.html#autotoc_md339", [
        [ "Compilers and Libraries", "md_doc_2_p_y_t_h_o_n___r_o_u_t_i_n_g.html#autotoc_md340", null ],
        [ "Default installation is in development mode (impacts macOS)", "md_doc_2_p_y_t_h_o_n___r_o_u_t_i_n_g.html#autotoc_md341", null ],
        [ "Some tips on installation if you run into issues", "md_doc_2_p_y_t_h_o_n___r_o_u_t_i_n_g.html#autotoc_md342", null ]
      ] ],
      [ "Using t-route with ngen", "md_doc_2_p_y_t_h_o_n___r_o_u_t_i_n_g.html#autotoc_md343", [
        [ "Realization Config", "md_doc_2_p_y_t_h_o_n___r_o_u_t_i_n_g.html#autotoc_md344", null ],
        [ "Routing Config", "md_doc_2_p_y_t_h_o_n___r_o_u_t_i_n_g.html#autotoc_md345", null ],
        [ "Configuration considerations for t-route with ngen", "md_doc_2_p_y_t_h_o_n___r_o_u_t_i_n_g.html#autotoc_md346", null ]
      ] ],
      [ "Running t-route separately with ngen output", "md_doc_2_p_y_t_h_o_n___r_o_u_t_i_n_g.html#autotoc_md347", null ],
      [ "Known issues", "md_doc_2_p_y_t_h_o_n___r_o_u_t_i_n_g.html#autotoc_md348", [
        [ "Cleanup of *.parquet files required", "md_doc_2_p_y_t_h_o_n___r_o_u_t_i_n_g.html#autotoc_md349", null ],
        [ "Bug in multiprocessing on macOS", "md_doc_2_p_y_t_h_o_n___r_o_u_t_i_n_g.html#autotoc_md350", null ]
      ] ]
    ] ],
    [ "Realization Configuration", "md_doc_2_r_e_a_l_i_z_a_t_i_o_n___c_o_n_f_i_g_u_r_a_t_i_o_n.html", [
      [ "Description and Top-Level Structure", "md_doc_2_r_e_a_l_i_z_a_t_i_o_n___c_o_n_f_i_g_u_r_a_t_i_o_n.html#autotoc_md352", [
        [ "Required Top-Level Keys", "md_doc_2_r_e_a_l_i_z_a_t_i_o_n___c_o_n_f_i_g_u_r_a_t_i_o_n.html#autotoc_md353", null ],
        [ "Optional Top-Level Keys", "md_doc_2_r_e_a_l_i_z_a_t_i_o_n___c_o_n_f_i_g_u_r_a_t_i_o_n.html#autotoc_md354", [
          [ "output", "md_doc_2_r_e_a_l_i_z_a_t_i_o_n___c_o_n_f_i_g_u_r_a_t_i_o_n.html#autotoc_md355", null ],
          [ "output_root", "md_doc_2_r_e_a_l_i_z_a_t_i_o_n___c_o_n_f_i_g_u_r_a_t_i_o_n.html#autotoc_md356", null ],
          [ "disable_catchment_output", "md_doc_2_r_e_a_l_i_z_a_t_i_o_n___c_o_n_f_i_g_u_r_a_t_i_o_n.html#autotoc_md357", null ],
          [ "per_formulation_nexus_files", "md_doc_2_r_e_a_l_i_z_a_t_i_o_n___c_o_n_f_i_g_u_r_a_t_i_o_n.html#autotoc_md358", null ],
          [ "catchments", "md_doc_2_r_e_a_l_i_z_a_t_i_o_n___c_o_n_f_i_g_u_r_a_t_i_o_n.html#autotoc_md359", null ],
          [ "routing", "md_doc_2_r_e_a_l_i_z_a_t_i_o_n___c_o_n_f_i_g_u_r_a_t_i_o_n.html#autotoc_md360", null ]
        ] ],
        [ "Examples of Top-Level Structure", "md_doc_2_r_e_a_l_i_z_a_t_i_o_n___c_o_n_f_i_g_u_r_a_t_i_o_n.html#autotoc_md361", null ]
      ] ],
      [ "The Global Section", "md_doc_2_r_e_a_l_i_z_a_t_i_o_n___c_o_n_f_i_g_u_r_a_t_i_o_n.html#autotoc_md362", null ],
      [ "The Time Section", "md_doc_2_r_e_a_l_i_z_a_t_i_o_n___c_o_n_f_i_g_u_r_a_t_i_o_n.html#autotoc_md363", null ],
      [ "Individual Catchments", "md_doc_2_r_e_a_l_i_z_a_t_i_o_n___c_o_n_f_i_g_u_r_a_t_i_o_n.html#autotoc_md364", null ],
      [ "The Routing Section", "md_doc_2_r_e_a_l_i_z_a_t_i_o_n___c_o_n_f_i_g_u_r_a_t_i_o_n.html#autotoc_md365", null ],
      [ "A Full Example", "md_doc_2_r_e_a_l_i_z_a_t_i_o_n___c_o_n_f_i_g_u_r_a_t_i_o_n.html#autotoc_md366", null ],
      [ "A Note on BMI Models", "md_doc_2_r_e_a_l_i_z_a_t_i_o_n___c_o_n_f_i_g_u_r_a_t_i_o_n.html#autotoc_md367", null ]
    ] ],
    [ "<a href=\"https://docs.opengeospatial.org/is/14-111r6/14-111r6.html\" >HY_Features</a>", "md_doc_2references.html", [
      [ "BMI", "md_doc_2references.html#autotoc_md369", null ],
      [ "Formulations", "md_doc_2references.html#autotoc_md370", null ],
      [ "HYMOD", "md_doc_2references.html#autotoc_md371", null ],
      [ "T-shirt", "md_doc_2references.html#autotoc_md372", null ],
      [ "GIUH", "md_doc_2references.html#autotoc_md373", [
        [ "Modeling References", "md_doc_2references.html#autotoc_md374", null ]
      ] ]
    ] ],
    [ "Release Management", "md_doc_2_r_e_l_e_a_s_e___m_a_n_a_g_e_m_e_n_t.html", [
      [ "The Release Process", "md_doc_2_r_e_l_e_a_s_e___m_a_n_a_g_e_m_e_n_t.html#autotoc_md376", [
        [ "TL;DR", "md_doc_2_r_e_l_e_a_s_e___m_a_n_a_g_e_m_e_n_t.html#autotoc_md377", null ],
        [ "Process Steps", "md_doc_2_r_e_l_e_a_s_e___m_a_n_a_g_e_m_e_n_t.html#autotoc_md378", null ]
      ] ]
    ] ],
    [ "Installation instructions", "md__i_n_s_t_a_l_l.html", [
      [ "Building and running with docker:", "md__i_n_s_t_a_l_l.html#autotoc_md381", null ],
      [ "Building manually:", "md__i_n_s_t_a_l_l.html#autotoc_md382", null ]
    ] ],
    [ "Guidance on How to Contribute", "md__c_o_n_t_r_i_b_u_t_i_n_g.html", [
      [ "Using the Issue Tracker", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md385", null ],
      [ "Changing the Code-Base", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md386", [
        [ "Summary", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md387", null ],
        [ "Getting Started", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md388", [
          [ "Optional: Git Hooks", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md389", null ]
        ] ],
        [ "Developing Changes", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md390", [
          [ "Work in a Dedicated Branch", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md391", null ],
          [ "Pushing Incremental Commits", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md392", null ],
          [ "Submitting Pull Requests", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md393", [
            [ "Guidelines for Pull Requests", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md394", null ],
            [ "PR Review and Requested Revisions", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md395", null ]
          ] ]
        ] ],
        [ "Keeping Forks Up to Date", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md396", [
          [ "A Rebase Strategy", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md397", null ],
          [ "Getting Remote Changes", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md398", [
            [ "For production Too", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md399", null ]
          ] ],
          [ "Rebasing Development Branches", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md400", [
            [ "Interactive Rebasing", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md401", null ]
          ] ],
          [ "Fixing Diverging Development Branches", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md402", null ]
        ] ]
      ] ]
    ] ],
    [ "Disclaimer", "md__t_e_r_m_s.html", [
      [ "Exceptions", "md__t_e_r_m_s.html#autotoc_md405", null ]
    ] ],
    [ "Testing", "md_test_2_r_e_a_d_m_e.html", [
      [ "Testing Frameworks", "md_test_2_r_e_a_d_m_e.html#autotoc_md407", [
        [ "Google Test", "md_test_2_r_e_a_d_m_e.html#autotoc_md408", [
          [ "(Re)generating CMake Project Buildsystem", "md_test_2_r_e_a_d_m_e.html#autotoc_md409", null ]
        ] ]
      ] ],
      [ "Executing Automated Tests", "md_test_2_r_e_a_d_m_e.html#autotoc_md410", [
        [ "C++ Tests", "md_test_2_r_e_a_d_m_e.html#autotoc_md411", [
          [ "Test Targets and Executables", "md_test_2_r_e_a_d_m_e.html#autotoc_md412", null ]
        ] ]
      ] ],
      [ "Creating New Automated Tests", "md_test_2_r_e_a_d_m_e.html#autotoc_md413", [
        [ "Adding Tests to CMake Builds", "md_test_2_r_e_a_d_m_e.html#autotoc_md414", null ],
        [ "Test Creation Rules of Thumb", "md_test_2_r_e_a_d_m_e.html#autotoc_md415", null ]
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
"classmodels_1_1bmi_1_1_bmi___c___adapter.html#ad535354b96c22665567b45954f4ed31b",
"classmodels_1_1bmi_1_1protocols_1_1_ngen_deserialization_protocol.html#a56b9df3eac23cc22710398fb23af45ac",
"classngen_1_1_layer.html#a96c3fed503f10d2b2f9ab00c229a5b91",
"classngen_1_1serialization_1_1_file_backend.html#a51615949e3e20f6bc7554ba6ff858ad6",
"classrealization_1_1_bmi___c___formulation.html#a70771d21690659633a04a2f375283945",
"classrealization_1_1_bmi___cpp___formulation.html#a9bf1db96b3ecb9b4ae5bc7b0c2e6c76b",
"classrealization_1_1_bmi___module___formulation.html#a291e55e27455ce2d068b02b6601fba00",
"classrealization_1_1_bmi___multi___formulation.html#a624c5166a6e42c6d1aafde6883a20388",
"classrealization_1_1_formulation___manager.html#a80cd071b92ac039c3a23e54a4838c6f5",
"functions_vars.html",
"md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md287",
"namespacehy__features_1_1hydrolocation.html#a7fcfe40520bcda2cd5be66a5b5bbbc0ba54e40d3becb67eb160d0618f620e1b88",
"namespacengen_1_1serialization.html#ae84cb2bca59109b764caad44a79633e5a2133fd717402a7966ee88d06f9e0b792",
"structgeojson_1_1_list.html#a28e0531d1eb1f1b8cbedbe4c759fefb7",
"structngen_1_1sqlite_1_1database_1_1iterator.html#a6c3b0d6cb4ca0421cc85ce2f20789cb9"
];

var SYNCONMSG = 'click to disable panel synchronisation';
var SYNCOFFMSG = 'click to enable panel synchronisation';