# Generated with cmake-format 0.5.4
# --------------------------
# General Formatting Options
# --------------------------
# How wide to allow formatted cmake files
line_width = 120
# How many spaces to tab for indent
tab_size = 4
# If arglists are longer than this, break them always
max_subargs_per_line = 10
# If true, separate flow control names from their parentheses with a space
separate_ctrl_name_with_space = False
# If true, separate function names from parentheses with a space
separate_fn_name_with_space = False
# If a statement is wrapped to more than one line, than dangle the closing
# parenthesis on it's own line
dangle_parens = True
# If the statement spelling length (including space and parenthesis is larger
# than the tab width by more than this amoung, then force reject un-nested
# layouts.
max_prefix_chars = 2
# If a candidate layout is wrapped horizontally but it exceeds this many lines,
# then reject the layout.
max_lines_hwrap = 1
# What style line endings to use in the output.
line_ending = 'unix'
# Format command names consistently as 'lower' or 'upper' case
command_case = 'lower'
# Format keywords consistently as 'lower' or 'upper' case
keyword_case = 'upper'
# Specify structure for custom cmake functions
additional_commands = {
    "ecm_setup_version": {
        "flags": [
            "VARIABLE_PREFIX",
            "VERION_HEADER",
            "PACKAGE_VERSION_FILE",
            "SOVERSION"
        ],
        "kwargs": {
            "VERSION_HEADER": "*",
            "VARIABLE_PREFIX": "*",
            "PACKAGE_VERSION_FILE": "*",
            "SOVERSION": "*"
        }
    },
    "ecm_qt_declare_logging_category": {
        "flags": [
            "VARIABLE_PREFIX",
        ],
        "kwargs": {
            "HEADER": "*",
            "IDENTIFIER": "*",
            "CATEGORY_NAME": "*",
            "DEFAULT_SEVERITY": "*",
            "DESCRIPTION": "*",
            "EXPORT": "*"
        }
    }
}
# A list of command names which should always be wrapped
always_wrap = []
# Specify the order of wrapping algorithms during successive reflow attempts
algorithm_order = [0, 1, 2, 3, 4]
# If true, the argument lists which are known to be sortable will be sorted
# lexicographicall
enable_sort = False
# If true, the parsers may infer whether or not an argument list is sortable
# (without annotation).
autosort = False
# If a comment line starts with at least this many consecutive hash characters,
# then don't lstrip() them off. This allows for lazy hash rulers where the first
# hash char is not separated by space
hashruler_min_length = 10
# A dictionary containing any per-command configuration overrides. Currently
# only `command_case` is supported.
per_command = {}
# --------------------------
# Comment Formatting Options
# --------------------------
# What character to use for bulleted lists
bullet_char = '*'
# What character to use as punctuation after numerals in an enumerated list
enum_char = '.'
# enable comment markup parsing and reflow
enable_markup = True
# If comment markup is enabled, don't reflow the first comment block in each
# listfile. Use this to preserve formatting of your copyright/license
# statements.
first_comment_is_literal = True
# If comment markup is enabled, don't reflow any comment block which matches
# this (regex) pattern. Default is `None` (disabled).
literal_comment_pattern = None
# Regular expression to match preformat fences in comments
# default=r'^\s*([`~]{3}[`~]*)(.*)$'
fence_pattern = '^\\s*([`~]{3}[`~]*)(.*)$'
# Regular expression to match rulers in comments
# default=r'^\s*[^\w\s]{3}.*[^\w\s]{3}$'
ruler_pattern = '^\\s*[^\\w\\s]{3}.*[^\\w\\s]{3}$'
# If true, then insert a space between the first hash char and remaining hash
# chars in a hash ruler, and normalize it's length to fill the column
canonicalize_hashrulers = True
# ---------------------------------
# Miscellaneous Options
# ---------------------------------
# If true, emit the unicode byte-order mark (BOM) at the start of the file
emit_byteorder_mark = False
# Specify the encoding of the input file. Defaults to utf-8.
input_encoding = 'utf-8'
# Specify the encoding of the output file. Defaults to utf-8. Note that cmake
# only claims to support utf-8 so be careful when using anything else
output_encoding = 'utf-8'