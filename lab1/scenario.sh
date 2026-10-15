# Сценарий работы bash, под которым снимается профиль.
# Покрывает типичный функционал оболочки: переменные, встроенные команды,
# запуск внешних программ, конвейер, перенаправление в файл, чтение файла, подоболочку.
cd /tmp
NAME="oslab"
echo "hello from $NAME" > out.txt          # встроенная echo + перенаправление (open/dup2)
echo "second line" >> out.txt              # дозапись (O_APPEND)
cat out.txt | wc -l                        # конвейер: pipe + 2x fork/exec
while read -r line; do :; done < out.txt   # чтение файла встроенной read
ls -l out.txt > /dev/null                  # поиск в PATH + exec внешней программы
( sleep 0.1 )                              # подоболочка + ожидание
rm out.txt                                 # удаление файла (unlinkat в rm)
