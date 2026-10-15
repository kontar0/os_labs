#!/bin/sh
# Тесты ЛР2: подаём имя файла в stdin родителя, сравниваем stdout и код возврата.
cd "$(dirname "$0")/.." || exit 1
failed=0

check() { # название, что подать в stdin, ожидаемый stdout, ожидаемый код
    output=$(printf '%s\n' "$2" | ./parent 2>/dev/null)
    code=$?
    expected=$(printf '%b' "$3")
    if [ "$output" = "$expected" ] && [ "$code" -eq "$4" ]; then
        echo "OK    $1"
    else
        echo "FAIL  $1: получили код $code, вывод [$output]; ждали код $4, вывод [$expected]"
        failed=1
    fi
}

check "обычный файл" tests/data/basic.txt '6\n100\n0\n42' 0
check "пустые строки, мусор, выход за int" tests/data/edge.txt '6\n24\n-2147483648\n100' 1
check "несуществующий файл" tests/data/no_such_file.txt '' 1
check "пустое имя файла" '' '' 1

# Обрыв связи: head читает одну строку и закрывает трубу, остальные данные писать некуда.
big_file=/tmp/lab2_big.txt
yes '1 1' | head -n 300000 > "$big_file"
echo "$big_file" | ./parent 2> /tmp/lab2_err.txt | head -n 1 > /dev/null
if grep -q 'Broken pipe' /tmp/lab2_err.txt && grep -q 'завершился с кодом 1' /tmp/lab2_err.txt; then
    echo "OK    обрыв трубы: оба получили EPIPE, ребёнок дождан"
else
    echo "FAIL  обрыв трубы"
    cat /tmp/lab2_err.txt
    failed=1
fi
rm -f "$big_file" /tmp/lab2_err.txt

[ $failed -eq 0 ] && echo "ВСЕ ТЕСТЫ ПРОЙДЕНЫ"
exit $failed
