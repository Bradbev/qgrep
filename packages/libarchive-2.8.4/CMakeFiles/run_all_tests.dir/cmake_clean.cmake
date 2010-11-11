FILE(REMOVE_RECURSE
  "CMakeFiles/run_all_tests"
)

# Per-language clean rules from dependency scanning.
FOREACH(lang)
  INCLUDE(CMakeFiles/run_all_tests.dir/cmake_clean_${lang}.cmake OPTIONAL)
ENDFOREACH(lang)
