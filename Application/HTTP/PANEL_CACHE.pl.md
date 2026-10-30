# Strony /main i /set: szablon w przeglądarce, dane z H7

Dotyczy wyłącznie panelu odczytowego (`/`, `/main`) i konfiguracji portów
(`/set`, `/Set`, `/3main`) obsługiwanych przez ESP32. Pozostałe formularze,
`/nowa`, Ethernet, Google i ustawienia UART pozostają w dotychczasowej obsłudze.

- Adres strony zwraca mały dokument startowy i ewentualny wynik zapisu.
- Skrypt `/assets/h7-panels-<wersja>.js` i szablony `-read.htm`, `-set.htm`
  mają `Cache-Control: private, max-age=31536000, immutable`. Są niezależne
  od ustawień i bieżących stanów wyjść. Używają istniejących stylów,
  kontrolek, skryptów i buforów odpowiedzi; nie wymagają plików na SD.
- `/api/panel-config` zawsze ma `Cache-Control: no-store`. Zwraca 33 pozycje
  `[tryb, nazwa, PWM_lub_null]` odczytane z tego samego modelu co HTTP i LCD.
  Nazwy JSON są escapowane, a w kafelkach wstawiane przez `textContent`.
- Kafelki odczytu tworzy przeglądarka zgodnie z konfiguracją, w dotychczasowej
  kolejności grup. Nieaktywne identyfikatory odświeżania są ukryte i unikalne.
- Pomiary i stany nadal odświeża istniejące `/TME.txt`, a PWM i przekaźniki
  nadal używają tych samych komend. Konfiguracja pobierana jest dodatkowo
  co 5 sekund, z najwyżej jednym takim żądaniem w toku. Brak odpowiedzi
  nie kasuje działającego panelu; po zakończeniu żądania następuje ponowienie.
- Niezapisana edycja nazw/trybów na `/set` chroni formularz przed nadpisaniem
  zdalną konfiguracją. Zapis pozostaje POST do `/lpc.cgi/A117`, z dotychczasową
  walidacją, zapisem SD i informacją o sukcesie/błędzie. Przeładowanie porzuca
  lokalny szkic i pobiera zapisane ustawienia.

Pierwsze wejście wymaga pobrania szablonu. Następne wejścia korzystają z cache
przeglądarki. Wymuszone czyszczenie cache lub nowa wersja szablonów oznaczają
ponowne pobranie. To ogranicza przesył danych; nie usuwa przerw serwera
wynikających z aktualnej obsługi Google w ESP32.

Po zmianie `web/panels.js`, `wwwPages.c/.h` lub neutralnego generatora `/set`
uruchom:

```
python3 tools/generate_http_panel_assets.py
```

Skrypt generuje `http_panel_assets.inc`, w tym adresy z hashem zależności.
Nie wymaga zmiany konfiguracji CubeMX. Kontrola aktualności:

```
python3 tools/generate_http_panel_assets.py --check
```

Testy: `tests/test_http_panel_cache.py` uruchamia Chromium przez Playwright,
przekazując rzeczywiste HTTP przez produkcyjny dekoder i routing C. Sprawdza
cache przy ponownych wejściach, nazwy UTF-8/HTML, zapisy formularzy, zdalną
zmianę trybu, ochronę szkicu, PWM i przekaźnik. Testy routingu, ustawień
oraz transmisji nadal obejmują walidację, błędy SD i izolację kolejek.
