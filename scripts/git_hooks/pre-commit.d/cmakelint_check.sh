#!/usr/bin/env bash
CMAKE_LINT_BIN=/usr/bin/cmake-lint
function check_cmake_lint_exists() {
    if [[ ! -f $CMAKE_LINT_BIN ]]; then
      echo "cmake-lint does not exists"
      echo "please install cmake-lint using command 'sudo apt install cmake-format'"
      return 1
    else
      return 0
    fi
}
numErrors=0
errorList=()
function check() {
  if ! check_cmake_lint_exists; then
      return 1
  fi
  files=$(git diff --name-only --cached --diff-filter=ACMR)
  for file in $files; do
      if [[ $file = *CMakeLists.txt ]] || [[ $file = *.cmake ]]; then
        $CMAKE_LINT_BIN "$file"
        if [[ $? != 0 ]]; then
          ((numErrors=numErrors+1))
          errorList+=($file)
        fi
      fi
  done
  if [[ $numErrors != 0 ]]; then
    printf "\nERROR: $numErrors cmake files has errors, you can use the following commands:\n"
    printf "        cmake-lint thefile # show error message about the file\n"
    printf "FILES:\n"
    for file in $errorList; do
      printf "        $file\n"
    done
    exit 1
  else
    exit 0
  fi
}
if [[ $# == 0 ]]; then
  check
else
  echo "error invalid command"
  exit 1
fi