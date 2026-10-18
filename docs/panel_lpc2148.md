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
