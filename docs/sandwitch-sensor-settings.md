# Progi temperatury i wilgotności — formularz Sandwitch na STM32H753

1. Otwórz `http://<adres-STM32>/set` (`/Set` i `/3main` pozostają aliasami).
2. W kolumnie **Ustaw / Odczyty** kliknij numer przy temperaturze.
   Przykłady: `/temp001` — temperatura; `/temp005` — temperatura i wilgotność.
3. Ustaw nazwę, dwa progi maksymalne, dwa minimalne, histerezę,
   korektę oraz czas zwłoki `mm:ss`, a następnie kliknij **Zapisz**.
4. Potwierdzenie pojawia się dopiero po poprawnym zapisie na SD.
   Ponowne wejście na tę stronę i ponowne uruchomienie urządzenia odczytują zapisane wartości.

Układ kart, klasy CSS, podpisy, szerokości pól i nazwy CGI pochodzą z
`Sandwitch/Inc/http.h` (`HttpCzujki`, `Http_zapisz`) oraz obsługi `/tempNNN`
w `Sandwitch/Src/http.c`, z archiwum dostarczonego przez użytkownika.
Wspólne `HttpStyle` i `HttpMainMenu` pozostają w `wwwPages.c`.
Limit długości pola liczbowego zwiększono z 5 do 7 znaków, przy zachowaniu
szerokości 50px, aby obsłużyć 100% wilgotności i cały przechowywany zakres.

## Dane i zapis

- Model: `Application/HTTP/http_sensor_settings.c/.h`.
- Formularz: `Application/HTTP/http_sensor_page.c/.h`.
- Zapis/odczyt FatFs: `Application/HTTP/http_sensor_storage.c/.h`.
- Odbiór pofragmentowanych ramek ESP-AT: `Application/HTTP/http_request_stream.c/.h`.
- Routing i kolejka odpowiedzi: `Application/esp32wroom.c`.

Temperatura i wilgotność są przechowywane osobno, z dokładnością 0,1.
Identyfikator CGI temperatury portu `p` wynosi `2*p-1`, a wilgotności `2*p`.
Struktura rekordu zawiera nazwę, max1/max2/min1/min2, histerezę,
korektę i zwłokę w sekundach. Wszystkie pola przesłanego czujnika muszą
być poprawne; błędny formularz nie zmienia żadnego ustawienia.

Karta SD musi być zamontowana przed uruchomieniem zadania WiFi.
Plik konfiguracji to `0:/h7sense.bin`; poprzednia poprawna wersja jest
zachowywana w `0:/h7sense.bak`. Zapis używa pliku tymczasowego
`0:/h7sense.tmp`, synchronizacji i zmiany nazwy. Wersja formatu i CRC32
służą do wykrywania uszkodzonych danych. Nie jest to gwarancja odporności
samej karty/systemu FAT na odłączenie zasilania w trakcie zapisu.
Brak karty lub błąd zapisu wyświetla błąd i zachowuje poprzednie wartości w RAM.
Gdy oba pliki są uszkodzone, są zachowywane do odzyskania, a zapis jest blokowany.

`HTTP_SettingsPortMode()` zawiera istniejące przykładowe tryby portów strony
ustawień; porty 5, 6 i 20 są obecnie typu temperatura/wilgotność. Te tryby
pozostają zgodne z dotychczasowym przykładem. Nie dodano w tym zadaniu trwałego
zapisu wyboru trybów ani implementacji fizycznych czujników. Nowe API
`HTTP_SensorSettingsGet(port, humidity)` udostępnia progi kodowi pomiarów/alarmów;
sterowanie alarmami wymaga podłączenia tego API do odpowiedniej logiki urządzenia.

Bufory generowanych stron są osobne dla każdego slotu kolejki, aby kolejne
żądanie nie nadpisało właśnie wysyłanej odpowiedzi. Odpowiedzi mają nagłówki
HTTP/1.1, UTF-8, Content-Length i Connection: close.

## Weryfikacja bez płytki

Uruchom z katalogu repozytorium:

```sh
python3 tests/test_settings_templates.py
python3 tests/test_http_sensor_settings.py
python3 tests/test_http_sensor_page.py
python3 tests/test_http_sensor_storage.py
python3 tests/test_http_request_stream.py
python3 tests/test_http_sensor_routes.py
```

Testy używają kompilatora C na hoście, ASan/UBSan i atrap interfejsów sprzętu.
Testy stron sprawdzają również JavaScript przy dostępnym Node.js, a niektóre
moduły kompilują dla Cortex-M7, gdy lokalny toolchain jest dostępny.
Zapis/odczyt na fizycznej karcie i wyświetlanie strony przez rzeczywisty ESP32
należy dodatkowo sprawdzić na płytce.
