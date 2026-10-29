# Dwa ekrany HTTP na LCD 800x480

11 — panel odczytowy `/main`: trzy kolumny pomniejszonych kafelków, czarne tło,
szare obramowanie/tarcza, jasne wartości, zielone nazwy i niebieskie przyciski
przekaźników. Aktywne porty i ich kolejność pochodzą z tego samego zestawu
trybów co HTTP: czujniki, liczniki, wejścia, EOL, ADC/PT1000, PWM, przekaźniki/IR,
timery i termostaty. Wyłączone porty są pomijane. Liczniki mają wyższe kafelki.

12 — ustawienia `/set`: wszystkie 33 porty, nazwa, wybór trybu, krótki
przycisk konfiguracji z numerem portu i aktualne odczyty. Temperatura/RH mają
dwa pola wartości; PWM ma procent i suwak, a przekaźnik stan ON/OFF. Przycisk zmienia wygląd przy wciśnięciu i wraca po puszczeniu.
Nie uruchamia dalszego ekranu i nie zapisuje ustawień.
Obrys wiersza ma 2 px; obrysy panelu odczytowego pozostają bez zmian. Przeciągnięcie pionowe
anuluje kliknięcie, aby przewijanie nie wykonywało przypadkowej akcji.

Oba ekrany są w tej samej pętli next/prev oraz debug `=` (teraz 0..12).
Można je również wybrać w kodzie przez SCREEN_SetNr(11) / SCREEN_SetNr(12).
Pasek przewijania jest po prawej, x=738..759, niezależnie od pola next
x=770..799. Kafelki/listę można przewijać palcem. Góra, środek i dół treści
są obcinane pikselowo do obszaru y=72..439; nagłówek i stopka pozostają stałe.

## Wspólne dane i sterowanie

HTTP_LCDPanelService działa tylko w właścicielskim zadaniu WiFi/HTTP. Kopiuje
nazwy i tryby przez te same HTTP_SettingsPortName/Mode co strony HTTP,
a pomiary z tego samego protokołu HttpRefr_sdram (`/TME.txt`): stany wejść,
EOL, temperatura/RH, ADC, PT1000 i liczniki. Alarmy/cold zachowują kolory HTTP.
Brak pola daje `---`, nie wymyślony pomiar. Nie są kopiowane hasła sieciowe.
LCD odczytuje krótką, chronioną migawkę, a nie zmieniany równolegle model.

Suwaki PWM portów 21..23 przekazują zakres 0..100 do RELAY_RequestPwm,
tej samej funkcji co HTTP. Przycisk przekaźnika/IR po puszczeniu przekazuje
polecenie do RELAY_RequestToggle, z uwzględnieniem stanu `/TME.txt`, jak HTTP.
Polecenia są wykonywane przez zadanie WiFi. Próbki PWM są scalane osobno dla
kanału (ostatnie ustawienie), kolejne kliknięcia przekaźników są zliczane.
Aktualny tryb portu jest ponownie sprawdzany przed wykonaniem polecenia.
Zmiana przez HTTP wraca do migawki LCD, a polecenie LCD jest widoczne HTTP.
To wspólny model poleceń/odpowiedzi, a nie nowy sterownik GPIO: nie dodano
przypisań fizycznych pinów, których nie posiada dotychczasowa obsługa HTTP.

Migawka aktualizowana jest do czterech razy na sekundę tylko przy aktywnym
ekranie albo po poleceniu. W tym czasie oczekiwanie WiFi ograniczane jest
do 250 ms, żeby LCD działał również bez zapytań przeglądarki. Gdy ekran jest
zamknięty, wraca dotychczasowy czas oczekiwania. Nie uruchomiono nowego wątku.

## Rysowanie i pamięć

Nie ma dodatkowego framebuffera ani kopii całej przewijanej strony w SDRAM.
Pierwsze 800x368 słów pLcd służy do kompozycji widocznego obszaru. W pozostałej
części tego samego pLcd jest mały bufor kafelka/wiersza; natywne fonty rysują
do niego przez LCD_StrChangeColorWindow. Gotowy, przycięty kafelek jest kopiowany
do widoku. Dopiero kompletny widok idzie do DMA2D — bez pokazywania pustego tła
pomiędzy kafelkami. Wymiary i rozdzielenie obszarów kontrolują _Static_assert.

