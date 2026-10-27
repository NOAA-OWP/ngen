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
      [ "What Are Automated Tests and How to Start Them", "md_doc_2_a_u_t_o_m_a_t_e_d___t_e_s_t.html#autotoc_md81", null ],
      [ "What Code Tests Are Performed", "md_doc_2_a_u_t_o_m_a_t_e_d___t_e_s_t.html#autotoc_md82", null ],
      [ "Local Testing", "md_doc_2_a_u_t_o_m_a_t_e_d___t_e_s_t.html#autotoc_md83", null ],
      [ "What to Do When a Test Fail", "md_doc_2_a_u_t_o_m_a_t_e_d___t_e_s_t.html#autotoc_md84", null ]
    ] ],
    [ "BMI External Models", "md_doc_2_b_m_i___m_o_d_e_l_s.html", [
      [ "Summary", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md86", null ],
      [ "Formulation Config", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md87", [
        [ "Required Parameters", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md88", null ],
        [ "Semi-Optional Parameters", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md90", null ],
        [ "Optional Parameters", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md91", null ]
      ] ],
      [ "BMI Models Written in C", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md92", [
        [ "BMI C Model As Shared Library", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md93", [
          [ "Dynamic Loading", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md94", null ]
        ] ],
        [ "BMI C CFE Example", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md95", null ],
        [ "BMI C Caveats", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md96", [
          [ "BMI C Activate/Deactivation Required in CMake Build", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md97", null ],
          [ "Additional Bootstrapping Function Needed", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md98", [
            [ "Why?", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md99", null ]
          ] ]
        ] ]
      ] ],
      [ "BMI Models Written in C++", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md100", [
        [ "BMI C++ Model As Shared Library", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md101", [
          [ "Dynamic Loading", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md102", null ],
          [ "Additional Bootstrapping Functions Needed", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md103", [
            [ "Why?", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md104", null ]
          ] ]
        ] ],
        [ "BMI C++ Example", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md105", null ]
      ] ],
      [ "BMI Models Written in Python", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md106", [
        [ "Enabling Python Integration", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md107", null ],
        [ "BMI Python Model as Package Class", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md108", null ],
        [ "BMI Python Example", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md109", null ]
      ] ],
      [ "BMI Models Written in Fortran", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md110", [
        [ "Enabling Fortran Integration", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md111", null ],
        [ "ISO C Binding Middleware", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md112", null ],
        [ "A Compiled Shared Library", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md113", [
          [ "Required Additional Fortran Registration Function", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md114", null ]
        ] ]
      ] ],
      [ "Multi-Module BMI Formulations", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md115", [
        [ "Passing Variables Between Nested Formulations", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md116", [
          [ "How Values Are Orchestrated", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md117", null ],
          [ "Look-back and Default Values", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md118", [
            [ "Example Look-Back Config", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md119", null ]
          ] ]
        ] ]
      ] ]
    ] ],
    [ "ngen BMI Serialization Protocol", "md_doc_2_b_m_i___s_e_r_i_a_l_i_z_a_t_i_o_n___p_r_o_t_o_c_o_l.html", [
      [ "TODO or Not TODO", "md_doc_2_b_m_i___s_e_r_i_a_l_i_z_a_t_i_o_n___p_r_o_t_o_c_o_l.html#autotoc_md121", [
        [ "Conformance is use-case dependent", "md_doc_2_b_m_i___s_e_r_i_a_l_i_z_a_t_i_o_n___p_r_o_t_o_c_o_l.html#autotoc_md122", null ]
      ] ],
      [ "Reserved BMI Variables", "md_doc_2_b_m_i___s_e_r_i_a_l_i_z_a_t_i_o_n___p_r_o_t_o_c_o_l.html#autotoc_md124", [
        [ "Item list behavior", "md_doc_2_b_m_i___s_e_r_i_a_l_i_z_a_t_i_o_n___p_r_o_t_o_c_o_l.html#autotoc_md125", null ]
      ] ],
      [ "Unit Conventions for Non-Physical Variables", "md_doc_2_b_m_i___s_e_r_i_a_l_i_z_a_t_i_o_n___p_r_o_t_o_c_o_l.html#autotoc_md127", null ],
      [ "Model Developer Implementation Guide", "md_doc_2_b_m_i___s_e_r_i_a_l_i_z_a_t_i_o_n___p_r_o_t_o_c_o_l.html#autotoc_md129", [
        [ "Width of the size variable", "md_doc_2_b_m_i___s_e_r_i_a_l_i_z_a_t_i_o_n___p_r_o_t_o_c_o_l.html#autotoc_md130", null ],
        [ "Recommended: defensive payload format", "md_doc_2_b_m_i___s_e_r_i_a_l_i_z_a_t_i_o_n___p_r_o_t_o_c_o_l.html#autotoc_md131", null ],
        [ "Constraints and non-goals", "md_doc_2_b_m_i___s_e_r_i_a_l_i_z_a_t_i_o_n___p_r_o_t_o_c_o_l.html#autotoc_md132", null ],
        [ "Optional: GetValuePtr for zero-copy state access", "md_doc_2_b_m_i___s_e_r_i_a_l_i_z_a_t_i_o_n___p_r_o_t_o_c_o_l.html#autotoc_md133", null ],
        [ "Recommended: single field list drives save, restore, and size", "md_doc_2_b_m_i___s_e_r_i_a_l_i_z_a_t_i_o_n___p_r_o_t_o_c_o_l.html#autotoc_md134", null ],
        [ "Reference implementations", "md_doc_2_b_m_i___s_e_r_i_a_l_i_z_a_t_i_o_n___p_r_o_t_o_c_o_l.html#autotoc_md135", null ]
      ] ]
    ] ],
    [ "BMI Conventions", "md_doc_2_b_m_iconventions.html", [
      [ "Compliance with the BMI v2.0 Standard", "md_doc_2_b_m_iconventions.html#autotoc_md137", null ],
      [ "Time Control", "md_doc_2_b_m_iconventions.html#autotoc_md138", [
        [ "Set and Get Values Per Timestep", "md_doc_2_b_m_iconventions.html#autotoc_md139", null ],
        [ "update vs. update_until", "md_doc_2_b_m_iconventions.html#autotoc_md140", null ],
        [ "Time Units", "md_doc_2_b_m_iconventions.html#autotoc_md141", null ],
        [ "Model time begin", "md_doc_2_b_m_iconventions.html#autotoc_md142", null ],
        [ "Model time end", "md_doc_2_b_m_iconventions.html#autotoc_md143", null ]
      ] ],
      [ "Metadata", "md_doc_2_b_m_iconventions.html#autotoc_md144", [
        [ "Variable Units", "md_doc_2_b_m_iconventions.html#autotoc_md145", null ],
        [ "Variable Types and Sizes", "md_doc_2_b_m_iconventions.html#autotoc_md146", null ]
      ] ],
      [ "Input and output published variables", "md_doc_2_b_m_iconventions.html#autotoc_md147", [
        [ "Input/output variables and the time loop", "md_doc_2_b_m_iconventions.html#autotoc_md148", null ],
        [ "Validation of formulations for input variables", "md_doc_2_b_m_iconventions.html#autotoc_md149", null ],
        [ "Validation of formulations for output variables", "md_doc_2_b_m_iconventions.html#autotoc_md150", null ],
        [ "Calling set_value on output variables", "md_doc_2_b_m_iconventions.html#autotoc_md151", null ],
        [ "Calling get_value on input variables", "md_doc_2_b_m_iconventions.html#autotoc_md152", null ],
        [ "Input and output with the same name not permitted", "md_doc_2_b_m_iconventions.html#autotoc_md153", null ]
      ] ],
      [ "Unpublished variables", "md_doc_2_b_m_iconventions.html#autotoc_md154", [
        [ "Unknown/unexpected variables passed to set_value", "md_doc_2_b_m_iconventions.html#autotoc_md155", null ],
        [ "Must support variable metadata functions", "md_doc_2_b_m_iconventions.html#autotoc_md156", null ]
      ] ],
      [ "Array representation", "md_doc_2_b_m_iconventions.html#autotoc_md157", [
        [ "Contiguousness", "md_doc_2_b_m_iconventions.html#autotoc_md158", null ],
        [ "Zero-Indexing", "md_doc_2_b_m_iconventions.html#autotoc_md159", null ],
        [ "Layout and Flattening", "md_doc_2_b_m_iconventions.html#autotoc_md160", [
          [ "C ordering required", "md_doc_2_b_m_iconventions.html#autotoc_md161", null ],
          [ "Proper ordering example", "md_doc_2_b_m_iconventions.html#autotoc_md162", null ],
          [ "When this matters", "md_doc_2_b_m_iconventions.html#autotoc_md163", [
            [ "Fortran models using z,y,x index ordering", "md_doc_2_b_m_iconventions.html#autotoc_md164", null ],
            [ "Clarification for get_value_at_indices and set_value_at_indices", "md_doc_2_b_m_iconventions.html#autotoc_md165", null ],
            [ "Spatial data arrays for structured grids", "md_doc_2_b_m_iconventions.html#autotoc_md166", null ]
          ] ]
        ] ]
      ] ],
      [ "Grid metadata functions", "md_doc_2_b_m_iconventions.html#autotoc_md167", [
        [ "Required metadata functions", "md_doc_2_b_m_iconventions.html#autotoc_md168", null ],
        [ "Ranks and sizes for scalar values", "md_doc_2_b_m_iconventions.html#autotoc_md169", null ]
      ] ],
      [ "Specific ngen uses of unpublished variables", "md_doc_2_b_m_iconventions.html#autotoc_md170", [
        [ "The model_params list", "md_doc_2_b_m_iconventions.html#autotoc_md171", null ],
        [ "Grid sizing parameters", "md_doc_2_b_m_iconventions.html#autotoc_md172", [
          [ "ngen_grid_N_shape", "md_doc_2_b_m_iconventions.html#autotoc_md173", null ],
          [ "ngen_grid_N_spacing", "md_doc_2_b_m_iconventions.html#autotoc_md174", null ],
          [ "ngen_grid_N_origin", "md_doc_2_b_m_iconventions.html#autotoc_md175", null ]
        ] ]
      ] ]
    ] ],
    [ "Project Builds", "md_doc_2_b_u_i_l_d_s___a_n_d___c_m_a_k_e.html", [
      [ "The TL;DR", "md_doc_2_b_u_i_l_d_s___a_n_d___c_m_a_k_e.html#autotoc_md177", null ],
      [ "Generating a Build System", "md_doc_2_b_u_i_l_d_s___a_n_d___c_m_a_k_e.html#autotoc_md178", [
        [ "Out-of-Source Building", "md_doc_2_b_u_i_l_d_s___a_n_d___c_m_a_k_e.html#autotoc_md179", null ],
        [ "Ignoring the Build Directory", "md_doc_2_b_u_i_l_d_s___a_n_d___c_m_a_k_e.html#autotoc_md180", null ],
        [ "Generation Commands", "md_doc_2_b_u_i_l_d_s___a_n_d___c_m_a_k_e.html#autotoc_md181", [
          [ "Generating from Build Directory", "md_doc_2_b_u_i_l_d_s___a_n_d___c_m_a_k_e.html#autotoc_md182", null ],
          [ "Generating from Project Root", "md_doc_2_b_u_i_l_d_s___a_n_d___c_m_a_k_e.html#autotoc_md183", null ],
          [ "Optional: Specifying Project Build Config Type", "md_doc_2_b_u_i_l_d_s___a_n_d___c_m_a_k_e.html#autotoc_md184", null ]
        ] ],
        [ "Regenerating", "md_doc_2_b_u_i_l_d_s___a_n_d___c_m_a_k_e.html#autotoc_md185", null ]
      ] ],
      [ "Building Targets", "md_doc_2_b_u_i_l_d_s___a_n_d___c_m_a_k_e.html#autotoc_md186", [
        [ "Building From Build Directory", "md_doc_2_b_u_i_l_d_s___a_n_d___c_m_a_k_e.html#autotoc_md187", null ],
        [ "Building From Source Directory", "md_doc_2_b_u_i_l_d_s___a_n_d___c_m_a_k_e.html#autotoc_md188", null ]
      ] ],
      [ "Dependencies", "md_doc_2_b_u_i_l_d_s___a_n_d___c_m_a_k_e.html#autotoc_md189", [
        [ "Build System Regeneration", "md_doc_2_b_u_i_l_d_s___a_n_d___c_m_a_k_e.html#autotoc_md190", null ],
        [ "Boost ENV Variable", "md_doc_2_b_u_i_l_d_s___a_n_d___c_m_a_k_e.html#autotoc_md191", null ]
      ] ],
      [ "Build Design", "md_doc_2_b_u_i_l_d_s___a_n_d___c_m_a_k_e.html#autotoc_md192", [
        [ "Library-Based Structure", "md_doc_2_b_u_i_l_d_s___a_n_d___c_m_a_k_e.html#autotoc_md193", null ],
        [ "Handling Changes", "md_doc_2_b_u_i_l_d_s___a_n_d___c_m_a_k_e.html#autotoc_md194", [
          [ "Adding New Subdirectories", "md_doc_2_b_u_i_l_d_s___a_n_d___c_m_a_k_e.html#autotoc_md195", null ]
        ] ]
      ] ]
    ] ],
    [ "Project Dependencies", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html", [
      [ "Summary", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md197", null ],
      [ "Details", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md198", [
        [ "Google Test", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md199", [
          [ "Setup", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md200", null ],
          [ "Version Requirements", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md201", null ]
        ] ],
        [ "C and C++ Compiler", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md202", [
          [ "Setup", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md203", null ],
          [ "Version Requirements", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md204", [
            [ "GCC", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md205", null ],
            [ "Clang", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md206", null ]
          ] ]
        ] ],
        [ "CMake", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md207", [
          [ "Setup", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md208", null ],
          [ "Version Requirements", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md209", null ]
        ] ],
        [ "Boost (Headers Only)", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md210", [
          [ "Setup", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md211", [
            [ "Setting BOOST_ROOT", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md212", null ]
          ] ],
          [ "Version Requirements", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md213", null ]
        ] ],
        [ "Udunits", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md214", [
          [ "Version Requirements", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md217", null ]
        ] ],
        [ "Python 3 Libraries", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md218", [
          [ "Setup", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md220", null ],
          [ "Version Requirements", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md221", null ]
        ] ],
        [ "pybind11", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md222", [
          [ "Setup", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md224", null ],
          [ "Version Requirements", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md225", null ]
        ] ],
        [ "The dmod.subsetservice Package", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md226", [
          [ "Setup", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md227", null ],
          [ "Version Requirements", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md228", null ]
        ] ],
        [ "t-route", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md229", [
          [ "Setup", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md230", null ]
        ] ],
        [ "NetCDF Libraries", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md231", [
          [ "Setup", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md232", null ],
          [ "Parallel NetCDF", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md233", null ]
        ] ]
      ] ]
    ] ],
    [ "Distributed Processing", "md_doc_2_d_i_s_t_r_i_b_u_t_e_d___p_r_o_c_e_s_s_i_n_g.html", [
      [ "Overview", "md_doc_2_d_i_s_t_r_i_b_u_t_e_d___p_r_o_c_e_s_s_i_n_g.html#autotoc_md235", null ],
      [ "Building the MPI-Enabled Nextgen Driver", "md_doc_2_d_i_s_t_r_i_b_u_t_e_d___p_r_o_c_e_s_s_i_n_g.html#autotoc_md236", null ],
      [ "Running the MPI-Enabled Nextgen Driver", "md_doc_2_d_i_s_t_r_i_b_u_t_e_d___p_r_o_c_e_s_s_i_n_g.html#autotoc_md237", [
        [ "Subdivided Hydrofabric", "md_doc_2_d_i_s_t_r_i_b_u_t_e_d___p_r_o_c_e_s_s_i_n_g.html#autotoc_md238", [
          [ "Driver Runtime Differences", "md_doc_2_d_i_s_t_r_i_b_u_t_e_d___p_r_o_c_e_s_s_i_n_g.html#autotoc_md239", null ],
          [ "File Names", "md_doc_2_d_i_s_t_r_i_b_u_t_e_d___p_r_o_c_e_s_s_i_n_g.html#autotoc_md240", null ],
          [ "On-the-fly Generation", "md_doc_2_d_i_s_t_r_i_b_u_t_e_d___p_r_o_c_e_s_s_i_n_g.html#autotoc_md241", null ]
        ] ],
        [ "Examples", "md_doc_2_d_i_s_t_r_i_b_u_t_e_d___p_r_o_c_e_s_s_i_n_g.html#autotoc_md242", [
          [ "Example 1 - Full Hydrofabric", "md_doc_2_d_i_s_t_r_i_b_u_t_e_d___p_r_o_c_e_s_s_i_n_g.html#autotoc_md243", null ],
          [ "Example 2 - Subdivided Hydrofabric", "md_doc_2_d_i_s_t_r_i_b_u_t_e_d___p_r_o_c_e_s_s_i_n_g.html#autotoc_md244", null ]
        ] ]
      ] ],
      [ "Partitioning Config Generator", "md_doc_2_d_i_s_t_r_i_b_u_t_e_d___p_r_o_c_e_s_s_i_n_g.html#autotoc_md245", null ]
    ] ],
    [ "Git Strategy", "md_doc_2_g_i_t___u_s_a_g_e.html", [
      [ "Branching Model", "md_doc_2_g_i_t___u_s_a_g_e.html#autotoc_md247", [
        [ "Feature Branches from master", "md_doc_2_g_i_t___u_s_a_g_e.html#autotoc_md248", null ],
        [ "Relating production, master, and Release Branches", "md_doc_2_g_i_t___u_s_a_g_e.html#autotoc_md249", null ]
      ] ],
      [ "Contributing", "md_doc_2_g_i_t___u_s_a_g_e.html#autotoc_md250", null ],
      [ "Optional: Setting Up Hook Scripts", "md_doc_2_g_i_t___u_s_a_g_e.html#autotoc_md251", null ]
    ] ],
    [ "Definitions", "md_doc_2glossary.html", [
      [ "References", "md_doc_2glossary.html#autotoc_md253", null ]
    ] ],
    [ "MPI Remote Nexus", "md_doc_2_m_p_i___r_e_m_o_t_e___n_e_x_u_s.html", [
      [ "Summary", "md_doc_2_m_p_i___r_e_m_o_t_e___n_e_x_u_s.html#autotoc_md255", null ]
    ] ],
    [ "NextGen on CONUS", "md_doc_2_next_gen___o_n___c_o_n_u_s.html", [
      [ "Summary", "md_doc_2_next_gen___o_n___c_o_n_u_s.html#autotoc_md257", null ],
      [ "Download the Codes", "md_doc_2_next_gen___o_n___c_o_n_u_s.html#autotoc_md258", null ],
      [ "Setting up the Environment", "md_doc_2_next_gen___o_n___c_o_n_u_s.html#autotoc_md259", null ],
      [ "Build the Executable", "md_doc_2_next_gen___o_n___c_o_n_u_s.html#autotoc_md260", null ],
      [ "CONUS Hydrofabric", "md_doc_2_next_gen___o_n___c_o_n_u_s.html#autotoc_md261", null ],
      [ "Generate Partition For Parallel Computation", "md_doc_2_next_gen___o_n___c_o_n_u_s.html#autotoc_md262", null ],
      [ "Prepare the Input Data", "md_doc_2_next_gen___o_n___c_o_n_u_s.html#autotoc_md263", null ],
      [ "Build the Realization Configurations", "md_doc_2_next_gen___o_n___c_o_n_u_s.html#autotoc_md264", null ],
      [ "Run Computations with Submodules", "md_doc_2_next_gen___o_n___c_o_n_u_s.html#autotoc_md265", null ],
      [ "Resource Usage", "md_doc_2_next_gen___o_n___c_o_n_u_s.html#autotoc_md266", null ],
      [ "Run Computation with Topmodel", "md_doc_2_next_gen___o_n___c_o_n_u_s.html#autotoc_md267", null ],
      [ "Run Computation with Routing", "md_doc_2_next_gen___o_n___c_o_n_u_s.html#autotoc_md268", null ]
    ] ],
    [ "ngen Serialization", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html", [
      [ "Engine behavior with non-conforming models", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md271", null ],
      [ "Wire Format", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md273", [
        [ "Two timestamps, not one", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md274", null ],
        [ "Wire-format version", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md275", null ],
        [ "Key properties", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md276", null ]
      ] ],
      [ "Save Protocol: NgenSerializationProtocol", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md278", [
        [ "Configuration", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md279", null ],
        [ "Support detection", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md280", null ],
        [ "Operation", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md281", null ],
        [ "Size variable width — how the engine composes with narrower models", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md282", null ]
      ] ],
      [ "Restore Protocol: NgenDeserializationProtocol", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md284", [
        [ "Configuration", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md285", null ],
        [ "Split save/restore paths", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md286", null ],
        [ "Support detection", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md287", null ],
        [ "Operation", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md288", null ]
      ] ],
      [ "Combined save + restore example", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md290", null ],
      [ "Restart semantics: hot start and resume", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md292", [
        [ "Hot start", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md293", null ],
        [ "Resume", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md294", null ],
        [ "Guaranteed consistent restore", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md295", null ],
        [ "Modules with no state", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md296", null ],
        [ "\"latest\"", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md297", null ],
        [ "Not supported yet", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md298", null ]
      ] ],
      [ "Identity Convention: compound_id()", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md300", [
        [ "How the compound gets set", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md301", null ]
      ] ],
      [ "Engine Integration", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md303", [
        [ "Save — per-timestep from Layer", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md304", null ],
        [ "Restore — once at the end of create_model", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md305", null ],
        [ "Lifecycle summary", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md306", null ]
      ] ],
      [ "Worked Example — multi-BMI formulation with save + restore", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md308", null ],
      [ "Realization-Level (Global) Configuration", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md310", [
        [ "Top-level placement", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md311", null ],
        [ "Precedence", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md312", null ],
        [ "Absent block", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md313", null ],
        [ "Path templating", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md314", null ]
      ] ],
      [ "Scaling Considerations", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md316", [
        [ "On-disk format", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md317", null ],
        [ "Save cost model", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md318", null ],
        [ "Restore cost model", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md319", null ],
        [ "id_subset for memory-bounded restores", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md320", null ],
        [ "Thread and process safety", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md321", null ],
        [ "When the protocol is not the right tool", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md322", null ]
      ] ]
    ] ],
    [ "Precision guidelines for validation of NGEN code translation and upgrades", "md_doc_2_precision___guidelines__for___ngen___code___validation.html", null ],
    [ "Standards for Programming", "md_doc_2programming__standards.html", [
      [ "Source Documentation", "md_doc_2programming__standards.html#autotoc_md325", null ],
      [ "Naming", "md_doc_2programming__standards.html#autotoc_md326", null ],
      [ "Other", "md_doc_2programming__standards.html#autotoc_md329", [
        [ "Headers", "md_doc_2programming__standards.html#autotoc_md330", null ],
        [ "Spaces vs Tabs", "md_doc_2programming__standards.html#autotoc_md331", null ],
        [ "Braces", "md_doc_2programming__standards.html#autotoc_md332", null ],
        [ "Line Width", "md_doc_2programming__standards.html#autotoc_md333", null ]
      ] ]
    ] ],
    [ "Python Routing", "md_doc_2_p_y_t_h_o_n___r_o_u_t_i_n_g.html", [
      [ "Summary", "md_doc_2_p_y_t_h_o_n___r_o_u_t_i_n_g.html#autotoc_md335", null ],
      [ "Setup Virtual Environment", "md_doc_2_p_y_t_h_o_n___r_o_u_t_i_n_g.html#autotoc_md336", null ],
      [ "Install t-route", "md_doc_2_p_y_t_h_o_n___r_o_u_t_i_n_g.html#autotoc_md337", null ],
      [ "Installation Caveats", "md_doc_2_p_y_t_h_o_n___r_o_u_t_i_n_g.html#autotoc_md338", [
        [ "Compilers and Libraries", "md_doc_2_p_y_t_h_o_n___r_o_u_t_i_n_g.html#autotoc_md339", null ],
        [ "Default installation is in development mode (impacts macOS)", "md_doc_2_p_y_t_h_o_n___r_o_u_t_i_n_g.html#autotoc_md340", null ],
        [ "Some tips on installation if you run into issues", "md_doc_2_p_y_t_h_o_n___r_o_u_t_i_n_g.html#autotoc_md341", null ]
      ] ],
      [ "Using t-route with ngen", "md_doc_2_p_y_t_h_o_n___r_o_u_t_i_n_g.html#autotoc_md342", [
        [ "Realization Config", "md_doc_2_p_y_t_h_o_n___r_o_u_t_i_n_g.html#autotoc_md343", null ],
        [ "Routing Config", "md_doc_2_p_y_t_h_o_n___r_o_u_t_i_n_g.html#autotoc_md344", null ],
        [ "Configuration considerations for t-route with ngen", "md_doc_2_p_y_t_h_o_n___r_o_u_t_i_n_g.html#autotoc_md345", null ]
      ] ],
      [ "Running t-route separately with ngen output", "md_doc_2_p_y_t_h_o_n___r_o_u_t_i_n_g.html#autotoc_md346", null ],
      [ "Known issues", "md_doc_2_p_y_t_h_o_n___r_o_u_t_i_n_g.html#autotoc_md347", [
        [ "Cleanup of *.parquet files required", "md_doc_2_p_y_t_h_o_n___r_o_u_t_i_n_g.html#autotoc_md348", null ],
        [ "Bug in multiprocessing on macOS", "md_doc_2_p_y_t_h_o_n___r_o_u_t_i_n_g.html#autotoc_md349", null ]
      ] ]
    ] ],
    [ "Realization Configuration", "md_doc_2_r_e_a_l_i_z_a_t_i_o_n___c_o_n_f_i_g_u_r_a_t_i_o_n.html", [
      [ "Description and Top-Level Structure", "md_doc_2_r_e_a_l_i_z_a_t_i_o_n___c_o_n_f_i_g_u_r_a_t_i_o_n.html#autotoc_md351", [
        [ "Required Top-Level Keys", "md_doc_2_r_e_a_l_i_z_a_t_i_o_n___c_o_n_f_i_g_u_r_a_t_i_o_n.html#autotoc_md352", null ],
        [ "Optional Top-Level Keys", "md_doc_2_r_e_a_l_i_z_a_t_i_o_n___c_o_n_f_i_g_u_r_a_t_i_o_n.html#autotoc_md353", [
          [ "output", "md_doc_2_r_e_a_l_i_z_a_t_i_o_n___c_o_n_f_i_g_u_r_a_t_i_o_n.html#autotoc_md354", null ],
          [ "output_root", "md_doc_2_r_e_a_l_i_z_a_t_i_o_n___c_o_n_f_i_g_u_r_a_t_i_o_n.html#autotoc_md355", null ],
          [ "disable_catchment_output", "md_doc_2_r_e_a_l_i_z_a_t_i_o_n___c_o_n_f_i_g_u_r_a_t_i_o_n.html#autotoc_md356", null ],
          [ "per_formulation_nexus_files", "md_doc_2_r_e_a_l_i_z_a_t_i_o_n___c_o_n_f_i_g_u_r_a_t_i_o_n.html#autotoc_md357", null ],
          [ "catchments", "md_doc_2_r_e_a_l_i_z_a_t_i_o_n___c_o_n_f_i_g_u_r_a_t_i_o_n.html#autotoc_md358", null ],
          [ "routing", "md_doc_2_r_e_a_l_i_z_a_t_i_o_n___c_o_n_f_i_g_u_r_a_t_i_o_n.html#autotoc_md359", null ]
        ] ],
        [ "Examples of Top-Level Structure", "md_doc_2_r_e_a_l_i_z_a_t_i_o_n___c_o_n_f_i_g_u_r_a_t_i_o_n.html#autotoc_md360", null ]
      ] ],
      [ "The Global Section", "md_doc_2_r_e_a_l_i_z_a_t_i_o_n___c_o_n_f_i_g_u_r_a_t_i_o_n.html#autotoc_md361", null ],
      [ "The Time Section", "md_doc_2_r_e_a_l_i_z_a_t_i_o_n___c_o_n_f_i_g_u_r_a_t_i_o_n.html#autotoc_md362", null ],
      [ "Individual Catchments", "md_doc_2_r_e_a_l_i_z_a_t_i_o_n___c_o_n_f_i_g_u_r_a_t_i_o_n.html#autotoc_md363", null ],
      [ "The Routing Section", "md_doc_2_r_e_a_l_i_z_a_t_i_o_n___c_o_n_f_i_g_u_r_a_t_i_o_n.html#autotoc_md364", null ],
      [ "A Full Example", "md_doc_2_r_e_a_l_i_z_a_t_i_o_n___c_o_n_f_i_g_u_r_a_t_i_o_n.html#autotoc_md365", null ],
      [ "A Note on BMI Models", "md_doc_2_r_e_a_l_i_z_a_t_i_o_n___c_o_n_f_i_g_u_r_a_t_i_o_n.html#autotoc_md366", null ]
    ] ],
    [ "<a href=\"https://docs.opengeospatial.org/is/14-111r6/14-111r6.html\" >HY_Features</a>", "md_doc_2references.html", [
      [ "BMI", "md_doc_2references.html#autotoc_md368", null ],
      [ "Formulations", "md_doc_2references.html#autotoc_md369", null ],
      [ "HYMOD", "md_doc_2references.html#autotoc_md370", null ],
      [ "T-shirt", "md_doc_2references.html#autotoc_md371", null ],
      [ "GIUH", "md_doc_2references.html#autotoc_md372", [
        [ "Modeling References", "md_doc_2references.html#autotoc_md373", null ]
      ] ]
    ] ],
    [ "Release Management", "md_doc_2_r_e_l_e_a_s_e___m_a_n_a_g_e_m_e_n_t.html", [
      [ "The Release Process", "md_doc_2_r_e_l_e_a_s_e___m_a_n_a_g_e_m_e_n_t.html#autotoc_md375", [
        [ "TL;DR", "md_doc_2_r_e_l_e_a_s_e___m_a_n_a_g_e_m_e_n_t.html#autotoc_md376", null ],
        [ "Process Steps", "md_doc_2_r_e_l_e_a_s_e___m_a_n_a_g_e_m_e_n_t.html#autotoc_md377", null ]
      ] ]
    ] ],
    [ "Installation instructions", "md__i_n_s_t_a_l_l.html", [
      [ "Building and running with docker:", "md__i_n_s_t_a_l_l.html#autotoc_md380", null ],
      [ "Building manually:", "md__i_n_s_t_a_l_l.html#autotoc_md381", null ]
    ] ],
    [ "Guidance on How to Contribute", "md__c_o_n_t_r_i_b_u_t_i_n_g.html", [
      [ "Using the Issue Tracker", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md384", null ],
      [ "Changing the Code-Base", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md385", [
        [ "Summary", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md386", null ],
        [ "Getting Started", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md387", [
          [ "Optional: Git Hooks", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md388", null ]
        ] ],
        [ "Developing Changes", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md389", [
          [ "Work in a Dedicated Branch", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md390", null ],
          [ "Pushing Incremental Commits", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md391", null ],
          [ "Submitting Pull Requests", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md392", [
            [ "Guidelines for Pull Requests", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md393", null ],
            [ "PR Review and Requested Revisions", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md394", null ]
          ] ]
        ] ],
        [ "Keeping Forks Up to Date", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md395", [
          [ "A Rebase Strategy", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md396", null ],
          [ "Getting Remote Changes", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md397", [
            [ "For production Too", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md398", null ]
          ] ],
          [ "Rebasing Development Branches", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md399", [
            [ "Interactive Rebasing", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md400", null ]
          ] ],
          [ "Fixing Diverging Development Branches", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md401", null ]
        ] ]
      ] ]
    ] ],
    [ "Disclaimer", "md__t_e_r_m_s.html", [
      [ "Exceptions", "md__t_e_r_m_s.html#autotoc_md404", null ]
    ] ],
    [ "Testing", "md_test_2_r_e_a_d_m_e.html", [
      [ "Testing Frameworks", "md_test_2_r_e_a_d_m_e.html#autotoc_md406", [
        [ "Google Test", "md_test_2_r_e_a_d_m_e.html#autotoc_md407", [
          [ "(Re)generating CMake Project Buildsystem", "md_test_2_r_e_a_d_m_e.html#autotoc_md408", null ]
        ] ]
      ] ],
      [ "Executing Automated Tests", "md_test_2_r_e_a_d_m_e.html#autotoc_md409", [
        [ "C++ Tests", "md_test_2_r_e_a_d_m_e.html#autotoc_md410", [
          [ "Test Targets and Executables", "md_test_2_r_e_a_d_m_e.html#autotoc_md411", null ]
        ] ]
      ] ],
      [ "Creating New Automated Tests", "md_test_2_r_e_a_d_m_e.html#autotoc_md412", [
        [ "Adding Tests to CMake Builds", "md_test_2_r_e_a_d_m_e.html#autotoc_md413", null ],
        [ "Test Creation Rules of Thumb", "md_test_2_r_e_a_d_m_e.html#autotoc_md414", null ]
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
"class_h_y___interior_catchment.html#a8420468ac27e8ac940b80ff330683df3",
"class_simulation___time.html#a2a837cc1f07062ba0ae4a99401f11e76",
"classdata__access_1_1_optional_wrapped_data_provider.html#ab5c87312068edadb502830d0f0fa27ab",
"classgeojson_1_1_feature_base.html#a3ff99f3984c2097aedd6207a8f1eb5f8",
"classgeojson_1_1_j_s_o_n_property.html#abd96e16e7da59f6d4e98fd3abf375224",
"classgeojson_1_1_multi_line_string_feature.html#ae3da4d699f37811a450985f094b45bf8",
"classgeojson_1_1_multi_polygon_feature.html#ae4569552d1767dad95836c54a582658e",
"classgeojson_1_1_sentinel_feature.html",
"classmodels_1_1bmi_1_1_abstract_c_lib_bmi_adapter.html#a4b021125add57e1921bb4344d9ccea5f",
"classmodels_1_1bmi_1_1_bmi___c___adapter.html#a21a54597ad76207eab2bbaf0bcecc664",
"classmodels_1_1bmi_1_1_bmi___cpp___adapter.html#a97a27219b39d6de5a061e6ea0780711c",
"classnetwork_1_1_network.html#a04dbb9e0615fbb4db09d8c9fb4f68102",
"classngen_1_1serialization_1_1_file_backend_1_1_reader.html#a02cc8049465912595bdbf858f57ef5e5",
"classrealization_1_1_bmi___c___formulation.html#aa92f9bd07b2cdf99ce74367fd7988b1c",
"classrealization_1_1_bmi___cpp___formulation.html#accff79d755091811e806fc3759d6539f",
"classrealization_1_1_bmi___module___formulation.html#a3ed78c910127ec5115d71fcfe4d47896",
"classrealization_1_1_bmi___multi___formulation.html#a771443399df99023794dac96a2dd2d88",
"classrealization_1_1_formulation___manager.html#a158bc03a7f3200c6246f067b45af5546",
"functions_func_w.html",
"md_doc_2_d_i_s_t_r_i_b_u_t_e_d___p_r_o_c_e_s_s_i_n_g.html#autotoc_md245",
"namespacehy__features_1_1hydrolocation.html#a7fcfe40520bcda2cd5be66a5b5bbbc0ba087b9933bc9964f1267b02216f31ac03",
"namespacerealization.html#a6b6d6d33a273c0a3de61473e05881ef6",
"structmodels_1_1bmi_1_1protocols_1_1_context.html#aace653cfd38c514210cdd61b218378d8",
"structrealization_1_1config_1_1_config.html#a23d056dab8408cc9e6b0d4f6546294eb"
];

var SYNCONMSG = 'click to disable panel synchronisation';
var SYNCOFFMSG = 'click to enable panel synchronisation';