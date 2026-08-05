# Scan for undocumented functions in C files
import re
import glob
import argparse

from pathlib import Path

# List of C keywords to exclude
C_KEYWORDS = {
    "if",
    "else",
    "while",
    "for",
    "do",
    "switch",
    "case",
    "default",
    "break",
    "continue",
    "return",
    "goto",
    "sizeof",
    "typedef",
    "struct",
    "union",
    "enum",
    "static",
    "extern",
    "const",
    "volatile",
    "inline",
    "register",
    "restrict",
    "auto",
    "void",
    "int",
    "char",
    "float",
    "double",
    "long",
    "short",
    "signed",
    "unsigned",
    "bool",
    "_Bool",
    "_Complex",
    "_Imaginary",
    "alignas",
    "alignof",
    "atomic",
    "thread_local",
    "noexcept",
    "static_assert",
    "constexpr",
}

SKIP_FILES = [
    Path("src/game/kerent.c"),
    Path("src/game/font.c"),
    Path("src/game/ovllist.c"),
]

# Match complete single-line C function signatures, including the callback
# parameters and weak declarations used by the decompilation sources. Keeping
# the return type separate prevents a nested callback signature from being
# mistaken for the enclosing function name.
FUNC_DEF = re.compile(
    r"^[ \t]*"
    r"(?:(?:static|inline)\s+|__declspec\([^)]*\)\s+)*"
    r"(?:[a-zA-Z_]\w*(?:\s+|\s*\*+\s*))+"
    r"([a-zA-Z_]\w*)\s*"
    r"\((?:[^()]|\([^()]*\))*\)\s*\{",
    re.MULTILINE,
)


def find_documented_functions(content, function_matches):
    """Return functions with an immediately preceding Doxygen comment."""
    documented = set()
    for match in function_matches:
        prefix = content[:match.start()]
        comment_end = prefix.rfind("*/")
        if comment_end == -1 or prefix[comment_end + 2:].strip():
            continue
        comment_start = prefix.rfind("/*", 0, comment_end)
        if comment_start != -1 and prefix.startswith("/**", comment_start):
            documented.add(match.group(1))
    return documented

# Decompiler-style identifiers are audited token-wise rather than through a
# declaration-shape regex. This catches pointer declarations, parameters, and
# prototypes while excluding ordinary names such as `speed` that merely begin
# with the letters "sp".
UNLABELLED_IDENTIFIER = re.compile(
    r"(?<!\.)(?<!->)\b(?:arg[0-9A-Fa-f]+|var_[A-Za-z0-9_]+|temp_[A-Za-z0-9_]+|"
    r"sp[0-9][A-Fa-f0-9]*|unk[A-Za-z0-9_]*|lbl_\d+_[A-Za-z0-9_]+)\b"
)


def find_unlabelled_identifiers(content):
    """Find decompiler-style identifiers outside comments and literals."""
    code = re.sub(r"/\*.*?\*/|//[^\n]*", " ", content, flags=re.DOTALL)
    code = re.sub(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'', " ", code)
    return sorted(set(UNLABELLED_IDENTIFIER.findall(code)))

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Scan for undocumented functions in C files")
    parser.add_argument(
        "directory",
        nargs="?",
        default="src/game",
        help="Directory or file to scan for C files (default: src/game)"
    )
    parser.add_argument(
        "-l", "--list",
        action="store_true",
        help="List all undocumented functions and unlabelled variables"
    )
    parser.add_argument(
        "--list-functions",
        action="store_true",
        help="List only undocumented functions"
    )
    parser.add_argument(
        "--list-variables",
        action="store_true",
        help="List only unlabelled variables"
    )
    args = parser.parse_args()
    
    # Check if the path is a file or directory
    path = Path(args.directory)
    if path.is_file():
        files = [str(path)]
    elif path.is_dir():
        files = glob.glob(f"{args.directory}/**/*.c", recursive=True)
    else:
        print(f"Error: {args.directory} is not a valid file or directory")
        exit(1)

    total_funcs = 0
    total_documented = 0
    total_unlabelled = 0
    
    # Determine what to list
    list_funcs = args.list or args.list_functions
    list_vars = args.list or args.list_variables

    if not (list_funcs or list_vars):
        print("File".ljust(31), "Doc %", "Total".ljust(10), "Unlabelled Vars")
        print("-" * 64)

    for file in files:
        file_path = Path(file)
        if file_path in SKIP_FILES:
            continue
        with open(file_path, "r", encoding="utf-8") as f:
            content = f.read()
            # Find all function definitions
            function_matches = list(FUNC_DEF.finditer(content))
            all_funcs = set(match.group(1) for match in function_matches)
            # Check documentation against the same parsed definitions used for
            # the denominator so callback and weak signatures cannot skew it.
            documented_funcs = find_documented_functions(content, function_matches)
            # Filter out C keywords
            all_funcs = {func for func in all_funcs if func not in C_KEYWORDS}
            # Find all unlabelled decompiler-style identifiers, including
            # parameters and pointer-form declarations.
            all_vars = find_unlabelled_identifiers(content)
            # Undocumented = all - documented
            undocumented = all_funcs - documented_funcs
            percent = (len(documented_funcs) / len(all_funcs)) * 100 if all_funcs else 0
            percent_str = f"{percent:05.2f}" if percent < 10 else f"{percent:05.2f}"
            total_count = f"({len(documented_funcs)}/{len(all_funcs)})".ljust(7)
            
            if not (list_funcs or list_vars):
                print(f"{file:<30}: {percent_str}% {total_count} : {len(all_vars)}")
            else:
                # Print file header with stats
                print(f"\n{file}: {percent_str}% {total_count} : {len(all_vars)}")
                
                # List undocumented functions if requested
                if list_funcs and undocumented:
                    print(f"  Undocumented functions ({len(undocumented)}):")
                    for func in sorted(undocumented):
                        print(f"    - {func}")
                
                # List unlabelled variables if requested
                if list_vars and all_vars:
                    print(f"  Unlabelled variables ({len(all_vars)}):")
                    for var_name in all_vars:
                        print(f"    - {var_name}")
            
            total_funcs += len(all_funcs)
            total_documented += len(documented_funcs)
            total_unlabelled += len(all_vars)

    if total_funcs:
        total_percent = (total_documented / total_funcs) * 100
    else:
        total_percent = 0

    print(
        f"\nTotal: {total_documented}/{total_funcs} functions documented ({total_percent:.2f}%)"
    )
    print(f"Total unlabelled variables: {total_unlabelled}")
