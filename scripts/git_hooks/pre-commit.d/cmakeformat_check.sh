#!/usr/bin/env bash
CMAKE_FORMAT_BIN=/usr/bin/cmake-format
function check_cmake_format_exists() {
    if [[ ! -f $CMAKE_FORMAT_BIN ]]; then
      echo "cmake-format does not exists"
      echo "please install cmake-format using command 'sudo apt install cmake-format'"
      return 1
    else
      return 0
    fi
}
function cmake_format_all() {
   files=$(git diff --name-only --cached --diff-filter=ACMR)
   for file in $files; do
       if [[ $file = *CMakeLists.txt ]] || [[ $file = $.cmake ]]; then
         $CMAKE_FORMAT_BIN -i $file
         git add $file
       fi
   done
}
numErrors=0
errorList=()
function check() {
  if ! check_cmake_format_exists; then
      return 1
  fi
  files=$(git diff --name-only --cached --diff-filter=ACMR)
  for file in $files; do
      if [[ $file = *CMakeLists.txt ]] || [[ $file = *.cmake ]]; then
        $CMAKE_FORMAT_BIN --check "$file" 2>/dev/null
        if [[ $? != 0 ]]; then
          ((numErrors=numErrors+1))
          errorList+=($file)
        fi
      fi
  done
  if [[ $numErrors != 0 ]]; then
    printf "\nERROR: $numErrors cmake files not formated. you can use the following commands:\n"
    printf "        cmake-format -i thefile # format a specific file\n"
    printf "        $0 format # format all git cached cmake files\n"
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
elif [[ $# == 1 ]] && [[ $1 == "format" ]]; then
  cmake_format_all
else
  echo "error invalid command"
  exit 1
fi