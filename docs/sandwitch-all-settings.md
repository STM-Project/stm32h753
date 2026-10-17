# Wszystkie podstrony ustawień Sandwitch na STM32H753

Otwórz `/set`. W kolumnie „Ustaw / Odczyty” numery portów prowadzą do
odpowiednich formularzy. Menu zawiera ustawienia globalne i podstrony graficzne.
Po wprowadzeniu danych wybierz „Zapisz”. Potwierdzenie oznacza zakończony zapis
na SD; błąd formularza lub karty zachowuje poprzednią konfigurację.

## Formularze

| Adres | Ustawienia |
|---|---|
| `/set`, aliasy `/Set`, `/3main` | Nazwy i tryby 33 portów; osobny zapis nazw i trybów. |
| `/temp001`–`/temp020` (bez 15/16) | Wcześniej przeniesione progi temperatury/wilgotności. |
| `/we001`–`/we018` | Nazwy i stany wejść, zwłoka, reakcja, przekaźnik i przytrzymanie. |
| `/eol001`–`/eol014` | Wejścia EOL, opisy stanów/uszkodzenia i czasy. |
| `/wypk001`–`/wypk013` | Nazwy wyjść i opis stanów ON/OFF. |
| `/tim001`–`/tim013` | Cztery pary godzin ON/OFF, dni tygodnia, aktywacja i opis wyjścia. |
| `/term001`–`/term013` | Czujnik temperatury/wilgotności, próg, histereza i zwłoka. |
| `/nap001`, `/nap002` | Pełne tabele przeliczeń ADC: 100 wierszy 0,0–9,9 V i jednostka. |
| `/etr001`–`/etr004` | Skalowanie impulsów, jednostka i progi godzinne/dobowe/tygodniowe/miesięczne. |
| `/lan` | Konfiguracja LAN oraz parametry panelu WWW. |
| `/wifi` | Sieć WiFi oraz profile STA i AP. |
| `/czas` | Data i czas oraz odnośnik LCD. |
| `/gsm` | Numer SIM, parametry GPRS i karta komend GSM z oryginału. |
| `/maskisms` | Osiem kontaktów, dzwonienie/raporty, macierz powiadomień i godzina raportu. |
| `/maskimail` | Osiem odbiorców, raporty, macierz powiadomień i konfiguracja SMTP. |
| `/nrf`, alias `/lora` | Częstotliwość, moc, spread factor, pasmo i długość pakietu LoRa. |
| `/rej` | Konfiguracja rejestrowania pomiarów i zdarzeń. |
| `/Rot` | Osiem cykli rotacji, macierz wyjść, harmonogram oraz progi i czasy. |
| `/logic`, `/logic001`–`/logic013` | Edytor siedmiu bramek, źródła, operatory, BUFFER/NOT; osobny bank dla każdego wyjścia. |
| `/wykres1`, `/wykres2` | Konfiguracja skali i rozmiaru wykresów pomiarów oraz liczników. |

Numery w adresach zachowują mapowanie Sandwitch, a nie zawsze numer portu fizycznego:

- `/we001`–`014` oznaczają porty 1–14, a `/we015`–`018` porty 17–20.
- Kolejne wyjścia 001–013 oznaczają porty `[28,29,30,31,32,33,24,25,26,27,21,22,23]`.
- Tabele ADC 001/002 należą do portów 15/16; liczniki 001–004 do portów 17–20.

Pola mają oryginalne nazwy CGI. Zachowano układ kart i klas CSS. Proceduralnie
odtworzono duże macierze, tabele i graficzny edytor logiki. W macierzy powiadomień
widoczne są wszystkie 20 wejść STM32H7. Zwiększono limity długości SSID/hasła
WiFi i nazwy serwera SMTP do pojemności istniejących struktur STM32; szerokości
pól pozostały takie jak w źródle. Puste pole hasła zachowuje dotychczasowe hasło.

## Co jest połączone z działającym kodem STM32

