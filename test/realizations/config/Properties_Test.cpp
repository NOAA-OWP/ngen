#include <gtest/gtest.h>

#include <string>

#include "JSONProperty.hpp"
#include "realizations/config/properties.hpp"

using realization::config::pattern_substitution;

namespace {
    std::string substitute(const std::string& value, const std::string& pattern, const std::string& replacement)
    {
        geojson::PropertyMap properties;
        properties.emplace("key", geojson::JSONProperty("key", value));
        pattern_substitution(properties, "key", pattern, replacement);
        return properties.at("key").as_string();
    }

    struct Case {
        const char* value;
        const char* pattern;
        const char* replacement;
        const char* expected;
    };
}

// Patterns of every length must be replaced in full, regardless of how
// long a `char*` is on the build platform.
TEST(Properties_Test, PatternSubstitution)
{
    const Case cases[] =
    {
        //  value                          pattern          replacement  expected
          { "cfg_{{id}}.txt"             , "{{id}}"       , "cat-67"   , "cfg_cat-67.txt"        }
        , { "{{id}}/cfg_{{id}}.txt"      , "{{id}}"       , "cat-67"   , "cat-67/cfg_cat-67.txt" }
        , { "cfg.txt"                    , "{{id}}"       , "cat-67"   , "cfg.txt"               }
        , { "cfg_{}.txt"                 , "{}"           , "cat-67"   , "cfg_cat-67.txt"        }
        , { "cfg_{{x}}.txt"              , "{{x}}"        , "cat-67"   , "cfg_cat-67.txt"        }
        , { "cfg_{{catchment}}.txt"      , "{{catchment}}", "cat-67"   , "cfg_cat-67.txt"        }
        , { "{{catchment}}_{{catchment}}", "{{catchment}}", "c"        , "c_c"                   }
    };

    for (const auto& c : cases) {
        EXPECT_EQ(substitute(c.value, c.pattern, c.replacement), c.expected)
            << "value=\"" << c.value << "\" pattern=\"" << c.pattern << "\"";
    }
}

TEST(Properties_Test, PatternSubstitutionMissingKey)
{
    geojson::PropertyMap properties;
    properties.emplace("other", geojson::JSONProperty("other", std::string("{{id}}")));
    pattern_substitution(properties, "key", "{{id}}", "cat-67");
    EXPECT_EQ(properties.count("key"), 0);
    EXPECT_EQ(properties.at("other").as_string(), "{{id}}");
}

TEST(Properties_Test, PatternSubstitutionNonString)
{
    geojson::PropertyMap properties;
    properties.emplace("key", geojson::JSONProperty("key", 42));
    pattern_substitution(properties, "key", "{{id}}", "cat-67");
    EXPECT_EQ(properties.at("key").as_natural_number(), 42);
}
