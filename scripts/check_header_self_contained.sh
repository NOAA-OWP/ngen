#!/usr/bin/env bash
# Check that each header under include/ compiles when included alone.
# Usage: scripts/check_header_self_contained.sh <build-dir> [header ...]
# <build-dir> must be configured with -DCMAKE_EXPORT_COMPILE_COMMANDS=ON;
# include paths and definitions are taken from the union of its compile commands.
set -u
build=${1:?usage: $0 <build-dir> [header ...]}; shift
src=$(cd "$(dirname "$0")/.." && pwd)
cxx=${CXX:-c++}
jobs=${JOBS:-$(nproc)}

flags=$(python3 - "$build/compile_commands.json" <<'PY'
import json, shlex, sys
from collections import Counter
seen = []
stds = Counter()
for e in json.load(open(sys.argv[1])):
    if not e["file"].endswith((".cpp", ".cxx", ".cc")):
        continue
    a = shlex.split(e["command"]) if "command" in e else e["arguments"]
    i = 0
    while i < len(a):
        t = a[i]
        if t in ("-I", "-isystem", "-D"):
            t = t + a[i + 1]; i += 1
        if t.startswith("-std="):
            stds[t] += 1
        elif t.startswith(("-I", "-D")):
            t = t.replace("-isystem", "-I", 1)
            if t not in seen: seen.append(t)
        elif t.startswith("-isystem"):
            t = "-I" + t[len("-isystem"):]
            if t not in seen: seen.append(t)
        i += 1
seen.insert(0, stds.most_common(1)[0][0])
print(" ".join(shlex.quote(f) for f in seen))
PY
)
# Every directory under include/ too, since sources rely on many of them.
for d in $(find "$src/include" -type d); do flags+=" -I$d"; done

if [ $# -eq 0 ]; then
    mapfile -t headers < <(cd "$src/include" && find . \( -name '*.h' -o -name '*.hpp' \) | sed 's|^\./||' | sort)
else
    headers=("$@")
fi

out=$(mktemp -d); trap 'rm -rf "$out"' EXIT
export cxx flags out
check() {
    h=$1; log="$out/$(echo "$h" | tr / _).log"
    if echo "#include \"$h\"" | eval "$cxx $flags -fsyntax-only -x c++ -" >"$log" 2>&1; then
        echo "PASS $h"
    else
        echo "FAIL $h"; sed 's/^/    /' "$log" | grep -m5 'error' 
    fi
}
export -f check
printf '%s\n' "${headers[@]}" | xargs -P "$jobs" -I{} bash -c 'check "$@"' _ {} | tee "$out/results"
fails=$(grep -c '^FAIL' "$out/results")
echo "$(grep -c '^PASS' "$out/results") passed, $fails failed"
[ "$fails" -eq 0 ]
