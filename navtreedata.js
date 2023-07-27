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
    [ "Next Gen Water Modeling Framework Prototype", "index.html#autotoc_md3", [
      [ "Structural Diagrams", "index.html#autotoc_md4", null ],
      [ "Dependencies", "index.html#autotoc_md5", null ],
      [ "Installation", "index.html#autotoc_md6", null ],
      [ "Configuration", "index.html#autotoc_md7", null ],
      [ "Usage", "index.html#autotoc_md8", null ],
      [ "How to test the software", "index.html#autotoc_md9", null ],
      [ "How to debug the software", "index.html#autotoc_md10", null ],
      [ "Known issues", "index.html#autotoc_md11", null ],
      [ "Getting help", "index.html#autotoc_md12", null ],
      [ "Getting involved", "index.html#autotoc_md13", null ],
      [ "Open source licensing info", "index.html#autotoc_md15", null ],
      [ "Credits and references", "index.html#autotoc_md17", null ]
    ] ],
    [ "CHANGELOG", "md__c_h_a_n_g_e_l_o_g.html", null ],
    [ "BMI External Models", "md_doc__b_m_i__m_o_d_e_l_s.html", [
      [ "Summary", "md_doc__b_m_i__m_o_d_e_l_s.html#autotoc_md24", null ],
      [ "Formulation Config", "md_doc__b_m_i__m_o_d_e_l_s.html#autotoc_md25", [
        [ "Required Parameters", "md_doc__b_m_i__m_o_d_e_l_s.html#autotoc_md26", null ],
        [ "Semi-Optional Parameters", "md_doc__b_m_i__m_o_d_e_l_s.html#autotoc_md28", null ],
        [ "Optional Parameters", "md_doc__b_m_i__m_o_d_e_l_s.html#autotoc_md29", null ]
      ] ],
      [ "BMI Models Written in C", "md_doc__b_m_i__m_o_d_e_l_s.html#autotoc_md30", [
        [ "BMI C Model As Shared Library", "md_doc__b_m_i__m_o_d_e_l_s.html#autotoc_md31", [
          [ "Dynamic Loading", "md_doc__b_m_i__m_o_d_e_l_s.html#autotoc_md32", null ]
        ] ],
        [ "BMI C CFE Example", "md_doc__b_m_i__m_o_d_e_l_s.html#autotoc_md33", null ],
        [ "BMI C Caveats", "md_doc__b_m_i__m_o_d_e_l_s.html#autotoc_md34", [
          [ "BMI C Activate/Deactivation Required in CMake Build", "md_doc__b_m_i__m_o_d_e_l_s.html#autotoc_md35", null ],
          [ "Additional Bootstrapping Function Needed", "md_doc__b_m_i__m_o_d_e_l_s.html#autotoc_md36", [
            [ "Why?", "md_doc__b_m_i__m_o_d_e_l_s.html#autotoc_md37", null ]
          ] ]
        ] ]
      ] ],
      [ "BMI Models Written in C++", "md_doc__b_m_i__m_o_d_e_l_s.html#autotoc_md38", [
        [ "BMI C++ Model As Shared Library", "md_doc__b_m_i__m_o_d_e_l_s.html#autotoc_md39", [
          [ "Dynamic Loading", "md_doc__b_m_i__m_o_d_e_l_s.html#autotoc_md40", null ],
          [ "Additional Bootstrapping Functions Needed", "md_doc__b_m_i__m_o_d_e_l_s.html#autotoc_md41", [
            [ "Why?", "md_doc__b_m_i__m_o_d_e_l_s.html#autotoc_md42", null ]
          ] ]
        ] ],
        [ "BMI C++ Example", "md_doc__b_m_i__m_o_d_e_l_s.html#autotoc_md43", null ]
      ] ],
      [ "BMI Models Written in Python", "md_doc__b_m_i__m_o_d_e_l_s.html#autotoc_md44", [
        [ "Enabling Python Integration", "md_doc__b_m_i__m_o_d_e_l_s.html#autotoc_md45", null ],
        [ "BMI Python Model as Package Class", "md_doc__b_m_i__m_o_d_e_l_s.html#autotoc_md46", null ],
        [ "BMI Python Example", "md_doc__b_m_i__m_o_d_e_l_s.html#autotoc_md47", null ]
      ] ],
      [ "BMI Models Written in Fortran", "md_doc__b_m_i__m_o_d_e_l_s.html#autotoc_md48", [
        [ "Enabling Fortran Integration", "md_doc__b_m_i__m_o_d_e_l_s.html#autotoc_md49", null ],
        [ "ISO C Binding Middleware", "md_doc__b_m_i__m_o_d_e_l_s.html#autotoc_md50", null ],
        [ "A Compiled Shared Library", "md_doc__b_m_i__m_o_d_e_l_s.html#autotoc_md51", [
          [ "Required Additional Fortran Registration Function", "md_doc__b_m_i__m_o_d_e_l_s.html#autotoc_md52", null ]
        ] ]
      ] ],
      [ "Multi-Module BMI Formulations", "md_doc__b_m_i__m_o_d_e_l_s.html#autotoc_md53", null ]
    ] ],
    [ "Project Builds", "md_doc__b_u_i_l_d_s__a_n_d__c_m_a_k_e.html", [
      [ "The TL;DR", "md_doc__b_u_i_l_d_s__a_n_d__c_m_a_k_e.html#autotoc_md55", null ],
      [ "Generating a Build System", "md_doc__b_u_i_l_d_s__a_n_d__c_m_a_k_e.html#autotoc_md56", [
        [ "Out-of-Source Building", "md_doc__b_u_i_l_d_s__a_n_d__c_m_a_k_e.html#autotoc_md57", null ],
        [ "Ignoring the Build Directory", "md_doc__b_u_i_l_d_s__a_n_d__c_m_a_k_e.html#autotoc_md58", null ],
        [ "Generation Commands", "md_doc__b_u_i_l_d_s__a_n_d__c_m_a_k_e.html#autotoc_md59", [
          [ "Generating from Build Directory", "md_doc__b_u_i_l_d_s__a_n_d__c_m_a_k_e.html#autotoc_md60", null ],
          [ "Generating from Project Root", "md_doc__b_u_i_l_d_s__a_n_d__c_m_a_k_e.html#autotoc_md61", null ],
          [ "Optional: Specifying Project Build Config Type", "md_doc__b_u_i_l_d_s__a_n_d__c_m_a_k_e.html#autotoc_md62", null ]
        ] ],
        [ "Regenerating", "md_doc__b_u_i_l_d_s__a_n_d__c_m_a_k_e.html#autotoc_md63", null ]
      ] ],
      [ "Building Targets", "md_doc__b_u_i_l_d_s__a_n_d__c_m_a_k_e.html#autotoc_md64", [
        [ "Building From Build Directory", "md_doc__b_u_i_l_d_s__a_n_d__c_m_a_k_e.html#autotoc_md65", null ],
        [ "Building From Source Directory", "md_doc__b_u_i_l_d_s__a_n_d__c_m_a_k_e.html#autotoc_md66", null ]
      ] ],
      [ "Dependencies", "md_doc__b_u_i_l_d_s__a_n_d__c_m_a_k_e.html#autotoc_md67", [
        [ "Build System Regeneration", "md_doc__b_u_i_l_d_s__a_n_d__c_m_a_k_e.html#autotoc_md68", null ],
        [ "Boost ENV Variable", "md_doc__b_u_i_l_d_s__a_n_d__c_m_a_k_e.html#autotoc_md69", null ]
      ] ],
      [ "Build Design", "md_doc__b_u_i_l_d_s__a_n_d__c_m_a_k_e.html#autotoc_md70", [
        [ "Library-Based Structure", "md_doc__b_u_i_l_d_s__a_n_d__c_m_a_k_e.html#autotoc_md71", null ],
        [ "Handling Changes", "md_doc__b_u_i_l_d_s__a_n_d__c_m_a_k_e.html#autotoc_md72", [
          [ "Adding New Subdirectories", "md_doc__b_u_i_l_d_s__a_n_d__c_m_a_k_e.html#autotoc_md73", null ]
        ] ]
      ] ]
    ] ],
    [ "Project Dependencies", "md_doc__d_e_p_e_n_d_e_n_c_i_e_s.html", [
      [ "Summary", "md_doc__d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md75", null ],
      [ "Details", "md_doc__d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md76", [
        [ "Google Test", "md_doc__d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md77", [
          [ "Setup", "md_doc__d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md78", null ],
          [ "Version Requirements", "md_doc__d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md79", null ]
        ] ],
        [ "C and C++ Compiler", "md_doc__d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md80", [
          [ "Setup", "md_doc__d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md81", null ],
          [ "Version Requirements", "md_doc__d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md82", [
            [ "GCC", "md_doc__d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md83", null ],
            [ "Clang", "md_doc__d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md84", null ]
          ] ]
        ] ],
        [ "CMake", "md_doc__d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md85", [
          [ "Setup", "md_doc__d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md86", null ],
          [ "Version Requirements", "md_doc__d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md87", null ]
        ] ],
        [ "Boost (Headers Only)", "md_doc__d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md88", [
          [ "Setup", "md_doc__d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md89", [
            [ "Setting <strong>BOOST_ROOT</strong>", "md_doc__d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md90", null ]
          ] ],
          [ "Version Requirements", "md_doc__d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md91", null ]
        ] ],
        [ "Udunits", "md_doc__d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md92", [
          [ "Version Requirements", "md_doc__d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md95", null ]
        ] ],
        [ "Python 3 Libraries", "md_doc__d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md96", [
          [ "Setup", "md_doc__d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md98", null ],
          [ "Version Requirements", "md_doc__d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md99", null ]
        ] ],
        [ "pybind11", "md_doc__d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md100", [
          [ "Setup", "md_doc__d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md102", null ],
          [ "Version Requirements", "md_doc__d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md103", null ]
        ] ],
        [ "The <tt>dmod.subsetservice</tt> Package", "md_doc__d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md104", [
          [ "Setup", "md_doc__d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md105", null ],
          [ "Version Requirements", "md_doc__d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md106", null ]
        ] ],
        [ "t-route", "md_doc__d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md107", [
          [ "Setup", "md_doc__d_e_p_e_n_d_e_n_c_i_e_s.html#autotoc_md108", null ]
        ] ]
      ] ]
    ] ],
    [ "Distributed Processing", "md_doc__d_i_s_t_r_i_b_u_t_e_d__p_r_o_c_e_s_s_i_n_g.html", [
      [ "Overview", "md_doc__d_i_s_t_r_i_b_u_t_e_d__p_r_o_c_e_s_s_i_n_g.html#autotoc_md110", null ],
      [ "Building the MPI-Enabled Nextgen Driver", "md_doc__d_i_s_t_r_i_b_u_t_e_d__p_r_o_c_e_s_s_i_n_g.html#autotoc_md111", null ],
      [ "Running the MPI-Enabled Nextgen Driver", "md_doc__d_i_s_t_r_i_b_u_t_e_d__p_r_o_c_e_s_s_i_n_g.html#autotoc_md112", [
        [ "Subdivided Hydrofabric", "md_doc__d_i_s_t_r_i_b_u_t_e_d__p_r_o_c_e_s_s_i_n_g.html#autotoc_md113", [
          [ "Driver Runtime Differences", "md_doc__d_i_s_t_r_i_b_u_t_e_d__p_r_o_c_e_s_s_i_n_g.html#autotoc_md114", null ],
          [ "File Names", "md_doc__d_i_s_t_r_i_b_u_t_e_d__p_r_o_c_e_s_s_i_n_g.html#autotoc_md115", null ],
          [ "On-the-fly Generation", "md_doc__d_i_s_t_r_i_b_u_t_e_d__p_r_o_c_e_s_s_i_n_g.html#autotoc_md116", null ]
        ] ],
        [ "Examples", "md_doc__d_i_s_t_r_i_b_u_t_e_d__p_r_o_c_e_s_s_i_n_g.html#autotoc_md117", [
          [ "Example 1 - Full Hydrofabric", "md_doc__d_i_s_t_r_i_b_u_t_e_d__p_r_o_c_e_s_s_i_n_g.html#autotoc_md118", null ],
          [ "Example 2 - Subdivided Hydrofabric", "md_doc__d_i_s_t_r_i_b_u_t_e_d__p_r_o_c_e_s_s_i_n_g.html#autotoc_md119", null ]
        ] ]
      ] ],
      [ "Partitioning Config Generator", "md_doc__d_i_s_t_r_i_b_u_t_e_d__p_r_o_c_e_s_s_i_n_g.html#autotoc_md120", null ]
    ] ],
    [ "Git Strategy", "md_doc__g_i_t__u_s_a_g_e.html", [
      [ "Branching Design", "md_doc__g_i_t__u_s_a_g_e.html#autotoc_md122", null ],
      [ "Contributing", "md_doc__g_i_t__u_s_a_g_e.html#autotoc_md123", [
        [ "Contributing TLDR;", "md_doc__g_i_t__u_s_a_g_e.html#autotoc_md124", null ],
        [ "Fork Consistency Requirements", "md_doc__g_i_t__u_s_a_g_e.html#autotoc_md125", null ],
        [ "Fork Setup Suggestions", "md_doc__g_i_t__u_s_a_g_e.html#autotoc_md126", null ]
      ] ],
      [ "Keeping Forks Up to Date", "md_doc__g_i_t__u_s_a_g_e.html#autotoc_md127", [
        [ "Setting the <strong>upstream</strong> Remote", "md_doc__g_i_t__u_s_a_g_e.html#autotoc_md128", null ],
        [ "Getting Upstream Changes", "md_doc__g_i_t__u_s_a_g_e.html#autotoc_md129", null ],
        [ "Rebasing Development Branches", "md_doc__g_i_t__u_s_a_g_e.html#autotoc_md130", null ],
        [ "Fixing Diverging Development Branches", "md_doc__g_i_t__u_s_a_g_e.html#autotoc_md131", null ]
      ] ]
    ] ],
    [ "Definitions", "md_doc_glossary.html", [
      [ "References", "md_doc_glossary.html#autotoc_md133", null ]
    ] ],
    [ "LSTM Model", "md_doc__l_s_t_m__m_o_d_e_l.html", [
      [ "Summary", "md_doc__l_s_t_m__m_o_d_e_l.html#autotoc_md135", null ],
      [ "Formulation Config", "md_doc__l_s_t_m__m_o_d_e_l.html#autotoc_md136", [
        [ "Required Parameters", "md_doc__l_s_t_m__m_o_d_e_l.html#autotoc_md137", null ]
      ] ]
    ] ],
    [ "MPI Remote Nexus", "md_doc__m_p_i__r_e_m_o_t_e__n_e_x_u_s.html", [
      [ "Summary", "md_doc__m_p_i__r_e_m_o_t_e__n_e_x_u_s.html#autotoc_md139", null ]
    ] ],
    [ "Precision guidelines for validation of NGEN code translation and upgrades", "md_doc__precision__guidelines_for__ngen__code__validation.html", null ],
    [ "Standards for Programming", "md_doc_programming_standards.html", [
      [ "Source Documentation", "md_doc_programming_standards.html#autotoc_md142", null ],
      [ "Naming", "md_doc_programming_standards.html#autotoc_md143", null ],
      [ "Other", "md_doc_programming_standards.html#autotoc_md146", [
        [ "Headers", "md_doc_programming_standards.html#autotoc_md147", null ],
        [ "Spaces vs Tabs", "md_doc_programming_standards.html#autotoc_md148", null ],
        [ "Braces", "md_doc_programming_standards.html#autotoc_md149", null ],
        [ "Line Width", "md_doc_programming_standards.html#autotoc_md150", null ]
      ] ]
    ] ],
    [ "Python Routing", "md_doc__p_y_t_h_o_n__r_o_u_t_i_n_g.html", [
      [ "Summary", "md_doc__p_y_t_h_o_n__r_o_u_t_i_n_g.html#autotoc_md152", null ],
      [ "Setup Virtual Environment", "md_doc__p_y_t_h_o_n__r_o_u_t_i_n_g.html#autotoc_md153", null ],
      [ "Install t-route", "md_doc__p_y_t_h_o_n__r_o_u_t_i_n_g.html#autotoc_md154", null ],
      [ "Installation Caveats", "md_doc__p_y_t_h_o_n__r_o_u_t_i_n_g.html#autotoc_md155", [
        [ "Compilers and Libraries", "md_doc__p_y_t_h_o_n__r_o_u_t_i_n_g.html#autotoc_md156", null ],
        [ "Default installation is in development mode (impacts macOS)", "md_doc__p_y_t_h_o_n__r_o_u_t_i_n_g.html#autotoc_md157", null ]
      ] ],
      [ "Using t-route with ngen", "md_doc__p_y_t_h_o_n__r_o_u_t_i_n_g.html#autotoc_md158", [
        [ "Realization Config", "md_doc__p_y_t_h_o_n__r_o_u_t_i_n_g.html#autotoc_md159", null ],
        [ "Routing Config", "md_doc__p_y_t_h_o_n__r_o_u_t_i_n_g.html#autotoc_md160", null ],
        [ "Configuration considerations for t-route with ngen", "md_doc__p_y_t_h_o_n__r_o_u_t_i_n_g.html#autotoc_md161", null ]
      ] ],
      [ "Running t-route separately with ngen output", "md_doc__p_y_t_h_o_n__r_o_u_t_i_n_g.html#autotoc_md162", null ],
      [ "Known issues", "md_doc__p_y_t_h_o_n__r_o_u_t_i_n_g.html#autotoc_md163", [
        [ "Cleanup of <tt>*.parquet</tt> files required", "md_doc__p_y_t_h_o_n__r_o_u_t_i_n_g.html#autotoc_md164", null ],
        [ "Bug in multiprocessing on macOS", "md_doc__p_y_t_h_o_n__r_o_u_t_i_n_g.html#autotoc_md165", null ]
      ] ]
    ] ],
    [ "Realization Configuration", "md_doc__r_e_a_l_i_z_a_t_i_o_n__c_o_n_f_i_g_u_r_a_t_i_o_n.html", null ],
    [ "<a href=\"https://docs.opengeospatial.org/is/14-111r6/14-111r6.html\" >HY_Features</a>", "md_doc_references.html", [
      [ "<a href=\"https://bmi-spec.readthedocs.io/en/latest/\" >BMI</a>", "md_doc_references.html#autotoc_md168", null ],
      [ "Formulations", "md_doc_references.html#autotoc_md169", null ],
      [ "HYMOD", "md_doc_references.html#autotoc_md170", null ],
      [ "T-shirt", "md_doc_references.html#autotoc_md171", null ],
      [ "GIUH", "md_doc_references.html#autotoc_md172", [
        [ "Modeling References", "md_doc_references.html#autotoc_md173", null ]
      ] ]
    ] ],
    [ "Installation instructions", "md__i_n_s_t_a_l_l.html", [
      [ "Building and running with docker:", "md__i_n_s_t_a_l_l.html#autotoc_md176", null ],
      [ "Building manually:", "md__i_n_s_t_a_l_l.html#autotoc_md177", null ]
    ] ],
    [ "Guidance on how to contribute", "md__c_o_n_t_r_i_b_u_t_i_n_g.html", [
      [ "Using the issue tracker", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md180", null ],
      [ "Changing the code-base", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md181", null ]
    ] ],
    [ "Disclaimer", "md__t_e_r_m_s.html", [
      [ "Exceptions", "md__t_e_r_m_s.html#autotoc_md184", null ]
    ] ],
    [ "Testing", "md_test__r_e_a_d_m_e.html", [
      [ "Testing Frameworks", "md_test__r_e_a_d_m_e.html#autotoc_md186", [
        [ "<strong>Google Test</strong>", "md_test__r_e_a_d_m_e.html#autotoc_md187", [
          [ "(Re)generating CMake Project Buildsystem", "md_test__r_e_a_d_m_e.html#autotoc_md188", null ]
        ] ]
      ] ],
      [ "Executing Automated Tests", "md_test__r_e_a_d_m_e.html#autotoc_md189", [
        [ "C++ Tests", "md_test__r_e_a_d_m_e.html#autotoc_md190", [
          [ "Test Targets and Executables", "md_test__r_e_a_d_m_e.html#autotoc_md191", null ]
        ] ]
      ] ],
      [ "Creating New Automated Tests", "md_test__r_e_a_d_m_e.html#autotoc_md192", [
        [ "Adding Tests to CMake Builds", "md_test__r_e_a_d_m_e.html#autotoc_md193", null ],
        [ "Test Creation Rules of Thumb", "md_test__r_e_a_d_m_e.html#autotoc_md194", null ]
      ] ]
    ] ],
    [ "Todo List", "todo.html", null ],
    [ "Namespaces", "namespaces.html", [
      [ "Namespace List", "namespaces.html", "namespaces_dup" ],
      [ "Namespace Members", "namespacemembers.html", [
        [ "All", "namespacemembers.html", null ],
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
        [ "Related Functions", "functions_rela.html", null ]
      ] ]
    ] ]
  ] ]
];

