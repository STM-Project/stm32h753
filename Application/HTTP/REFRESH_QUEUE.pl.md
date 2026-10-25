# Kolejka odświeżeń HTTP przez ESP32

Scalane są wyłącznie dokładne endpointy odczytowe: `/TME.txt`, `/api/nowa`
i `/api/lpcdata`. Adresy CGI, zapisy formularzy, polecenia PK/PWM, HTML oraz
pliki SD zachowują oddzielną obsługę. Biblioteki i ustawienia USART6 są bez zmian.

Odczyt rezerwuje slot odpowiedzi, ale nie generuje od razu migawki. Pierwsza
próba wysłania przygotowuje aktualne dane i kopiuje je do pozostałych oczekujących
klientów tego samego endpointu. Nie przetwarzamy więc kolejno starych migawek.
Każdy klient nadal dostaje pełną odpowiedź HTTP; nie anulujemy żądań innej karty
przeglądarki ani nie wymagamy obsługi pustych odpowiedzi przez starszy JavaScript.
Fizyczna kolejka nadal ma maksymalnie pięć slotów, po jednym na kanał ESP.

Nowszy odczyt unieważnia tylko niewysłane migawki tego samego rodzaju. Obsługa
innego żądania (w tym zapisu) unieważnia niewysłane odczyty, żeby po zapisie
pokazały aktualne ustawienia/stany. Nagłówek i treść są zamrożone, gdy ESP
przyjmie pierwsze AT+CIPSEND; nie wolno ich już nadpisywać ani zmieniać długości.
Zapisów nie scala się i nie usuwa. Istniejąca ochrona przed podwójnym wykonaniem
tego samego żądania na zajętym kanale pozostaje aktywna.

Trzy starsze panele `/TME.txt` mają jedną aktywną operację XMLHttpRequest.
Tykanie timera podczas trwającego odczytu jest pomijane. Kolejne odświeżenie
może wystartować po zakończeniu albo po błędzie/timeoutcie (10 s). Zdarzenia ze
starego obiektu XHR nie odblokują nowszego żądania. Nie dotyczy to funkcji
zapisywania formularzy, PWM ani przełączania przekaźników. Nowsze panele JSON
już miały własne blokady `busy` i nie wymagały aktualizacji plików SD.

Testy:

- `tests/test_http_tx.py`: leniwa migawka, scalanie oczekujących odczytów,
  zamrożenie aktywnego nagłówka/treści, zapis SD przy zajętej kolejce i ponowne
  odczytanie po zapisie; dotychczasowe testy transmisji i strumieniowania LPC.
- `tests/test_http_refresh_browser.py`: rzeczywiste trzy funkcje JavaScript,
  wolne XHR, pomijanie kolejnych tyknięć, błędy/timeouty/abort oraz stare zdarzenia.
