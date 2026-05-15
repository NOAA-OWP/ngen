#ifdef NGEN_BMI_PY_TESTS_ACTIVE

#include "gtest/gtest.h"

#include <exception>
#include <memory>
#include <string>

#include "utilities/python/InterpreterUtil.hpp"

using utils::ngenPy::InterpreterUtil;

class InterpreterUtilTest : public ::testing::Test {
private:
    // Keep the embedded Python interpreter alive across all tests in this
    // fixture; the interpreter is reference-counted and torn down once the
    // last shared_ptr is dropped.
    static std::shared_ptr<InterpreterUtil> interpreter;
};
std::shared_ptr<InterpreterUtil> InterpreterUtilTest::interpreter =
    InterpreterUtil::getInstance();

// Regression test for commit c928aa98: importTopLevelModule() exception safety.
//
// Old behavior:
//   importedTopLevelModules[name] = py::module_::import(name.c_str());
// operator[] inserted a default-constructed (NULL-pointered) py::object under
// `name` before the right-hand side was evaluated. When import() threw, the
// map kept that NULL entry. A subsequent getModule(name) call saw the key,
// took the "already imported" branch in isImported(), and returned the NULL
// handle silently -- masking the original failure and primed to crash the
// first time anything dereferenced it.
//
// New behavior: the map is mutated only after import() succeeds, so a repeat
// import of a bad name fails the same way every time.
//
// NOTE: The test will not actually fail over the old behavior when
// InterpreterUtil is built as C++17 or later, due to P0145 specifying
// evaluation order such that the older single-statement
// implementation of InterpreterUtil::importTopLevelModule() becomes
// exception-safe
TEST_F(InterpreterUtilTest, FailedImportDoesNotPoisonModuleMap) {
    auto interp = InterpreterUtil::getInstance();
    const std::string bogus_name = "ngen_definitely_not_a_real_module_xyzzy";

    // First attempt: the import must fail.
    ASSERT_THROW(interp->getModule(bogus_name), py::error_already_set);

    // Set up for the second attempt, by ensuring that `module` is not
    // going to be nullptr, and allowing us to better assert that
    // getModule() should not have returned nullptr
    py::object module;
    ASSERT_NO_THROW(module = interp->getModule("numpy"));
    ASSERT_NE(module.ptr(), nullptr);
    py::object module_saved = module;

    // Second attempt:
    //   - With the bug, the map carries a NULL entry for bogus_name,
    //     getModule() short-circuits via isImported(), returns silently,
    //     and this expectation fails (red).
    //   - With the fix, the map is untouched, getModule() re-attempts the
    //     import, and the exception is raised again (green).
    EXPECT_THROW(module = interp->getModule(bogus_name), py::error_already_set);
    EXPECT_EQ(module.ptr(), module_saved.ptr());
}

#endif  // NGEN_BMI_PY_TESTS_ACTIVE
