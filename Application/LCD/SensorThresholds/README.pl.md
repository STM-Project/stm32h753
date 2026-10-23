# Ustawienia czujnikow — ekran LCD 10

Uruchomienie w zadaniu GUI:

```c
SCREEN_SetNr(10);
```

Nowa funkcja `LCD_SensorThresholds(mode)` jest wywolywana przez istniejaca
petle ekranow. Pliki sa w oddzielnym folderze `Application/LCD/SensorThresholds`.

## Zrodlo wygladu i pol

Odwzorowanie strony z repozytorium
[STM-Project/stm32h753](https://github.com/STM-Project/stm32h753),
master, commit `5af0e949636384b8d84d5ab32fcf9d2adabbcad1`:

- `Application/HTTP/http_sensor_page.c`, funkcja `PageCard`;
- `Application/HTTP/http_sensor_settings.h` i `.c`, pola, jednostki i walidacja;
- `Application/wwwPages.c`, `HttpStyle`: czarne tlo, szare zaokraglone kafelki,
  zielone etykiety, czerwone naglowki, gradientowe pola i niebieski Zapisz.

Dwa kafelki sa dopasowane do 800x480. Nazwy i wartosci nie sa obrazkiem:
rysuje je Twoje API LCD oraz fonty Arial 12, Arial 14, Arial 18 bold z SD.
Podglad `preview-800x480.png` uzywa zastepczej Liberation Sans.
Panel dziala w projekcie H7 i korzysta z jego modelu HTTP.

## Obsluga

Dotknij pola: otworzy sie Twoja klawiatura z `Keyboard.c`, przez
`KEYBOARD_StartUp` i `KEYBOARD__ServiceSetTxt`. `alt` udostepnia cyfry i znaki.
`Gotowe` zatwierdza pole, `Anuluj` lub klawisz exit odrzuca biezacy wpis.
Enter w natywnej klawiaturze jest nowa linia — nie zatwierdza ustawienia.
Nazwy sa jednoliniowe, maksymalnie 30 bajtow, jak w modelu H7.

Progi max1/max2, min1/min2, histereza i korekta sa w dziesiatych czesciach
stopnia lub procenta: `255` oznacza `25.5`. Klawiatura przyjmuje np.
`-12.3`, `+25.5`, `80`, rowniez przecinek zamiast kropki.
Progi wilgotnosci sa 0..100%, histereza nieujemna; cztery progi sa niezalezne
(tak jak na stronie H7). Korekta wilgotnosci: -100..100%.
Temperatura: zakres typu int16 w jednostkach 0.1 C, bez dopisywania nowych
ograniczen do modelu H7. Czas zwloki: 0..5999 sekund, na kafelku jako mm:ss;
w edytorze podaj liczbe SEKUND, np. `90` daje `01:30`.

`Zapisz` zapisuje oba kafelki danego portu. `Odrzuc` przywraca ostatnio
zapisane wartosci. Strzalki wybieraja port 1..20; niezapisane zmiany blokuja
zmiane portu do uzycia Zapisz lub Odrzuc. Wyjscie z ekranu odrzuca szkic.

## Wspolne ustawienia LCD / HTTP / SD

`LCD_SensorThreshold` jest aliasem `HttpSensorSettings`. Jedyny model
zatwierdzonych wartosci to `http_sensor_settings.c`, uzywany rowniez przez
formularze HTTP. LCD nie ma osobnej tablicy zatwierdzonych ustawien.

- `ui.draft[0]` / `[1]`: niezapisana edycja temperatury / wilgotnosci.
- `ui.base`: kopia wartosci z chwili rozpoczecia edycji, do wykrywania konfliktu.
- `ui.edit`: bufor klawiatury, 64 bajty (2 zarezerwowane przez Keyboard.c).

Most `Application/HTTP/http_sensor_lcd.c` przekazuje GUI spojne kopie
ustawien i pojedyncze zadanie zapisu. Kopie i skrzynka zadania sa chronione
krotkimi sekcjami krytycznymi FreeRTOS; zapis SD nigdy nie odbywa sie w sekcji
krytycznej. Model, bufor serializacji oraz FatFs pozostaja w zadaniu WiFi.
`HTTP_SensorLCDService()` jest wywolywane po inicjalizacji modelu i przed
kazdym oczekiwaniem w petli WiFi. Zapis z LCD budzi to zadanie powiadomieniem.

Po zmianie z WWW otwarty LCD odswieza pola automatycznie przy kolejnym
UPDATE, jezeli nie trwa edycja ani dotyk. Aktywna edycja pozostaje na ekranie;
jesli WWW zmieni ten sam port, zapis starego szkicu zostanie odrzucony.
`Odrzuc` pobiera wtedy najnowsze wartosci. Zmiana innego portu nie blokuje zapisu.

`Zapisz` wysyla oba czujniki do modelu. Wynik 1 z `LCD_SensorThresholds`
pojawia sie dopiero po potwierdzonym zapisie, nie po dodaniu do kolejki.
Obie drogi uzywaja tej samej walidacji i adaptera SD: `0:/h7sense.bin`,
kopia `.bak`, plik przejsciowy `.tmp`. Po sukcesie aktualizowane sa rowniez
pola `HTTP_AllSettingsData.sensors`. Po bledzie SD szkic zostaje na LCD,
a zatwierdzone dane HTTP pozostaja bez zmian. Po resecie oba interfejsy
korzystaja z wartosci odczytanych z SD. Zapis nie steruje GPIO.

Odczyt z zadania GUI:

```c
HttpSensorSettings sensors[2];
if (HTTP_SensorLCDRead(1, sensors)) {
    // sensors[0].max1: temperatura, sensors[1].max1: wilgotnosc, jednostki 0.1
}
```

`LCD_SensorThresholdGet(port, humidity)` rowniez zwraca kopie odczytu;
wskaznik jest wazny do kolejnego wywolania tego gettera. NULL oznacza
bledny port lub model jeszcze niewczytany przez WiFi. Przed gotowoscia
modelu edycja LCD jest wstrzymana. Usunieto opcjonalny `lcdThresholdSave`:
nie trzeba rejestrowac callbacku ani tworzyc drugiego pliku konfiguracji.

## Sprawdzenie

`python3 tests/lcd_sensor_thresholds_test.py` sprawdza faktyczny kod panelu
z atrapami API LCD/Keyboard: walidacje liczb i nazw, jednostki, edycje obu
czujnikow, zapis/anulowanie, porty, HTTP -> LCD, LCD -> HTTP, blad zapisu,
konflikt edycji, odczyt zapisanego formatu i granice rysowania.
Mock nie zastepuje testu fizycznego dotyku, SD-fontow i natywnej klawiatury.
Pelna kompilacja ARM obejmuje prawdziwy `Keyboard.c` bez jego modyfikacji.


Dotyk klawiatury uzywa zakresu ID 180..215. To istotne: natywny Keyboard.c
przechowuje startTouchIdx w uint8_t. Wczesniejsze ID 62100 bylo obcinane,
przez co ekran nie odbieral klawiszy. Rejestracja pozostaje w Twoim
KEYBOARD__ServiceSetTxt; ekran mapuje odebrane ID na KEY_Q..KEY_style,
odswieza KEY_All_release oraz obsluguje exit. Keyboard.c nie jest zmieniony.


Wyglad klawiatury: oryginalna paleta z SCREEN_FontsLCD/SCREEN_VisualParamLCD
(tlo #383838, okno #313131, klawisze #3B3B3B, ramki #D0D0D0,
wcisniety klawisz #606060 z ramka #BABABA, opis #999999).
Odstep miedzy klawiszami: 10 px, jak w pierwotnym wywolaniu; bold=0.
Okno zaczyna sie na X=24, Y=115. KeysAutoSize i domyslny padding 10 px,
jak w oryginale. Padding jest zmniejszany tylko jesli dany plik fontu
nie pozwala zmiescic klawiatury w 480-pikselowej wysokosci ekranu. Keyboard.c nadal bez zmian.


Przytrzymanie klawisza: ekran przekazuje kolejne zdarzenia tego samego klawisza
bez KEY_All_release. Przerwy pomiedzy zdarzeniami powtarzania, gdy palec nadal
jest na ekranie, nie zwalniaja klawisza. Dopiero puszczenie palca lub przejscie
na inny klawisz wysyla release. Dzieki temu oryginalny Keyboard.c sam wybiera
znaki alternatywne (np. q -> 0, a -> a z ogonkiem, f -> minus/plus).
Czas i mapa znakow pozostaja takie jak w Twoim Keyboard.c, bez jego zmiany.
