#!/bin/sh
# Тесты ЛР2 (вариант 19). Первые две строки ввода — имена файлов, дальше строки текста.
cd "$(dirname "$0")/.." || exit 1
tmp=$(mktemp -d)
failed=0

ok()   { echo "OK    $1"; }
fail() { echo "FAIL  $1"; failed=1; }

# 1. Гласные удаляются (латинские и русские), ни одна строка не теряется.
{ echo "$tmp/out1.txt"; echo "$tmp/out2.txt"; yes 'Hello, Мир! AEIOU Ёжик yes' | head -n 200; } \
    | ./parent 2>/dev/null
code=$?
total=$(cat "$tmp/out1.txt" "$tmp/out2.txt" | wc -l)
unique=$(cat "$tmp/out1.txt" "$tmp/out2.txt" | sort -u)
if [ $code -eq 0 ] && [ "$total" -eq 200 ] && [ "$unique" = "Hll, Мр!  жк ys" ]; then
    ok "гласные удалены, все 200 строк дошли"
else
    fail "гласные: код $code, строк $total, содержимое [$unique]"
fi

# 2. Фильтр 80/20: из 1000 строк в pipe1 должно попасть примерно 800.
{ echo "$tmp/out1.txt"; echo "$tmp/out2.txt"; yes 'abc' | head -n 1000; } | ./parent 2>/dev/null
first=$(wc -l < "$tmp/out1.txt")
second=$(wc -l < "$tmp/out2.txt")
if [ "$first" -ge 700 ] && [ "$first" -le 900 ] && [ $((first + second)) -eq 1000 ]; then
    ok "фильтр 80/20: в child1 $first строк, в child2 $second"
else
    fail "фильтр: в child1 $first, в child2 $second"
fi

# 3. Нет строк текста: оба файла создаются пустыми, код 0.
printf '%s\n%s\n' "$tmp/e1.txt" "$tmp/e2.txt" | ./parent 2>/dev/null
code=$?
if [ $code -eq 0 ] && [ -f "$tmp/e1.txt" ] && [ ! -s "$tmp/e1.txt" ] && [ ! -s "$tmp/e2.txt" ]; then
    ok "пустой ввод: два пустых файла"
else
    fail "пустой ввод: код $code"
fi

# 4. Файл нельзя создать (нет такой папки) — ошибка.
printf '%s\n%s\n' /no/such/dir/a.txt "$tmp/b.txt" | ./parent 2>/dev/null
[ $? -eq 1 ] && ok "нельзя открыть файл: код 1" || fail "нельзя открыть файл"

# 5. Пустое имя файла — ошибка.
printf '\n%s\n' "$tmp/b.txt" | ./parent 2>/dev/null
[ $? -eq 1 ] && ok "пустое имя файла: код 1" || fail "пустое имя файла"

# 6. Введено только одно имя — ошибка.
printf '%s\n' "$tmp/a.txt" | ./parent 2>/dev/null
[ $? -eq 1 ] && ok "нет второго имени: код 1" || fail "нет второго имени"

rm -rf "$tmp"
[ $failed -eq 0 ] && echo "ВСЕ ТЕСТЫ ПРОЙДЕНЫ"
exit $failed
