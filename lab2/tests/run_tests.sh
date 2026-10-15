#!/bin/sh
# Автотесты ЛР2: сравнивают stdout родителя и код возврата с ожидаемыми.
cd "$(dirname "$0")/.." || exit 1
BIN=./build/parent
fail=0

check() { # name input expected_stdout expected_rc
    out=$(printf '%s\n' "$2" | $BIN 2>/dev/null); rc=$?
    exp=$(printf '%b' "$3")
    if [ "$out" = "$exp" ] && [ "$rc" -eq "$4" ]; then
        echo "PASS  $1"
    else
        echo "FAIL  $1: got rc=$rc out=[$out], want rc=$4 out=[$exp]"; fail=1
    fi
}

check "basic sums"            tests/data/basic.txt               '6\n100\n0\n42' 0
check "edge cases"            tests/data/edge.txt                '6\n24\n-2147483648\n100' 1
check "empty file"            tests/data/empty.txt               ''              0
check "no trailing newline"   tests/data/no_trailing_newline.txt '3\n7'          0
check "file name with spaces" "   tests/data/basic.txt  "        '6\n100\n0\n42' 0
check "missing file"          tests/data/nope.txt                ''              1
check "empty file name"       ''                                 ''              1

# длинная строка: 200000 единиц -> 200000
seq 200000 | sed 's/.*/1/' | tr '\n' ' ' > /tmp/lab2_long.txt; echo >> /tmp/lab2_long.txt
check "200k numbers in a line" /tmp/lab2_long.txt '200000' 0
rm -f /tmp/lab2_long.txt

# обрыв канала: читатель stdout родителя закрывается после первой строки
yes '1 1' | head -n 300000 > /tmp/lab2_big.txt
echo /tmp/lab2_big.txt | $BIN 2>/tmp/lab2_err.txt | head -n 1 >/dev/null
if grep -q 'exited with code 2' /tmp/lab2_err.txt; then
    echo "PASS  broken pipe handled (child exits with code 2, no crash)"
else
    echo "FAIL  broken pipe"; cat /tmp/lab2_err.txt; fail=1
fi
rm -f /tmp/lab2_big.txt /tmp/lab2_err.txt

[ $fail -eq 0 ] && echo "ALL TESTS PASSED"
exit $fail
