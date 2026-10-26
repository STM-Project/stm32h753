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
sprawdzenie rozmiaru, f_lseek do granicy sektora 512 B i f_read pełnych
sektorów przez istniejący wyrównany bufor DMA,
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

## Odczyt sektorowy i cache H7

Nie wolno utożsamiać wyrównania początku bufora z wyrównaniem wszystkich
odczytów DMA wewnątrz FatFs. Przy offsetach HTTP co 2039 B f_read może najpierw
skopiować końcówkę sektora przez CPU, a dalsze sektory czytać DMA bezpośrednio
za tą końcówką. Wygenerowany SD_read wybiera szybki DMA już przy wyrównaniu
4 B; unieważnienie cache dla takiego odczytu może objąć brudny koniec prefiksu
skopiowanego przez CPU. Skutkiem są podmienione bajty HTML przy zachowanej
długości całej odpowiedzi, a następnie błędy składni JavaScript/CSS.

Przesłana odpowiedź /nowa miała 505213 B, tak jak oryginał, lecz 823 bajty
różniły się; pierwsze uszkodzenie było na pozycji 8188..8191. Układ większości
błędów odpowiada końcówkom linii cache przed granicami sektorów przy pakietach
2039 B. To problem danych, nie brak bibliotek w paczce SD.

LPC_StorageReadPageChunk wyrównuje offset odczytu w dół do 512 B i długość
w górę do pełnych sektorów (ostatni odczyt jest ograniczony końcem pliku).
Dopiero z gotowego bufora kopiuje żądane okno HTTP. Większe okno przecinające
4 KiB stagingu obsługiwane jest w pętli, bez dodatkowego bufora. Do FatFs nie
trafiają już niepełne początki sektorów, więc nie ma mieszanego prefiksu CPU
przed kolejnym DMA w tej samej linii. Nie zmieniono sd_diskio.c, konfiguracji
CubeMX, USART6, rozmiaru pakietów ESP ani rezerwacji pełnych stron w SDRAM.

Test SD wymusza wyrównanie każdego f_lseek/f_read i sprawdza wszystkie 512
przesunięć sektora dla okna 4096 B, całą stronę /nowa, przypadki EOF, błędy
odczytu/zamknięcia i zwalnianie blokady. Poprawność rzeczywistej transmisji
po zmianie wymaga ponownego sprawdzenia odpowiedzi pobranej z płytki.
