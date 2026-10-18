# Panel graficzny LPC2148 na STM32H753

Adres: `http://<adres STM32>/nowa`. Dotychczasowy panel `/` i `/main` oraz
ustawienia `/set` pozostają dostępne. Menu zawiera link do panelu graficznego.

Układ trzech termometrów, wykresu, siedmiu wejść i czterech wyjść pochodzi z
`LPC2148_NEW300/System_folder/System/Strona/czolowka.htm`. Zachowano oryginalne
kafelki i termometry SVG. Skrypty odczytujące format danych LPC zastąpiono
adapterem STM32, odświeżającym `/api/nowa` co sekundę bez nakładania żądań.
Wykres przechowuje ostatnie 300 odczytów w przeglądarce; nie jest archiwum SD.

## Pliki i pamięć

- `Application/LPC2148/web/index.html`: przeniesiony układ, lokalne style i nawigacja.
- `Application/LPC2148/web/panel.js`: odczyt JSON, termometry, wykres i przyciski PK.
- `Application/LPC2148/web/jquery/` i `svg/`: biblioteki i grafika z dostarczonego LPC2148.
  W CSS odwołania do brakujących wariantów ikon wskazują dostarczoną ikonę.
- `Application/LPC2148/http_lpc_panel.c/.h`: pomiary, przypisanie portów i JSON.
- `Application/LPC2148/http_lpc_assets.c`: wygenerowane, stałe zasoby Flash.
- `Application/LPC2148/generate_assets.py`: regeneracja po zmianie HTML/JS/CSS/obrazów.

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

## Źródło odczytów — bufor SDRAM

`/api/nowa` odczytuje bezpośrednio `HttpRefr_sdram`, czyli ten sam bufor, który
obsługuje `/TME.txt`. Aktualizowanie bufora przez dotychczasowy kod od razu
aktualizuje nowy panel. Nie trzeba dodatkowo wywoływać funkcji publikujących.

Format danych (offsety liczone od zera):

- Wyjścia: od bajtu 25; kolejność jak `RelayStatesData.outputs`.
  Kody 0/2 = OFF, 1/3 = ON.
- Wejścia: od bajtu 39; porty 1–14 w kolejnych bajtach.
  Kody 0/2 = ON, 1/3 = OFF (logika aktywna stanem niskim).
  Panel wykorzystuje bajty 42–48 dla portów 4–10.
- Temperatury: pięć znaków od offsetu `73 + (port-1)*12`.
  Pierwsze trzy temperatury zaczynają się w bajtach 73, 85, 97.
  Przykładowo ` 25.1`, `-78.3`, `-56.0`.

Wartości temperatur z bufora są gotowe do wyświetlenia, tak jak na dotychczasowej
stronie. Korekcja nie jest dodawana drugi raz. Progi skali termometrów i nazwy
nadal pochodzą z ustawień. Niepełne lub nieprawidłowe pola dają „brak danych”;
termometry pozostają widoczne, ale przygaszone. Nie są zastępowane zerem.
Zadane stany przekaźników nadal są oznaczone osobno jako „zadano ON/OFF”.
Przed poleceniem przycisku odczytywany jest bieżący stan wyjścia z bufora.

Bufor należy aktualizować w zadaniu właściciela lub synchronizować aktualizację
z zadaniem HTTP. Panel nie tworzy pomiarów samodzielnie: pokazuje wartości
umieszczone w `HttpRefr_sdram`, w tym dane testowe, jeśli takie zawiera bufor.

Alternatywny interfejs `HTTP_LpcSetTemperature()`, `HTTP_LpcSetInput()` oraz
`HTTP_LpcPanelJson()` pozostał dostępny dla kodu korzystającego z osobnej
struktury `HTTP_LpcMeasurementsData`. Nie jest domyślnym źródłem `/nowa`.

## Kolor i wysyłanie strony

Kolor cieczy termometru zmienia się płynnie od niebieskiego przy dolnym progu
przez fioletowy do czerwonego przy górnym progu. Wartości poza skalą przyjmują
kolor najbliższego końca skali, a liczbowy odczyt pozostaje nieobcięty.

Kolejkę HTTP modyfikuje wyłącznie zadanie Wi-Fi. Callback timeoutu zgłasza reset,
który to zadanie wykonuje przed kolejną obsługą. Każdy pakiet ma sprawdzony
indeks fragmentu, offset i długość; spóźnione/powtórzone prompty i potwierdzenia
nie przesuwają kursora ponownie. Test `tests/test_http_tx.py` przesyła całą
stronę i sprawdza przeplatanie kanałów oraz błędne zdarzenia pod ASan/UBSan.

## Pozostałe strony LPC2148 na SD

Moduł znajduje się w `Application/LPC2148` (źródła C, oryginalne szablony
`source_pages`, zasoby `web`, importer i generator). Numerowane strony są
na SD, ponieważ osadzenie wszystkich 195 dokumentów przekroczyłoby Flash.
Dotychczasowe `/nowa` pozostaje we Flashu i działa bez tych plików.

