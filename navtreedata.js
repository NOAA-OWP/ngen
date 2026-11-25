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
    [ "NextGen Water Modeling Engine (ngen)", "index.html#autotoc_md33", [
      [ "Structural Diagrams", "index.html#autotoc_md34", null ],
      [ "Dependencies", "index.html#autotoc_md35", null ],
      [ "Installation", "index.html#autotoc_md36", null ],
      [ "Configuration", "index.html#autotoc_md37", null ],
      [ "Usage", "index.html#autotoc_md38", null ],
      [ "How to test the software", "index.html#autotoc_md39", null ],
      [ "How to debug the software", "index.html#autotoc_md40", null ],
      [ "Known issues", "index.html#autotoc_md41", null ],
      [ "Getting help", "index.html#autotoc_md42", null ],
      [ "Getting involved", "index.html#autotoc_md43", null ],
      [ "Open source licensing info", "index.html#autotoc_md45", null ],
      [ "Credits and references", "index.html#autotoc_md47", null ]
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
      [ "What Are Automated Tests and How to Start Them", "md_doc_2_a_u_t_o_m_a_t_e_d___t_e_s_t.html#autotoc_md56", null ],
      [ "What Code Tests Are Performed", "md_doc_2_a_u_t_o_m_a_t_e_d___t_e_s_t.html#autotoc_md57", null ],
      [ "Local Testing", "md_doc_2_a_u_t_o_m_a_t_e_d___t_e_s_t.html#autotoc_md58", null ],
      [ "What to Do When a Test Fail", "md_doc_2_a_u_t_o_m_a_t_e_d___t_e_s_t.html#autotoc_md59", null ]
    ] ],
    [ "BMI External Models", "md_doc_2_b_m_i___m_o_d_e_l_s.html", [
      [ "Summary", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md61", null ],
      [ "Formulation Config", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md62", [
        [ "Required Parameters", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md63", null ],
        [ "Semi-Optional Parameters", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md65", null ],
        [ "Optional Parameters", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md66", null ]
      ] ],
      [ "BMI Models Written in C", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md67", [
        [ "BMI C Model As Shared Library", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md68", [
          [ "Dynamic Loading", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md69", null ]
        ] ],
        [ "BMI C CFE Example", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md70", null ],
        [ "BMI C Caveats", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md71", [
          [ "BMI C Activate/Deactivation Required in CMake Build", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md72", null ],
          [ "Additional Bootstrapping Function Needed", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md73", [
            [ "Why?", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md74", null ]
          ] ]
        ] ]
      ] ],
      [ "BMI Models Written in C++", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md75", [
        [ "BMI C++ Model As Shared Library", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md76", [
          [ "Dynamic Loading", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md77", null ],
          [ "Additional Bootstrapping Functions Needed", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md78", [
            [ "Why?", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md79", null ]
          ] ]
        ] ],
        [ "BMI C++ Example", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md80", null ]
      ] ],
      [ "BMI Models Written in Python", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md81", [
        [ "Enabling Python Integration", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md82", null ],
        [ "BMI Python Model as Package Class", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md83", null ],
        [ "BMI Python Example", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md84", null ]
      ] ],
      [ "BMI Models Written in Fortran", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md85", [
        [ "Enabling Fortran Integration", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md86", null ],
        [ "ISO C Binding Middleware", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md87", null ],
        [ "A Compiled Shared Library", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md88", [
          [ "Required Additional Fortran Registration Function", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md89", null ]
        ] ]
      ] ],
      [ "Multi-Module BMI Formulations", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md90", [
        [ "Passing Variables Between Nested Formulations", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md91", [
          [ "How Values Are Orchestrated", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md92", null ],
          [ "Look-back and Default Values", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md93", [
            [ "Example Look-Back Config", "md_doc_2_b_m_i___m_o_d_e_l_s.html#autotoc_md94", null ]
          ] ]
        ] ]
      ] ]
    ] ],
    [ "ngen BMI Serialization Protocol", "md_doc_2_b_m_i___s_e_r_i_a_l_i_z_a_t_i_o_n___p_r_o_t_o_c_o_l.html", [
      [ "TODO or Not TODO", "md_doc_2_b_m_i___s_e_r_i_a_l_i_z_a_t_i_o_n___p_r_o_t_o_c_o_l.html#autotoc_md96", [
        [ "Conformance is use-case dependent", "md_doc_2_b_m_i___s_e_r_i_a_l_i_z_a_t_i_o_n___p_r_o_t_o_c_o_l.html#autotoc_md97", null ]
      ] ],
      [ "Reserved BMI Variables", "md_doc_2_b_m_i___s_e_r_i_a_l_i_z_a_t_i_o_n___p_r_o_t_o_c_o_l.html#autotoc_md99", [
        [ "Item list behavior", "md_doc_2_b_m_i___s_e_r_i_a_l_i_z_a_t_i_o_n___p_r_o_t_o_c_o_l.html#autotoc_md100", null ]
      ] ],
      [ "Unit Conventions for Non-Physical Variables", "md_doc_2_b_m_i___s_e_r_i_a_l_i_z_a_t_i_o_n___p_r_o_t_o_c_o_l.html#autotoc_md102", null ],
      [ "Model Developer Implementation Guide", "md_doc_2_b_m_i___s_e_r_i_a_l_i_z_a_t_i_o_n___p_r_o_t_o_c_o_l.html#autotoc_md104", [
        [ "Width of the size variable", "md_doc_2_b_m_i___s_e_r_i_a_l_i_z_a_t_i_o_n___p_r_o_t_o_c_o_l.html#autotoc_md105", null ],
        [ "Recommended: defensive payload format", "md_doc_2_b_m_i___s_e_r_i_a_l_i_z_a_t_i_o_n___p_r_o_t_o_c_o_l.html#autotoc_md106", null ],
        [ "Constraints and non-goals", "md_doc_2_b_m_i___s_e_r_i_a_l_i_z_a_t_i_o_n___p_r_o_t_o_c_o_l.html#autotoc_md107", null ],
        [ "Optional: GetValuePtr for zero-copy state access", "md_doc_2_b_m_i___s_e_r_i_a_l_i_z_a_t_i_o_n___p_r_o_t_o_c_o_l.html#autotoc_md108", null ],
        [ "Recommended: single field list drives save, restore, and size", "md_doc_2_b_m_i___s_e_r_i_a_l_i_z_a_t_i_o_n___p_r_o_t_o_c_o_l.html#autotoc_md109", null ],
        [ "Reference implementations", "md_doc_2_b_m_i___s_e_r_i_a_l_i_z_a_t_i_o_n___p_r_o_t_o_c_o_l.html#autotoc_md110", null ]
      ] ]
    ] ],
    [ "BMI Conventions", "md_doc_2_b_m_iconventions.html", [
      [ "Compliance with the BMI v2.0 Standard", "md_doc_2_b_m_iconventions.html#autotoc_md112", null ],
      [ "Time Control", "md_doc_2_b_m_iconventions.html#autotoc_md113", [
        [ "Set and Get Values Per Timestep", "md_doc_2_b_m_iconventions.html#autotoc_md114", null ],
        [ "update vs. update_until", "md_doc_2_b_m_iconventions.html#autotoc_md115", null ],
        [ "Time Units", "md_doc_2_b_m_iconventions.html#autotoc_md116", null ],
        [ "Model time begin", "md_doc_2_b_m_iconventions.html#autotoc_md117", null ],
        [ "Model time end", "md_doc_2_b_m_iconventions.html#autotoc_md118", null ]
      ] ],
      [ "Metadata", "md_doc_2_b_m_iconventions.html#autotoc_md119", [
        [ "Variable Units", "md_doc_2_b_m_iconventions.html#autotoc_md120", null ],
        [ "Variable Types and Sizes", "md_doc_2_b_m_iconventions.html#autotoc_md121", null ]
      ] ],
      [ "Input and output published variables", "md_doc_2_b_m_iconventions.html#autotoc_md122", [
        [ "Input/output variables and the time loop", "md_doc_2_b_m_iconventions.html#autotoc_md123", null ],
        [ "Validation of formulations for input variables", "md_doc_2_b_m_iconventions.html#autotoc_md124", null ],
        [ "Validation of formulations for output variables", "md_doc_2_b_m_iconventions.html#autotoc_md125", null ],
        [ "Calling set_value on output variables", "md_doc_2_b_m_iconventions.html#autotoc_md126", null ],
        [ "Calling get_value on input variables", "md_doc_2_b_m_iconventions.html#autotoc_md127", null ],
        [ "Input and output with the same name not permitted", "md_doc_2_b_m_iconventions.html#autotoc_md128", null ]
      ] ],
      [ "Unpublished variables", "md_doc_2_b_m_iconventions.html#autotoc_md129", [
        [ "Unknown/unexpected variables passed to set_value", "md_doc_2_b_m_iconventions.html#autotoc_md130", null ],
        [ "Must support variable metadata functions", "md_doc_2_b_m_iconventions.html#autotoc_md131", null ]
      ] ],
      [ "Array representation", "md_doc_2_b_m_iconventions.html#autotoc_md132", [
        [ "Contiguousness", "md_doc_2_b_m_iconventions.html#autotoc_md133", null ],
        [ "Zero-Indexing", "md_doc_2_b_m_iconventions.html#autotoc_md134", null ],
        [ "Layout and Flattening", "md_doc_2_b_m_iconventions.html#autotoc_md135", [
          [ "C ordering required", "md_doc_2_b_m_iconventions.html#autotoc_md136", null ],
          [ "Proper ordering example", "md_doc_2_b_m_iconventions.html#autotoc_md137", null ],
          [ "When this matters", "md_doc_2_b_m_iconventions.html#autotoc_md138", [
            [ "Fortran models using z,y,x index ordering", "md_doc_2_b_m_iconventions.html#autotoc_md139", null ],
            [ "Clarification for get_value_at_indices and set_value_at_indices", "md_doc_2_b_m_iconventions.html#autotoc_md140", null ],
            [ "Spatial data arrays for structured grids", "md_doc_2_b_m_iconventions.html#autotoc_md141", null ]
          ] ]
        ] ]
      ] ],
      [ "Grid metadata functions", "md_doc_2_b_m_iconventions.html#autotoc_md142", [
        [ "Required metadata functions", "md_doc_2_b_m_iconventions.html#autotoc_md143", null ],
        [ "Ranks and sizes for scalar values", "md_doc_2_b_m_iconventions.html#autotoc_md144", null ]
      ] ],
      [ "Specific ngen uses of unpublished variables", "md_doc_2_b_m_iconventions.html#autotoc_md145", [
        [ "The model_params list", "md_doc_2_b_m_iconventions.html#autotoc_md146", null ],
        [ "Grid sizing parameters", "md_doc_2_b_m_iconventions.html#autotoc_md147", [
          [ "ngen_grid_N_shape", "md_doc_2_b_m_iconventions.html#autotoc_md148", null ],
          [ "ngen_grid_N_spacing", "md_doc_2_b_m_iconventions.html#autotoc_md149", null ],
          [ "ngen_grid_N_origin", "md_doc_2_b_m_iconventions.html#autotoc_md150", null ]
        ] ]
      ] ]
    ] ],
    [ "Project Builds", "md_doc_2_b_u_i_l_d_s___a_n_d___c_m_a_k_e.html", [
      [ "The TL;DR", "md_doc_2_b_u_i_l_d_s___a_n_d___c_m_a_k_e.html#autotoc_md152", null ],
      [ "Generating a Build System", "md_doc_2_b_u_i_l_d_s___a_n_d___c_m_a_k_e.html#autotoc_md153", [
        [ "Out-of-Source Building", "md_doc_2_b_u_i_l_d_s___a_n_d___c_m_a_k_e.html#autotoc_md154", null ],
        [ "Ignoring the Build Directory", "md_doc_2_b_u_i_l_d_s___a_n_d___c_m_a_k_e.html#autotoc_md155", null ],
        [ "Generation Commands", "md_doc_2_b_u_i_l_d_s___a_n_d___c_m_a_k_e.html#autotoc_md156", [
          [ "Generating from Build Directory", "md_doc_2_b_u_i_l_d_s___a_n_d___c_m_a_k_e.html#autotoc_md157", null ],
          [ "Generating from Project Root", "md_doc_2_b_u_i_l_d_s___a_n_d___c_m_a_k_e.html#autotoc_md158", null ],
          [ "Optional: Specifying Project Build Config Type", "md_doc_2_b_u_i_l_d_s___a_n_d___c_m_a_k_e.html#autotoc_md159", null ]
        ] ],
        [ "Regenerating", "md_doc_2_b_u_i_l_d_s___a_n_d___c_m_a_k_e.html#autotoc_md160", null ]
      ] ],
      [ "Building Targets", "md_doc_2_b_u_i_l_d_s___a_n_d___c_m_a_k_e.html#autotoc_md161", [
        [ "Building From Build Directory", "md_doc_2_b_u_i_l_d_s___a_n_d___c_m_a_k_e.html#autotoc_md162", null ],
        [ "Building From Source Directory", "md_doc_2_b_u_i_l_d_s___a_n_d___c_m_a_k_e.html#autotoc_md163", null ]
      ] ],
      [ "Dependencies", "md_doc_2_b_u_i_l_d_s___a_n_d___c_m_a_k_e.html#autotoc_md164", [
        [ "Build System Regeneration", "md_doc_2_b_u_i_l_d_s___a_n_d___c_m_a_k_e.html#autotoc_md165", null ],
        [ "Boost ENV Variable", "md_doc_2_b_u_i_l_d_s___a_n_d___c_m_a_k_e.html#autotoc_md166", null ]
      ] ],
      [ "Build Design", "md_doc_2_b_u_i_l_d_s___a_n_d___c_m_a_k_e.html#autotoc_md167", [
        [ "Library-Based Structure", "md_doc_2_b_u_i_l_d_s___a_n_d___c_m_a_k_e.html#autotoc_md168", null ],
        [ "Handling Changes", "md_doc_2_b_u_i_l_d_s___a_n_d___c_m_a_k_e.html#autotoc_md169", [
          [ "Adding New Subdirectories", "md_doc_2_b_u_i_l_d_s___a_n_d___c_m_a_k_e.html#autotoc_md170", null ]
        ] ]
      ] ]
    ] ],
    [ "Project Dependencies", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html", [
      [ "Summary", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md172", null ],
      [ "Details", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md173", [
        [ "Google Test", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md174", [
          [ "Setup", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md175", null ],
          [ "Version Requirements", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md176", null ]
        ] ],
        [ "C and C++ Compiler", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md177", [
          [ "Setup", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md178", null ],
          [ "Version Requirements", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md179", [
            [ "GCC", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md180", null ],
            [ "Clang", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md181", null ]
          ] ]
        ] ],
        [ "CMake", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md182", [
          [ "Setup", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md183", null ],
          [ "Version Requirements", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md184", null ]
        ] ],
        [ "Boost (Headers Only)", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md185", [
          [ "Setup", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md186", [
            [ "Setting BOOST_ROOT", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md187", null ]
          ] ],
          [ "Version Requirements", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md188", null ]
        ] ],
        [ "Udunits", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md189", [
          [ "Version Requirements", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md192", null ]
        ] ],
        [ "Python 3 Libraries", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md193", [
          [ "Setup", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md195", null ],
          [ "Version Requirements", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md196", null ]
        ] ],
        [ "pybind11", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md197", [
          [ "Setup", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md199", null ],
          [ "Version Requirements", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md200", null ]
        ] ],
        [ "t-route", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md201", [
          [ "Setup", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md202", null ]
        ] ],
        [ "NetCDF Libraries", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md203", [
          [ "Setup", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md204", null ],
          [ "Parallel NetCDF", "md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md205", null ]
        ] ]
      ] ]
    ] ],
    [ "Distributed Processing", "md_doc_2_d_i_s_t_r_i_b_u_t_e_d___p_r_o_c_e_s_s_i_n_g.html", [
      [ "Overview", "md_doc_2_d_i_s_t_r_i_b_u_t_e_d___p_r_o_c_e_s_s_i_n_g.html#autotoc_md207", null ],
      [ "Building the MPI-Enabled Nextgen Driver", "md_doc_2_d_i_s_t_r_i_b_u_t_e_d___p_r_o_c_e_s_s_i_n_g.html#autotoc_md208", null ],
      [ "Running the MPI-Enabled Nextgen Driver", "md_doc_2_d_i_s_t_r_i_b_u_t_e_d___p_r_o_c_e_s_s_i_n_g.html#autotoc_md209", [
        [ "Automatic Partitioning", "md_doc_2_d_i_s_t_r_i_b_u_t_e_d___p_r_o_c_e_s_s_i_n_g.html#autotoc_md210", null ],
        [ "Examples", "md_doc_2_d_i_s_t_r_i_b_u_t_e_d___p_r_o_c_e_s_s_i_n_g.html#autotoc_md211", [
          [ "Example 1 - Partition Config", "md_doc_2_d_i_s_t_r_i_b_u_t_e_d___p_r_o_c_e_s_s_i_n_g.html#autotoc_md212", null ],
          [ "Example 2 - Automatic Partitioning", "md_doc_2_d_i_s_t_r_i_b_u_t_e_d___p_r_o_c_e_s_s_i_n_g.html#autotoc_md213", null ]
        ] ]
      ] ],
      [ "Partitioning Config Generator", "md_doc_2_d_i_s_t_r_i_b_u_t_e_d___p_r_o_c_e_s_s_i_n_g.html#autotoc_md214", null ]
    ] ],
    [ "Git Strategy", "md_doc_2_g_i_t___u_s_a_g_e.html", [
      [ "Branching Model", "md_doc_2_g_i_t___u_s_a_g_e.html#autotoc_md216", [
        [ "Feature Branches from master", "md_doc_2_g_i_t___u_s_a_g_e.html#autotoc_md217", null ],
        [ "Relating production, master, and Release Branches", "md_doc_2_g_i_t___u_s_a_g_e.html#autotoc_md218", null ]
      ] ],
      [ "Contributing", "md_doc_2_g_i_t___u_s_a_g_e.html#autotoc_md219", null ],
      [ "Optional: Setting Up Hook Scripts", "md_doc_2_g_i_t___u_s_a_g_e.html#autotoc_md220", null ]
    ] ],
    [ "Definitions", "md_doc_2glossary.html", [
      [ "References", "md_doc_2glossary.html#autotoc_md222", null ]
    ] ],
    [ "NextGen on CONUS", "md_doc_2_next_gen___o_n___c_o_n_u_s.html", [
      [ "Summary", "md_doc_2_next_gen___o_n___c_o_n_u_s.html#autotoc_md224", null ],
      [ "Download the Codes", "md_doc_2_next_gen___o_n___c_o_n_u_s.html#autotoc_md225", null ],
      [ "Setting up the Environment", "md_doc_2_next_gen___o_n___c_o_n_u_s.html#autotoc_md226", null ],
      [ "Build the Executable", "md_doc_2_next_gen___o_n___c_o_n_u_s.html#autotoc_md227", null ],
      [ "CONUS Hydrofabric", "md_doc_2_next_gen___o_n___c_o_n_u_s.html#autotoc_md228", null ],
      [ "Prepare the Input Data", "md_doc_2_next_gen___o_n___c_o_n_u_s.html#autotoc_md229", null ],
      [ "Build the Realization Configurations", "md_doc_2_next_gen___o_n___c_o_n_u_s.html#autotoc_md230", null ],
      [ "Run Computations with Submodules", "md_doc_2_next_gen___o_n___c_o_n_u_s.html#autotoc_md231", null ],
      [ "Resource Usage", "md_doc_2_next_gen___o_n___c_o_n_u_s.html#autotoc_md232", null ],
      [ "Run Computation with Topmodel", "md_doc_2_next_gen___o_n___c_o_n_u_s.html#autotoc_md233", null ],
      [ "Run Computation with Routing", "md_doc_2_next_gen___o_n___c_o_n_u_s.html#autotoc_md234", null ]
    ] ],
    [ "ngen Serialization", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html", [
      [ "Engine behavior with non-conforming models", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md237", null ],
      [ "Wire Format", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md239", [
        [ "Two timestamps, not one", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md240", null ],
        [ "Wire-format version", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md241", null ],
        [ "Key properties", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md242", null ]
      ] ],
      [ "Save Protocol: NgenSerializationProtocol", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md244", [
        [ "Configuration", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md245", null ],
        [ "Support detection", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md246", null ],
        [ "Operation", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md247", null ],
        [ "Size variable width — how the engine composes with narrower models", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md248", null ]
      ] ],
      [ "Restore Protocol: NgenDeserializationProtocol", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md250", [
        [ "Configuration", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md251", null ],
        [ "Split save/restore paths", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md252", null ],
        [ "Support detection", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md253", null ],
        [ "Operation", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md254", null ]
      ] ],
      [ "Combined save + restore example", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md256", null ],
      [ "Restart semantics: hot start and resume", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md258", [
        [ "Hot start", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md259", null ],
        [ "Resume", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md260", null ],
        [ "Guaranteed consistent restore", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md261", null ],
        [ "Modules with no state", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md262", null ],
        [ "\"latest\"", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md263", null ],
        [ "Not supported yet", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md264", null ]
      ] ],
      [ "Identity Convention: compound_id()", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md266", [
        [ "How the compound gets set", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md267", null ]
      ] ],
      [ "Engine Integration", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md269", [
        [ "Save — per-timestep from Layer", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md270", null ],
        [ "Restore — once at the end of create_model", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md271", null ],
        [ "Lifecycle summary", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md272", null ]
      ] ],
      [ "Worked Example — multi-BMI formulation with save + restore", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md274", null ],
      [ "Realization-Level (Global) Configuration", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md276", [
        [ "Top-level placement", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md277", null ],
        [ "Precedence", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md278", null ],
        [ "Absent block", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md279", null ],
        [ "Path templating", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md280", null ]
      ] ],
      [ "Scaling Considerations", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md282", [
        [ "On-disk format", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md283", null ],
        [ "Save cost model", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md284", null ],
        [ "Restore cost model", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md285", null ],
        [ "id_subset for memory-bounded restores", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md286", null ],
        [ "Thread and process safety", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md287", null ],
        [ "When the protocol is not the right tool", "md_doc_2_n_g_e_n___s_e_r_i_a_l_i_z_a_t_i_o_n.html#autotoc_md288", null ]
      ] ]
    ] ],
    [ "Precision guidelines for validation of NGEN code translation and upgrades", "md_doc_2_precision___guidelines__for___ngen___code___validation.html", null ],
    [ "Standards for Programming", "md_doc_2programming__standards.html", [
      [ "Source Documentation", "md_doc_2programming__standards.html#autotoc_md291", null ],
      [ "Naming", "md_doc_2programming__standards.html#autotoc_md292", null ],
      [ "Other", "md_doc_2programming__standards.html#autotoc_md295", [
        [ "Headers", "md_doc_2programming__standards.html#autotoc_md296", null ],
        [ "Spaces vs Tabs", "md_doc_2programming__standards.html#autotoc_md297", null ],
        [ "Braces", "md_doc_2programming__standards.html#autotoc_md298", null ],
        [ "Line Width", "md_doc_2programming__standards.html#autotoc_md299", null ]
      ] ]
    ] ],
    [ "Python Routing", "md_doc_2_p_y_t_h_o_n___r_o_u_t_i_n_g.html", [
      [ "Summary", "md_doc_2_p_y_t_h_o_n___r_o_u_t_i_n_g.html#autotoc_md301", null ],
      [ "Setup Virtual Environment", "md_doc_2_p_y_t_h_o_n___r_o_u_t_i_n_g.html#autotoc_md302", null ],
      [ "Install t-route", "md_doc_2_p_y_t_h_o_n___r_o_u_t_i_n_g.html#autotoc_md303", null ],
      [ "Installation Caveats", "md_doc_2_p_y_t_h_o_n___r_o_u_t_i_n_g.html#autotoc_md304", [
        [ "Compilers and Libraries", "md_doc_2_p_y_t_h_o_n___r_o_u_t_i_n_g.html#autotoc_md305", null ],
        [ "Default installation is in development mode (impacts macOS)", "md_doc_2_p_y_t_h_o_n___r_o_u_t_i_n_g.html#autotoc_md306", null ],
        [ "Some tips on installation if you run into issues", "md_doc_2_p_y_t_h_o_n___r_o_u_t_i_n_g.html#autotoc_md307", null ]
      ] ],
      [ "Using t-route with ngen", "md_doc_2_p_y_t_h_o_n___r_o_u_t_i_n_g.html#autotoc_md308", [
        [ "Realization Config", "md_doc_2_p_y_t_h_o_n___r_o_u_t_i_n_g.html#autotoc_md309", null ],
        [ "Routing Config", "md_doc_2_p_y_t_h_o_n___r_o_u_t_i_n_g.html#autotoc_md310", null ],
        [ "Configuration considerations for t-route with ngen", "md_doc_2_p_y_t_h_o_n___r_o_u_t_i_n_g.html#autotoc_md311", null ]
      ] ],
      [ "Running t-route separately with ngen output", "md_doc_2_p_y_t_h_o_n___r_o_u_t_i_n_g.html#autotoc_md312", null ],
      [ "Known issues", "md_doc_2_p_y_t_h_o_n___r_o_u_t_i_n_g.html#autotoc_md313", [
        [ "Cleanup of *.parquet files required", "md_doc_2_p_y_t_h_o_n___r_o_u_t_i_n_g.html#autotoc_md314", null ],
        [ "Bug in multiprocessing on macOS", "md_doc_2_p_y_t_h_o_n___r_o_u_t_i_n_g.html#autotoc_md315", null ]
      ] ]
    ] ],
    [ "Realization Configuration", "md_doc_2_r_e_a_l_i_z_a_t_i_o_n___c_o_n_f_i_g_u_r_a_t_i_o_n.html", [
      [ "Description and Top-Level Structure", "md_doc_2_r_e_a_l_i_z_a_t_i_o_n___c_o_n_f_i_g_u_r_a_t_i_o_n.html#autotoc_md317", [
        [ "Required Top-Level Keys", "md_doc_2_r_e_a_l_i_z_a_t_i_o_n___c_o_n_f_i_g_u_r_a_t_i_o_n.html#autotoc_md318", null ],
        [ "Optional Top-Level Keys", "md_doc_2_r_e_a_l_i_z_a_t_i_o_n___c_o_n_f_i_g_u_r_a_t_i_o_n.html#autotoc_md319", [
          [ "output", "md_doc_2_r_e_a_l_i_z_a_t_i_o_n___c_o_n_f_i_g_u_r_a_t_i_o_n.html#autotoc_md320", null ],
          [ "output_root", "md_doc_2_r_e_a_l_i_z_a_t_i_o_n___c_o_n_f_i_g_u_r_a_t_i_o_n.html#autotoc_md321", null ],
          [ "disable_catchment_output", "md_doc_2_r_e_a_l_i_z_a_t_i_o_n___c_o_n_f_i_g_u_r_a_t_i_o_n.html#autotoc_md322", null ],
          [ "per_formulation_nexus_files", "md_doc_2_r_e_a_l_i_z_a_t_i_o_n___c_o_n_f_i_g_u_r_a_t_i_o_n.html#autotoc_md323", null ],
          [ "catchments", "md_doc_2_r_e_a_l_i_z_a_t_i_o_n___c_o_n_f_i_g_u_r_a_t_i_o_n.html#autotoc_md324", null ],
          [ "routing", "md_doc_2_r_e_a_l_i_z_a_t_i_o_n___c_o_n_f_i_g_u_r_a_t_i_o_n.html#autotoc_md325", null ]
        ] ],
        [ "Examples of Top-Level Structure", "md_doc_2_r_e_a_l_i_z_a_t_i_o_n___c_o_n_f_i_g_u_r_a_t_i_o_n.html#autotoc_md326", null ]
      ] ],
      [ "The Global Section", "md_doc_2_r_e_a_l_i_z_a_t_i_o_n___c_o_n_f_i_g_u_r_a_t_i_o_n.html#autotoc_md327", null ],
      [ "The Time Section", "md_doc_2_r_e_a_l_i_z_a_t_i_o_n___c_o_n_f_i_g_u_r_a_t_i_o_n.html#autotoc_md328", null ],
      [ "Individual Catchments", "md_doc_2_r_e_a_l_i_z_a_t_i_o_n___c_o_n_f_i_g_u_r_a_t_i_o_n.html#autotoc_md329", null ],
      [ "The Routing Section", "md_doc_2_r_e_a_l_i_z_a_t_i_o_n___c_o_n_f_i_g_u_r_a_t_i_o_n.html#autotoc_md330", null ],
      [ "A Full Example", "md_doc_2_r_e_a_l_i_z_a_t_i_o_n___c_o_n_f_i_g_u_r_a_t_i_o_n.html#autotoc_md331", null ],
      [ "A Note on BMI Models", "md_doc_2_r_e_a_l_i_z_a_t_i_o_n___c_o_n_f_i_g_u_r_a_t_i_o_n.html#autotoc_md332", null ]
    ] ],
    [ "<a href=\"https://docs.opengeospatial.org/is/14-111r6/14-111r6.html\" >HY_Features</a>", "md_doc_2references.html", [
      [ "BMI", "md_doc_2references.html#autotoc_md334", null ],
      [ "Formulations", "md_doc_2references.html#autotoc_md335", null ],
      [ "HYMOD", "md_doc_2references.html#autotoc_md336", null ],
      [ "T-shirt", "md_doc_2references.html#autotoc_md337", null ],
      [ "GIUH", "md_doc_2references.html#autotoc_md338", [
        [ "Modeling References", "md_doc_2references.html#autotoc_md339", null ]
      ] ]
    ] ],
    [ "Release Management", "md_doc_2_r_e_l_e_a_s_e___m_a_n_a_g_e_m_e_n_t.html", [
      [ "The Release Process", "md_doc_2_r_e_l_e_a_s_e___m_a_n_a_g_e_m_e_n_t.html#autotoc_md341", [
        [ "TL;DR", "md_doc_2_r_e_l_e_a_s_e___m_a_n_a_g_e_m_e_n_t.html#autotoc_md342", null ],
        [ "Process Steps", "md_doc_2_r_e_l_e_a_s_e___m_a_n_a_g_e_m_e_n_t.html#autotoc_md343", null ]
      ] ]
    ] ],
    [ "Output Variable Units from Running Model Engine", "md_doc_2_u_n_i_t_s___i_n___next_gen.html", [
      [ "Summary", "md_doc_2_u_n_i_t_s___i_n___next_gen.html#autotoc_md345", null ],
      [ "CFE Model", "md_doc_2_u_n_i_t_s___i_n___next_gen.html#autotoc_md346", null ],
      [ "PET Model", "md_doc_2_u_n_i_t_s___i_n___next_gen.html#autotoc_md347", null ],
      [ "Noah OWP Modular", "md_doc_2_u_n_i_t_s___i_n___next_gen.html#autotoc_md348", null ],
      [ "Topmodel", "md_doc_2_u_n_i_t_s___i_n___next_gen.html#autotoc_md349", null ]
    ] ],
    [ "Installation instructions", "md__i_n_s_t_a_l_l.html", [
      [ "Building and running with docker:", "md__i_n_s_t_a_l_l.html#autotoc_md352", null ],
      [ "Building manually:", "md__i_n_s_t_a_l_l.html#autotoc_md353", null ]
    ] ],
    [ "Guidance on How to Contribute", "md__c_o_n_t_r_i_b_u_t_i_n_g.html", [
      [ "Using the Issue Tracker", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md356", null ],
      [ "Changing the Code-Base", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md357", [
        [ "Summary", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md358", null ],
        [ "Getting Started", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md359", [
          [ "Optional: Git Hooks", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md360", null ]
        ] ],
        [ "Developing Changes", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md361", [
          [ "Work in a Dedicated Branch", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md362", null ],
          [ "Pushing Incremental Commits", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md363", null ],
          [ "Submitting Pull Requests", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md364", [
            [ "Guidelines for Pull Requests", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md365", null ],
            [ "PR Review and Requested Revisions", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md366", null ]
          ] ]
        ] ],
        [ "Keeping Forks Up to Date", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md367", [
          [ "A Rebase Strategy", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md368", null ],
          [ "Getting Remote Changes", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md369", [
            [ "For production Too", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md370", null ]
          ] ],
          [ "Rebasing Development Branches", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md371", [
            [ "Interactive Rebasing", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md372", null ]
          ] ],
          [ "Fixing Diverging Development Branches", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md373", null ]
        ] ]
      ] ]
    ] ],
    [ "Disclaimer", "md__t_e_r_m_s.html", [
      [ "Exceptions", "md__t_e_r_m_s.html#autotoc_md376", null ]
    ] ],
    [ "Testing", "md_test_2_r_e_a_d_m_e.html", [
      [ "Testing Frameworks", "md_test_2_r_e_a_d_m_e.html#autotoc_md378", [
        [ "Google Test", "md_test_2_r_e_a_d_m_e.html#autotoc_md379", [
          [ "(Re)generating CMake Project Buildsystem", "md_test_2_r_e_a_d_m_e.html#autotoc_md380", null ]
        ] ]
      ] ],
      [ "Executing Automated Tests", "md_test_2_r_e_a_d_m_e.html#autotoc_md381", [
        [ "C++ Tests", "md_test_2_r_e_a_d_m_e.html#autotoc_md382", [
          [ "Test Targets and Executables", "md_test_2_r_e_a_d_m_e.html#autotoc_md383", null ]
        ] ]
      ] ],
      [ "Creating New Automated Tests", "md_test_2_r_e_a_d_m_e.html#autotoc_md384", [
        [ "Adding Tests to CMake Builds", "md_test_2_r_e_a_d_m_e.html#autotoc_md385", null ],
        [ "Test Creation Rules of Thumb", "md_test_2_r_e_a_d_m_e.html#autotoc_md386", null ]
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
"class_null_forcing_provider.html#af1886f5b162417aa407ef9c8598d5eb8",
"classdata__access_1_1_deferred_wrapped_provider.html#a262de984f51ed9ce1ae01aacd8e627f9",
"classgeojson_1_1_collection_feature.html#a98fe6bee4701b344c4fa65378fd24552",
"classgeojson_1_1_feature_collection.html#ac21acb826a2c6943192936ff54412511",
"classgeojson_1_1_multi_line_string_feature.html#a14890e668e6d6b49c0f4b9c0e03542f4",
"classgeojson_1_1_multi_polygon_feature.html#a15ae69e7a5b7bb92995a3b52c8f2c51d",
"classgeojson_1_1_polygon_feature.html#a15e8c3693a636453b0af958337cb516f",
"classmodels_1_1bmi_1_1_abstract_c_lib_bmi_adapter.html#a6bead8b75c803a2fd51f4df6f6f91627",
"classmodels_1_1bmi_1_1_bmi___c___adapter.html#a2d15cade3652d580838f0b74c8e0f5ba",
"classmodels_1_1bmi_1_1_bmi___cpp___adapter.html#aaa7231e53236d33b6fdcd0115593a895",
"classmultithreading_1_1per__thread.html#ae922b2a6965b55860613eb2514f3d3dc",
"classngen_1_1hydrofabric_1_1_geo_package_hydrofabric_reader.html#abc077c82c85f7fa1fe6574826167e544",
"classngen_1_1sqlite_1_1database.html#aacd197f5b23451791e7f6ea9b882f4b7",
"classrealization_1_1_bmi___cpp___formulation.html#a1efc460f907baec4e2b1d59639c14d9b",
"classrealization_1_1_bmi___formulation.html#a93fab1a9af5c9d1917219528bec89894",
"classrealization_1_1_bmi___module___formulation.html#ad20a0abc6e06c670c587d7292494d96b",
"classrealization_1_1_bmi___var___details.html#ae233bc01243910c9893890b3ca68a3f9",
"functions_b.html",
"md_doc_2_d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md185",
"namespacegeojson.html#a9c216b34a0daddc5c999f512e06125ed",
"namespacengen_1_1geopackage.html#afd11eef2894e7e6e837c5f7ff4d3286a",
"struct_grid_data_selector.html#ae08b218bb235955417417fbf9113f158",
"structngen_1_1_layer_description.html#a4ce3bc73676b15cc43b07c81277b48d6",
"structrealization_1_1config_1_1_layer.html#a4f13c6328c7fbff19f3fe0a7ddfa37a5"
];

var SYNCONMSG = 'click to disable panel synchronisation';
var SYNCOFFMSG = 'click to enable panel synchronisation';