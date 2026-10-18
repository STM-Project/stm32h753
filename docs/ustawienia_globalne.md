# Jedna struktura wszystkich ustawień

Deklaracje są w `Application/HTTP/http_all_settings.h`. Globalny obiekt
`HTTP_AllSettingsData` zawiera podstruktury wszystkich pól formularzy i ustawień
czujników. Funkcja `HTTP_SettingsGetAll()` zwraca stały adres tego obiektu:

```c
#include "http_all_settings.h"

const HTTP_AllSettings *settings = HTTP_SettingsGetAll();

uint8_t godzina = settings->timers[0].on[0].hour;
uint8_t minuta = settings->timers[0].on[0].minute;
uint8_t poniedzialek = settings->timers[0].weekdays[0];
uint8_t aktywny = settings->timers[0].enabled;
const char *nazwaPK = settings->outputs[0].name;
int64_t prog = settings->thermostats[0].threshold_x100; /* 2350 = 23,50 */
uint32_t zwloka = settings->thermostats[0].delay_seconds;
uint8_t trybPortu = settings->ports[0].mode;
const HttpSensorSettings *czujnik = &settings->sensors[0][0];
```

Indeksy zaczynają się od zera. `timers[0]` oznacza `/tim001`, a nie port GPIO 1.
Timery, termostaty i PK mają wspólną kolejność wyjść fizycznych:
28, 29, 30, 31, 32, 33, 24, 25, 26, 27, 21, 22, 23.
`ports[0]` oznacza port 1. `sensors[port-1][0]` to temperatura, `[1]` — wilgotność.

| Pole główne | Zawartość |
| --- | --- |
| `lan`, `wifi`, `clock`, `gsm`, `lora` | Sieć, WWW, czas, GSM, LoRa |
| `inputs[18]`, `eol[14]`, `outputs[13]` | Wejścia, EOL, przekaźniki/wyjścia |
| `timers[13]`, `thermostats[13]`, `counters[4]` | Timery, termostaty, liczniki |
| `sms`, `email`, `recorder`, `rotation` | Powiadomienia, rejestrator, rotacja |
| `adc[2]`, `logic[13]` | Tabele przeliczeń ADC, banki logiki |
| `measurementGraphs[40]`, `counterGraphs[16]` | Ustawienia wykresów |
| `ports[33]`, `sensors[20][2]` | Nazwy/tryby portów i ustawienia czujników |

Godziny mają pola `hour`, `minute`, `second`. Dni tygodnia to `weekdays[0..6]`
od poniedziałku do niedzieli. Flagi mają wartości 0/1. Pola `_x100` przechowują
liczby w setnych częściach jednostki, czasy `_seconds` są w sekundach. Czujniki
zachowują jednostki w dziesiątych częściach, opisane w `http_sensor_settings.h`.
Adres IPv4 `192.168.1.2` ma wartość `0xC0A80102`.

Podstruktury pozostałych stron zachowują rozpoznawalne nazwy pól Sandwitch.
Komentarz przy każdym polu w nagłówku wskazuje jego pełną nazwę formularza.
Nazwy i opisy wspólnego wyjścia są aktualizowane równocześnie w `outputs`,
`timers[].output` i `thermostats[].output`.

Struktura odzwierciedla zatwierdzone ustawienia, a pola jeszcze niezapisane mają
te same wartości domyślne co formularze. Aktualizuje się po udanym zapisie lub
wczytaniu danych; błąd walidacji/zapisu nie zmienia jej zawartości. Pole `revision`
zmienia się po odświeżeniu danych. Adres struktury pozostaje stały.

Traktuj ją jako dane do odczytu. Bezpośrednia zmiana pola nie wykonuje walidacji
ani zapisu na SD i zostanie zastąpiona przy kolejnym zatwierdzeniu formularza.
Dostęp jest przeznaczony dla zadania HTTP; odczyt kilku pól z innego zadania
wymaga synchronizacji z zadaniem aktualizującym strukturę. Obiekt znajduje się
w SDRAM, więc używaj go po inicjalizacji tej pamięci. Struktura zawiera także
zapisane hasła — nie wypisuj jej całej do logów.

Formaty `h7conf.bin` oraz plików czujników pozostają zgodne z poprzednią wersją.
Nowa struktura nie uruchamia samodzielnie sprzętowej logiki timerów/termostatów.

Nagłówek i powiązania pól generuje `python3 tools/generate_settings_struct.py`.
Po zmianie katalogu odtwórz oba pliki tym poleceniem. Kontrola zgodności:
`python3 tools/generate_settings_struct.py --check`. Test całej integracji:
`python3 tests/test_http_all_settings.py`.
