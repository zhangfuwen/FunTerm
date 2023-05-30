#!/usr/bin/env bash
# Based on okular/hooks/pre-commit, credits go to Albert Astals Cid
readonly output=$(git clang-format -v --diff)
if [[ ! -f .clang-format ]]; then
    echo "ERROR: no .clang-format file found in repository root, abort format"
    echo "       run cmake for this repository to generate it"
    exit 1
fi
if [[ "$output" == *"no modified files to format"* ]]; then exit 0; fi
if [[ "$output" == *"clang-format did not modify any files"* ]]; then exit 0; fi
echo "ERROR: C++ files not formatted, please format your files. You can do this using the following commands:"
echo "       git clang-format --force # format the changed parts"
echo "       git clang-format --diff # preview the changes done by the formatter"
exit 1