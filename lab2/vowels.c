#include <string.h>

#include "vowels.h"

/* Русские буквы в UTF-8 занимают по 2 байта, поэтому храним их строками. */
static const char *russian_vowels[] = {
    "а", "е", "ё", "и", "о", "у", "ы", "э", "ю", "я",
    "А", "Е", "Ё", "И", "О", "У", "Ы", "Э", "Ю", "Я",
};

static int is_russian_vowel(const char *position)
{
    int count = sizeof(russian_vowels) / sizeof(russian_vowels[0]);
    for (int i = 0; i < count; i++)
        if (strncmp(position, russian_vowels[i], 2) == 0)
            return 1;
    return 0;
}

void remove_vowels(char *line)
{
    char *from = line; /* откуда читаем */
    char *to = line;   /* куда пишем: строка сжимается на месте */

    while (*from != '\0') {
        if (strchr("aeiouAEIOU", *from) != NULL) {
            from += 1; /* латинская гласная: 1 байт, пропускаем */
        } else if (is_russian_vowel(from)) {
            from += 2; /* русская гласная: 2 байта, пропускаем */
        } else {
            *to = *from; /* остальное копируем как есть */
            to++;
            from++;
        }
    }
    *to = '\0';
}