Zapisane profile WiFi i SMTP są kopiowane do istniejącej struktury `Const`.
Przy pierwszym otwarciu formularze tych usług pokazują bieżące wartości,
z pominięciem haseł. Ustawienia sieci są używane przy następnym połączeniu
lub restarcie, aby nie zerwać odpowiedzi HTTP w trakcie zapisywania.
SMTP korzysta z profilu nadawcy 0 i ośmiu istniejących odbiorców.
Manualny zapis daty/czasu aktualizuje zegar programowy. Przy restarcie nie
odtwarza starej zapisanej daty; synchronizacja SNTP pozostaje dostępna.
Obecny typ `Const.sntp.time` jest 32-bitowy, więc sterownik czasu zachowuje
jego dotychczasowy zakres reprezentowalnych dat.

Pozostałe formularze są dostępne i mają trwały zapis, a dane można pobierać
przez `HTTP_SettingsGet*()`. W tym projekcie nie ma jeszcze silników Sandwitch
sterujących GSM, LoRa, Ethernetem, rotacją, bramkami logicznymi, alarmami,
timerami wyjść ani rejestrowaniem fizycznych pomiarów. Wprowadzenie ich
parametrów nie uruchamia takich funkcji sprzętowych. Parametry loginu/hasła WWW
są przechowywane; wdrożenie uwierzytelniania serwera HTTP stanowi osobną funkcję.
Wykresy udostępniają konfigurację; dane wymagają podłączenia rejestratora.

Akcje wymagające nieobecnych modułów (m.in. programowanie IR, LCD, skan WiFi,
kasowanie rejestru, aktualizacja firmware i podgląd sesji) zwracają czytelną stronę z kodem HTTP 501.
Nie sygnalizują fikcyjnego wykonania operacji.

## Pliki i zapis

- `http_settings_catalog.c/.h` — katalog tras, szablonów, pól i walidacji.
- `http_settings_page.c/.h`, `http_settings_logic.inc` — generowanie HTML i edytora logiki.
- `http_settings_model.c/.h` — walidacja, transakcje i odczyt parametrów.
- `http_settings_storage.c/.h` — zapis FatFs i odzyskiwanie kopii.
- `http_settings_bindings.c/.h` — połączenie WiFi, SMTP i zegara z istniejącym firmware.
- `esp32wroom.c`, `wwwPages.c` — routing, menu, nazwy/tryby portów i odpowiedzi HTTP.
- `tools/import_sandwitch_settings.py` oraz `tools/sandwitch_*.json` — odtwarzalny import szablonów i schematów z dostarczonego archiwum.

Ustawienia są zapisane w `0:/h7conf.bin`, z plikiem tymczasowym `.tmp` i kopią
`.bak`. Wcześniejsze progi czujników zachowują osobne pliki `h7sense.*`.
Format ma wersję, podpis schematu i CRC. Zapis jest zatwierdzany w RAM dopiero
po poprawnym zapisie i sprawdzeniu pliku na SD. Pliki nie zawierają danych z
archiwum użytkownika innych niż szablony i nowe ustawienia wprowadzone w formularzach.
Wartości poufne wprowadzone przez użytkownika są zapisane w konfiguracji SD;
nie są umieszczane w HTML ani wypisywane w dziennikach nowych modułów.

Duże formularze są odbierane w wielu ramkach ESP-AT, do 65535 bajtów adresu GET.
Bufory stron, parsera i konfiguracji znajdują się w SDRAM; należy ją zainicjować
przed zadaniem WiFi. Projekt CubeIDE ma już katalog `Application` w źródłach;
po pobraniu zmian odśwież projekt i wykonaj czyste budowanie.

## Testy

Z katalogu repozytorium uruchom testy `tests/test_http_settings_*.py` oraz
wcześniejsze testy `test_settings_templates.py`, `test_http_sensor_*.py`
i `test_http_request_stream.py`. Używają one rzeczywistych modułów C,
ASan/UBSan i kontrolowanych atrap sprzętu. Generator sprawdza się poleceniem:

```sh
python3 tools/import_sandwitch_settings.py --check
```

Testy na hoście i kompilacja nie zastępują sprawdzenia strony, karty SD oraz
połączenia ESP32 na fizycznej płytce.
