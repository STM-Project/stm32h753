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
Nie przeniesiono serwera HTTP, H7 HAL ani bibliotek H7 do projektu F7.

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

## Gdzie sa lokalne zmienne

Wszystkie sa w nowym pliku `.c`:

```c
ui.edit           // bufor Twojej klawiatury, 64 bajty (2 na jej indeks kursora)
ui.draft[0]       // edytowana temperatura
ui.draft[1]       // edytowana wilgotnosc
localSettings[port - 1][0] // zatwierdzona temperatura
localSettings[port - 1][1] // zatwierdzona wilgotnosc
```

`ui` i `localSettings` sa `static`: lokalne dla tego pliku i zachowuja dane
miedzy wywolaniami. Nie sa na stosie funkcji. Bez dodatkowej konfiguracji
zapis jest tylko w RAM i znika po resecie/zasilaniu; informacja jest na LCD.
Dane nie sa wysylane przez siec i nie zmieniaja rzeczywistych alarmow ani GPIO.

Odczyt zatwierdzonej konfiguracji z kodu aplikacji:

```c
#include "../SensorThresholds/LCD_SensorThresholds.h"
const LCD_SensorThreshold *t = LCD_SensorThresholdGet(1, 0); // port 1, temperatura
const LCD_SensorThreshold *h = LCD_SensorThresholdGet(1, 1); // port 1, wilgotnosc
if (t) {
    // t->name, t->max1, t->max2, t->min1, t->min2,
    // t->hysteresis, t->correction, t->delay_seconds
}
```

Getter zwraca NULL przed pierwszym wejsciem lub dla blednego portu.
Opcjonalny `lcdThresholdSave` moze polaczyc Zapisz z Twoim mechanizmem
trwalego zapisu; domyslnie NULL. Callback dostaje port i dwie struktury,
musi zwrocic 1 po sukcesie. Blad pozostawia szkic i stare ustawienia.
Nie przechowuj przekazanego wskaznika szkicu po powrocie z callbacku.
Wszystkie operacje z modelu/GUI wykonuj z tego samego zadania lub synchronizuj.

## Sprawdzenie

`python3 tests/lcd_sensor_thresholds_test.py` sprawdza faktyczny kod panelu
z atrapami API LCD/Keyboard: walidacje liczb i nazw, jednostki, edycje obu
czujnikow, zapis/anulowanie, porty, callback z bledem i granice rysowania.
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
