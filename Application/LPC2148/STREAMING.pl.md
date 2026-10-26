# Strony LPC z SD bez pełnych kopii w SDRAM

HTTP przez ESP32 odczytuje `/nowaN` z `0:/LPC/Pnnnnn.HTM` porcjami.
Panel `/nowa` z trzema termometrami jest odczytywany tak samo z
`0:/LPC/NOWA.HTM`. W `http_lpc_assets.c` zostaje tylko mały opis trasy;
HTML, JavaScript, CSS i grafika nie są już osadzone we FLASH.

Po aktualizacji firmware rozpakuj `Application/LPC2148/sdcard.zip` i skopiuj
folder `LPC` do katalogu głównego karty. Dla samego panelu wystarczy skopiować
`LPC/NOWA.HTM`. Adres `/nowa` oraz JSON `/api/nowa` pozostają bez zmian.
Brak pliku daje odpowiedź 503 z instrukcją skopiowania pakietu.
`generate_assets.py` generuje tylko metadane trasy, a `build_pages.py` tworzy
pakiet SD (w tym kompletną stronę `NOWA.HTM` z zasobów `web/`).
Usunięto `httpLpcSdBuff[5][768 KiB]`. Zastępuje go pięć buforów po 2048 B;
pojedynczy pakiet UART/ESP pozostaje ograniczony do dotychczasowych 2039 B.

Przy kolejkowaniu odpowiedzi sprawdzany jest plik i jego rozmiar, używany jako
Content-Length. Przed AT+CIPSEND odczytywana jest jedna porcja: otwarcie pliku,
sprawdzenie rozmiaru, f_lseek, f_read przez istniejący wyrównany bufor DMA,
f_close. Dopiero potem zwalniana jest blokada SD i wysyłany pakiet do ESP.
Plik ani blokada nie pozostają zajęte pomiędzy porcjami. Każdy slot ma własny
bufor, identyfikator strony i zapamiętany offset przygotowanego pakietu.

Brak pliku przed rozpoczęciem daje dotychczasową odpowiedź 503. Błąd SD lub
zmiana rozmiaru podczas transferu zamyka odpowiedź przed zapowiedzią kolejnego
pakietu. Nie można wtedy wysłać nowego statusu HTTP, bo nagłówek już wysłano.
Przerwanie połączenia/reset kolejki nie wymaga zamykania dodatkowych uchwytów.

JSON ustawień korzysta ze zwykłego bufora strony. Jeśli jest większy od tego
bufora, serializer generuje okna odpowiedzi do małego bufora pakietu. Rozmiar
oraz fingerprint całej odpowiedzi są weryfikowane przy każdej porcji; zmiana
ustawień podczas takiego transferu przerywa odpowiedź. Ta rzadko używana ścieżka
ponownie przelicza JSON dla każdej porcji i wymienia pamięć na dodatkową pracę CPU.

Biblioteki, konfiguracja USART6 i parametry transmisji ESP pozostają bez zmian.
Zmienia się tylko przygotowanie oraz pobieranie treści HTTP LPC.

Weryfikacja:

- `tests/test_lpc_storage.py`: prawdziwe API SD, offsety i zawartość, krótkie
  odczyty/błędy/zamknięcia, brak pozostawionych uchwytów i blokad.
- `tests/test_http_tx.py`: rzeczywisty nadajnik ESP, dwie różne strony SD
  przeplatane na kanałach, błąd w trakcie, JSON z serializerem okienkowym.
- `tests/test_http_sensor_routes.py`: routing, nagłówki, kolejka i CGI.
- `tests/test_lpc_pages.py`: strony/formularze i zgodność JSON/okien serializera.
