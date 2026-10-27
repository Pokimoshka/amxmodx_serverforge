# Примеры миграции небезопасного кода

## 1. Небезопасное копирование строк

### До (НЕБЕЗОПАСНО):
```cpp
char buffer[256];
strcpy(buffer, user_input);  // Buffer overflow if user_input > 255 chars!
```

### После (БЕЗОПАСНО):
```cpp
#include "string_utils.h"

char buffer[256];
SAFE_STRCOPY(buffer, user_input);  // Automatically limits to buffer size
```

Или:
```cpp
amx::safe_strcopy(buffer, sizeof(buffer), user_input);
```

## 2. Небезопасная конкатенация

### До (НЕБЕЗОПАСНО):
```cpp
char path[256] = "/home/user/";
strcat(path, filename);  // Overflow if filename too long!
```

### После (БЕЗОПАСНО):
```cpp
char path[256] = "/home/user/";
SAFE_STRCAT(path, filename);
```

## 3. Небезопасное форматирование

### До (НЕБЕЗОПАСНО):
```cpp
char msg[128];
sprintf(msg, "Player %s has %d points", name, points);  // Overflow!
```

### После (БЕЗОПАСНО):
```cpp
char msg[128];
SAFE_SPRINTF(msg, "Player %s has %d points", name, points);
```

## 4. Использование strncpy (ОПАСНО!)

### До (ОПАСНО - может не добавить null terminator):
```cpp
char dest[10];
strncpy(dest, src, sizeof(dest));
// dest может быть не null-terminated!
```

### После (БЕЗОПАСНО):
```cpp
char dest[10];
SAFE_STRCOPY(dest, src);
// Всегда null-terminated
```

## 5. Format string vulnerability

### До (КРИТИЧЕСКАЯ УЯЗВИМОСТЬ):
```cpp
char error_msg[256];
sprintf(error_msg, user_input);  // User can inject %s, %n, etc!
```

### После (БЕЗОПАСНО):
```cpp
char error_msg[256];
SAFE_SPRINTF(error_msg, "%s", user_input);  // user_input treated as data
```

## 6. Динамические строки

### До (утечки памяти, ошибки):
```cpp
char* str = (char*)malloc(100);
strcpy(str, "hello");
// ... забыли free(str)
```

### После (RAII):
```cpp
#include "string_utils.h"

amx::String str;
str.assign("hello");
str.append(" world");
// Автоматически освобождается
```

## 7. Проверка null-termination

### До (непроверено):
```cpp
void process_buffer(char* buf, size_t size) {
    printf("%s\n", buf);  // Может выйти за границы!
}
```

### После (с проверкой):
```cpp
#include "string_utils.h"

void process_buffer(char* buf, size_t size) {
    if (amx::is_null_terminated(buf, size)) {
        printf("%s\n", buf);
    } else {
        // Ошибка: buffer не null-terminated
        buf[size - 1] = '\0';  // Принудительно завершаем
        printf("%s\n", buf);
    }
}
```

## 8. Безопасное измерение длины

### До (может читать за границами):
```cpp
size_t len = strlen(untrusted_buffer);  // Может уйти в бесконечность!
```

### После (ограничено):
```cpp
#include "string_utils.h"

size_t len = amx::safe_strlen(untrusted_buffer, MAX_BUFFER_SIZE);
```

## 9. Работа с путями

### До (множественные overflow):
```cpp
char fullpath[MAX_PATH];
strcpy(fullpath, basedir);
strcat(fullpath, "/");
strcat(fullpath, filename);
strcat(fullpath, ".txt");
```

### После (безопасно):
```cpp
char fullpath[MAX_PATH];
SAFE_STRCOPY(fullpath, basedir);
SAFE_STRCAT(fullpath, "/");
SAFE_STRCAT(fullpath, filename);
SAFE_STRCAT(fullpath, ".txt");
```

Или ещё лучше:
```cpp
amx::String fullpath(MAX_PATH);
fullpath.assign(basedir);
fullpath.append("/");
fullpath.append(filename);
fullpath.append(".txt");
```

## 10. Работа с пользовательским вводом

### До (КРИТИЧНО):
```cpp
void handle_command(const char* user_cmd) {
    char buffer[256];
    sprintf(buffer, "Executing: %s", user_cmd);  // Overflow!
    system(buffer);  // Command injection!
}
```

### После (безопаснее, но всё ещё нужна валидация):
```cpp
void handle_command(const char* user_cmd) {
    // 1. Валидация ввода
    if (!is_valid_command(user_cmd)) {
        return;
    }
    
    // 2. Безопасное форматирование
    char buffer[256];
    SAFE_SPRINTF(buffer, "Executing: %s", user_cmd);
    
    // 3. Безопасное выполнение (лучше использовать execv вместо system)
    log_message(buffer);
}
```

## Checklist для code review

При review кода ищите:

- [ ] `strcpy()` → заменить на `SAFE_STRCOPY()`
- [ ] `strcat()` → заменить на `SAFE_STRCAT()`
- [ ] `sprintf()` → заменить на `SAFE_SPRINTF()`
- [ ] `strncpy()` → заменить на `SAFE_STRCOPY()` (strncpy опасен!)
- [ ] `strncat()` → заменить на `SAFE_STRCAT()`
- [ ] `printf(user_string)` → заменить на `printf("%s", user_string)`
- [ ] `strlen()` на ненадёжных данных → `safe_strlen()`
- [ ] Ручное `malloc/free` для строк → `amx::String`
- [ ] Проверить все буферы на null-termination

## Performance Note

Все safe_* функции inline и компилируются в эффективный код. Overhead минимальный:
- `SAFE_STRCOPY` → примерно тоже самое что strncpy + добавление '\0'
- `SAFE_SPRINTF` → вызывает vsnprintf (стандартная библиотека)
- Проверки assert() компилируются только в DEBUG режиме

В release builds с оптимизацией производительность будет практически идентична unsafe версиям.