Bez zmiany danych/dotyku nie ma ponownego wysyłania widoku. Zmiana samego
zegara odrysowuje nagłówek, zmiana pojedynczego portu lub stanu przycisku odrysowuje tylko jego widoczny
kafelek/wiersz. Dopiero przewijanie i zmiana układu portów odrysowują viewport.
Strzałki nawigacji są chronione istniejącą maską i znikają po 5 sekundach.
Fonty: Arial 12, Arial 16, Arial 28 bold, z dotychczasowej karty SD.
Nie zmieniono CubeMX, sterowników HAL ani zawartości SD.

## Weryfikacja

- tests/test_http_lcd_panels.py: rzeczywisty most danych i model RelayStates,
  temperatury/RH/ADC/PT1000/EOL, kolejka PWM, oba kierunki synchronizacji,
  liczenie kliknięć, aktualny tryb i przetrwanie polecenia po EXIT.
- tests/test_lcd_http_panels.py: rzeczywiste dwa ekrany, 33 porty, obcinanie
  tekstu, skrajne położenia suwaka i treści, PWM 0/100, klik/drag przekaźnika,
  naciśnięcie/puszczenie przycisku ustawień bez zapisu/nawigacji.
- Podglądy generowane przez ten sam kod rysowania, z czcionką komputerową
  zbliżoną do Arial, są w Debug/cloud/http-lcd-previews/*.png (800x480).
  Podgląd nie jest pomiarem wydajności ani testem fizycznego dotyku na H7.

## Rozwijany wybór trybu

Dotknij pola trybu w ustawieniach. Lista wybiera dropdown albo dropup według
miejsca nad/pod polem i zawsze mieści się w obszarze widoku. Zawiera dokładnie
te same zestawy opcji co HTTP dla portów 1..14, 15..16, 17..20, 21..23,
24..27 oraz 28..33. Aktualny wybór jest oznaczony, dotyk daje podświetlenie,
a dotknięcie poza listą ją zamyka, bez przekazania kliknięcia do przycisku pod nią.

Wybór typu w dropdownie zmienia wyłącznie lokalny szkic LCD. Szkic przetrwa
przewijanie i przejście next/prev do innego ekranu; restart go usuwa.
Przycisk „Zapisz” w stałym nagłówku zatwierdza wszystkie zmienione typy jednym
formularzem HTTP i jednym zapisem na SD. Wciśnięcie daje efekt wizualny,
a zapis następuje dopiero po puszczeniu palca wewnątrz przycisku.
Podczas zapisu przycisk pokazuje „Zapis...”, a dropdowny są zablokowane.
Bez zmian przycisk nie wykonuje zapisu SD.

HTTP_LCDPanelSaveModes przekazuje niezmienną paczkę do właścicielskiego
zadania HTTP/WiFi. HTTP_SettingsApplyTarget i HTTP_SettingsBindingsCommitted
zapewniają tę samą walidację i trwały zapis co HTTP. Wszystkie zmienione porty
zapisują się razem albo żaden. Pozostałe porty korzystają z najnowszych wartości
HTTP, a nazwy nie są modyfikowane. Równoczesna zmiana przez HTTP tego samego
edytowanego portu powoduje odmowę: wybierz typ ponownie i naciśnij „Zapisz”.
Przy błędzie SD szkic pozostaje na LCD i można ponowić zapis.
Udany zapis usuwa szkic; HTTP pokazuje go przy kolejnym pobraniu /set.

Zmiany zapisane przez HTTP automatycznie odświeżają LCD (do 4 Hz), o ile
port nie ma niezapisanego lokalnego wyboru. Wartości dla lokalnego typu innego
niż zapisany pokazują „---”; taki szkic nie pozwala sterować wyjściem
niezgodnie z jego faktycznym typem. Odczyty w ustawieniach używają większej
czcionki Arial 18 bold zamiast Arial 12.

Popup używa tego samego obszaru scratch w pLcd, bez dodatkowej bitmapy SDRAM.
Test ekranu generuje podglądy dropdown/dropup i sprawdza dobór opcji, krawędzie,
wybór wspólny z HTTP, zamknięcie bez akcji pod popupem oraz sterowanie obok numeru portu.

Pola wartości, dropdowny i przyciski w ustawieniach mają jednolite kolory bez
gradientu i promień rogów 4 px. Zapis pokazuje bursztynowe „Zapisywanie...”
i komunikat pod ekranem, a po potwierdzonym zapisie SD zielone „Zapisano”.
Potwierdzenie pozostaje do następnej lokalnej zmiany typu lub kolejnego zapisu;
błąd zamiast potwierdzenia wyświetla czerwone „Blad zapisu”.

Ekran odczytu nie pokazuje już typu i numeru portu nad wartością kafelka.
Opis portu jest zawijany czcionką Arial 12 do trzech wierszy, również dla
długich słów (z zachowaniem granic UTF-8). Temperatura i RH są obok siebie
czcionką Arial 20, a jednostki Arial 12. Pozostałe odczyty także używają
zwykłej czcionki. W ustawieniach białe pola mają czarne wartości Arial 18
bez pogrubienia. Napisy stanów przekaźników na niebieskich przyciskach są
białe bez pogrubienia na obu ekranach.

Układ pomiarów: pojedyncza temperatura Arial 28, wyśrodkowana; temp/RH
Arial 20, temperatura u góry po lewej, RH niżej po prawej. RH pokazujemy bez
części dziesiętnej (surowe dane HTTP nie są zmieniane). Jednostki na obu ekranach
używają Arial 12 i LCD_GetFontHalfHeight do wyrównania linii bazowej, zamiast
pełnej wysokości uwzględniającej dolne wydłużenia y/j. Przed C rysujemy puste
kółko stopnia 5x5 px, niezależne od znaków dostępnych w czcionce na SD.

Pięć czcionek panelu zajmuje identyfikatory fontID_13..fontID_17 (indeksy
12..16). Rozpoczęcie od fontID_14 przekraczało 17 dostępnych slotów przy
piątej czcionce i przerywało wejście na panel. Test pilnuje limitu identyfikatorów
i przechodzi dwa pełne cykle next/prev, w tym 0 → 12 → 11.

### Ekran 13 — LPC /nowa

Nowy panel jest następnym ekranem po /set w cyklu 0..13, zarówno next/prev,
jak i debug „=”. Białe tło, trzy duże szklane termometry oraz cztery przyciski
przekaźników portów 28..31. Temperatury i zakresy min1/max1 oraz nazwy
pochodzą ze wspólnego snapshotu właściciela HTTP; nie są przykładami demo.
Nazwy /nowa pobierane są z tych samych n_txtval001..003 i ustawień czujników.
Brak pomiaru pokazuje --- i rozjaśniony, szary termometr. Zakres niepoprawny
używa -10..50 C, tak samo jak panel.js. Przyciski działają tylko dla trybu
przekaźnik/IR, przez istniejący mailbox HTTP_LCDPanelToggle, z efektem dotyku.
Nie ma dodatkowego odpytywania HTTP ani zależności od otwartej przeglądarki.

Grafika jest generowana z niezmienionego web/svg/thermo-bottom.svg, wraz z
gradientami i refleksami. tools/generate_lcd_thermometers.py odtwarza geometrię
i rampę RGB z panel.js/jquery.thermometer.js, tworząc 101 poziomów 0..100%.
Kolor rampy: R=round(255*p), G=round(128*(1-p)), B=round(255*(1-p)).
Bitmapy RGB565 są spakowane blokami LZ4 z klatką kluczową co 10 poziomów;
mały lokalny dekoder nie dodaje biblioteki do firmware. Kwantyzacja grafiki
wynosi 1% zakresu, wartość liczbowa zachowuje 0.1 C. Nie trzeba zmieniać SD.
Generator wymaga tylko na komputerze cairosvg, lz4, Pillow i lxml.
Trzy bufory bitmap w SDRAM mają łącznie 188160 B, brak nowego framebufferu;
reszta rysowania używa istniejącego pLcd. Spakowane dane we FLASH: 339889 B.