var NAVTREEINDEX =
[
"annotated.html",
"class_h_y___hydro_nexus.html#a11dc68437650f47aad75eadc071b51e5",
"class_reservoir_1_1_explicit___time_1_1_reservoir___outlet.html#aeffe8449c5cbe459c2bceedfcbb661e2",
"class_simulation___time.html#a2932772505ed5505173b0cb8208cd748",
"classdata__access_1_1_data_provider.html#a8254f58e64499bb4ae799ab120fd0934",
"classgeojson_1_1_collection_feature.html#a984573e0ad014345634d5e859846ef35",
"classgeojson_1_1_feature_collection.html#ac486986e8a46952a122e392539b86ed2",
"classgeojson_1_1_multi_line_string_feature.html#a3ff99f3984c2097aedd6207a8f1eb5f8",
"classgeojson_1_1_multi_polygon_feature.html#a77d7f52df0473ed087c9a72a7c66fce9",
"classgeojson_1_1_polygon_feature.html#a81ae3cf3eac8a0100bd0fcd49dc75db9",
"classhy__features_1_1_h_y___features.html#a3a28c4767f2f53b70d200148de764d9e",
"classmodels_1_1bmi_1_1_bmi___adapter.html#a2132b18abc07635c6c1e9cdfe7e186ce",
"classmodels_1_1bmi_1_1_bmi___c___adapter.html#ab59c92e91f3ebe1cf5eaceac16100043",
"classnetwork_1_1_network.html#a17193c5c4eeebbcbe2eeda9879e22e83",
"classrealization_1_1_bmi___c___formulation.html#ac24b58505cb49f6594fd79fec5005014",
"classrealization_1_1_bmi___cpp___formulation.html#ac96032fca42092ad95727dcbf1d02a7b",
"classrealization_1_1_bmi___module___formulation.html#a2e84d036f980988830aec7155e2e7ba8",
"classrealization_1_1_bmi___multi___formulation.html#a4b3db6bb593b3d3261ac5ed2f35684b3",
"classrealization_1_1_et___accountable.html#a786de4d8f78bddb751896ea9865c9a9a",
"classrealization_1_1_tshirt___realization.html#a12fad75c2e47f5da298e022429243199",
"functions_vars_b.html",
"namespacegeojson.html#a24198ccd74c3e63a59b0ec353e126352",
"structboost_1_1detail_1_1span__has__data.html#a5c7e354cb8cebd786875bbbdd06a9458",
"structevapotranspiration__options.html",
"structsolar__radiation__results.html#ac8347cf2ca96658b48cd6b7b571b77b1"
];

var SYNCONMSG = 'click to disable panel synchronisation';
var SYNCOFFMSG = 'click to enable panel synchronisation';