1. Wypakuj `Application/LPC2148/sdcard.zip` do głównego katalogu karty SD.
   Powinny powstać pliki `LPC/P00001.HTM` … `LPC/P00195.HTM`.
2. Wgraj zaktualizowany firmware i włóż kartę.
3. Otwórz `/lpc`: spis pokazuje powiązanie pliku LPC z `/nowa1` … `/nowa195`.
   Pełna tabela jest też w `routes.json` i w `LPC/STRONY.csv` paczki.

Każdy HTML zawiera skrypty, style i obrazy. Kolejne żądania dotyczą danych
(`/api/lpcdata`, `/api/lpcN`) oraz poleceń; nie pobierają bibliotek ani obrazów.
Pomiary pochodzą z `HttpRefr_sdram`. Dostępnych jest do 20 temperatur,
15 wejść i 13 wyjść; brak pomiaru oznaczany jest jako „brak danych”.

Formularze używają POST, parametrów wspólnych dla wariantów klasycznych,
mobilnych i SSL oraz pliku `LPCSET.BIN` z CRC i kopią `LPCSET.BAK`.
To osobny zestaw parametrów LPC, dostępny przez `LPC_GetSetting()` i
`LPC_GetSavedSetting()`. Nie zastępuje `HTTP_AllSettingsData`.
Zapis nazwy LPC zmienia nazwę na panelach. Domyślne nazwy, progi i wybrane
parametry LAN są odczytywane z konfiguracji H7, dopóki nie zapisano ich w LPC.
Pozostałe parametry LPC są przechowywane, ale wymagają podłączenia logiki
sprzętowej H7, aby sterowały timerami, siecią lub automatyką.
Aktywne tryby portów H7 nadal ustawia się w `/set`. Polecenia resetowania,
transmisji i odczytu sprzętu specyficznego dla LPC nie są emulowane: zwracają
HTTP 501. Warianty SSL są wariantami wyglądu, nie dodają HTTPS do firmware.

HTTP ma pięć niezależnych buforów stron po 768 KiB w SDRAM, a dwa bufory
ustawień zajmują razem około 2,95 MiB SDRAM. Nie obciążają stosu zadania.
Wysyłanie korzysta z dotychczasowej kolejki i kontroli granic pakietów.
Brak pliku na SD daje HTTP 503 z instrukcją i odnośnikiem do `/lpc`.

Regeneracja po zmianie oryginałów:

```sh
python3 Application/LPC2148/import_pages.py /sciezka/do/LPC2148/System
python3 Application/LPC2148/build_pages.py
python3 Application/LPC2148/build_pages.py --check
python3 tests/test_lpc_pages.py
```

## Poprawka granicy SDRAM

Konfiguracja FMC (8 bitów kolumn, 12 bitów wierszy, cztery banki, szyna 32-bit)
oraz `SDRAM_DEVICE_SIZE` wskazują 16 MiB. Linker i MPU mają teraz ten sam limit.
Poprzednie bufory 5 × 2 MiB powodowały przekroczenie fizycznej pamięci przez
`httpRequestStream`; całkowita sekcja SDRAM miała 16 894 976 bajtów.
Bufory 5 × 768 KiB mieszczą wszystkie aktualne strony z zapasem, a test paczki
sprawdza limit każdego HTML. Nadmiar pamięci zostanie teraz odrzucony przy linkowaniu.

## Samodzielne dokumenty `/nowaN` i diagnostyka ładowania

Każdy dokument SD zawiera własne biblioteki JavaScript, style, SVG, ikony oraz
obraz tła `tlo364.jpg` jako dane wewnątrz HTML. Skrypty znajdują się po treści,
a widoczność strony przed zakończeniem transmisji nie trzeba czekać na jQuery
Mobile. Nagłówek CSP blokuje pobieranie zewnętrznych skryptów i obrazów.
Generator sprawdza odwołania do zasobów. Test Chromium otwiera wszystkie 195
stron, kontroluje ich widoczność oraz dozwolone żądania: dokument, konfigurację
`/api/lpcN`, odczyty `/api/lpcdata`. Dodatkowe żądania skryptów, stylów, fontów
lub obrazów są błędem testu. Odświeżanie danych nadal wymaga kolejnych żądań API.

Po aktualizacji wymień folder `LPC` na SD zawartością nowego `sdcard.zip`
i wgraj nowy firmware. Samo wgranie firmware nie aktualizuje HTML na karcie.
Czytnik stron używa wyrównanego bufora pośredniego w wewnętrznym RAM
i odczytów po maksymalnie 4096 bajtów, kopiowanych do bufora odpowiedzi SDRAM.
Na konsoli są komunikaty `HTTP LPC: ... odczyt SD`, `SD OK, ... bajtow`,
błąd z kodem FatFs lub informacja o wysłaniu całej odpowiedzi. Brak pliku,
niepełny odczyt, zajęta karta i przekroczenie bufora zwracają HTTP 503
z konkretną przyczyną na stronie.
