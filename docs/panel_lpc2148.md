# Panel graficzny LPC2148 na STM32H753

Adres: `http://<adres STM32>/nowa`. Dotychczasowy panel `/` i `/main` oraz
ustawienia `/set` pozostają dostępne. Menu zawiera link do panelu graficznego.

Układ trzech termometrów, wykresu, siedmiu wejść i czterech wyjść pochodzi z
`LPC2148_NEW300/System_folder/System/Strona/czolowka.htm`. Zachowano oryginalne
kafelki i termometry SVG. Skrypty odczytujące format danych LPC zastąpiono
adapterem STM32, odświeżającym `/api/nowa` co sekundę bez nakładania żądań.
Wykres przechowuje ostatnie 300 odczytów w przeglądarce; nie jest archiwum SD.

## Pliki i pamięć

- `web/lpc2148/index.html`: przeniesiony układ, lokalne style i nawigacja.
- `web/lpc2148/panel.js`: odczyt JSON, termometry, wykres i przyciski PK.
- `web/lpc2148/jquery/` i `svg/`: biblioteki i grafika z dostarczonego LPC2148.
  W CSS odwołania do brakujących wariantów ikon wskazują dostarczoną ikonę.
- `Application/HTTP/http_lpc_panel.c/.h`: pomiary, przypisanie portów i JSON.
- `Application/HTTP/http_lpc_assets.c`: wygenerowane, stałe zasoby Flash.
- `tools/generate_lpc_assets.py`: regeneracja po zmianie HTML/JS/CSS/obrazów.

Generator łączy skrypty i style z HTML, kod SVG osadza jako tekst, a obrazy
CSS jako data URI. Otwarcie `/nowa` pobiera tylko jeden dokument, bez żądań
o biblioteki, CSS i SVG. Pomiary i przyciski nadal używają osobnych żądań HTTP;
serwer zamyka połączenie po każdej odpowiedzi. Nie jest to jedno trwałe
połączenie TCP na całą sesję.

Strona i biblioteki mieszczą się w wewnętrznej pamięci Flash 2 MB, dlatego
nie wymagają kopiowania plików na SD ani dostępu do Internetu. Zasoby są wysyłane
bez kopiowania całych bibliotek do RAM. Karta SD nadal służy do zapisu ustawień.
Zachowano biblioteki dostarczone w archiwum, w tym Highcharts; ich warunki
licencyjne obowiązują również przy dalszym użyciu.

## Domyślne przypisanie

Są to numery logiczne portów, **nie numery pinów GPIO**.

| Element panelu | Porty logiczne |
| --- | --- |
| Temperatura 1–3 | 1, 2, 3 |
| Wejście 1–7 | 4, 5, 6, 7, 8, 9, 10 |
| PK1–PK4 | 28, 29, 30, 31 |

Domyślne tryby `/set` odpowiadają temu przypisaniu: porty 1–3 — temperatura,
4–10 — wejście cyfrowe, 28–31 — przekaźnik. Pozostałe porty są domyślnie
wyłączone. Konfiguracja zapisana wcześniej na SD ma pierwszeństwo i nie jest
nadpisywana aktualizacją firmware. Na urządzeniu z wcześniejszą konfiguracją
ustaw te tryby w `/set` i wybierz „Zapisz Tryb”.

Tablice `HTTP_LpcTemperaturePorts`, `HTTP_LpcInputPorts` i
`HTTP_LpcOutputPorts` są w `http_lpc_panel.c`. PK1–PK4 wykorzystują istniejące
polecenia `/mobile/pk001`–`004`; ich numery muszą odpowiadać mapowaniu
`RelayStatesData.outputs`. Przyciski są dostępne dla trybu przekaźnika/IR
ustawionego w `/set`. Panel nie zmienia zapisanych trybów automatycznie.
Nazwy wejść/wyjść pochodzą z `/set`, a nazwy, progi termometrów i korekcja
z `/temp001`–`/temp003`. Jeśli zakres progów jest pusty, skala wynosi -10..50°C.

## Przekazywanie rzeczywistych odczytów

```c
#include "HTTP/http_lpc_panel.h"  /* z pliku w Application */
#include "relay_states.h"

/* Po odczycie czujników: wartości surowe, w dziesiątych częściach stopnia. */
HTTP_LpcSetTemperature(1, 251, 1);  /* 25,1°C */
HTTP_LpcSetTemperature(2, -123, 1); /* -12,3°C */
HTTP_LpcSetTemperature(3, 405, 1);  /* 40,5°C */
HTTP_LpcSetInput(4, 1, 1);         /* pierwsze wejście: ON */
HTTP_LpcSetInput(5, 0, 1);         /* drugie wejście: OFF */

/* Gdy pomiar znika/kończy się jego dopuszczalny czas ważności: */
HTTP_LpcSetTemperature(1, 0, 0);
HTTP_LpcSetInput(4, 0, 0);

/* Po wykonaniu/odczytaniu wyjścia przez sterownik sprzętowy: */
RELAY_UpdateState(28, 1);
/* Gdy sterownik nie zna stanu: */
RELAY_InvalidateState(28);
```

Funkcje zwracają 1 dla poprawnego portu/argumentów, 0 dla błędu. Temperatura
z ważnym pomiarem musi mieścić się w -55..150°C. Korekcja z ustawień jest
stosowana raz, podczas generowania JSON. Do funkcji należy przekazać odczyt
przed korekcją. Zmienne pomiarów są w `HTTP_LpcMeasurementsData`; początkowo
wszystkie odczyty są nieważne. Nie są odtwarzane z SD. Wywołuj funkcje w jednym
zadaniu właściciela albo zapewnij synchronizację z zadaniem HTTP.

Projekt nie zawiera obecnie sterowników czujników temperatury ani przypisania
GPIO przekaźników. Te funkcje są punktem podłączenia Twojego kodu sprzętowego;
nie uruchamiają pomiaru ani nie przełączają pinów samodzielnie. Do czasu podania
pomiarów strona pokazuje „brak danych”, a bez zgłoszenia stanu przekaźnika —
„brak potwierdzenia”. Zadana komenda jest oznaczona osobno jako „zadano ON/OFF”.
