# Dwa ekrany HTTP na LCD 800x480

11 — panel odczytowy `/main`: trzy kolumny pomniejszonych kafelków, czarne tło,
szare obramowanie/tarcza, jasne wartości, zielone nazwy i niebieskie przyciski
przekaźników. Aktywne porty i ich kolejność pochodzą z tego samego zestawu
trybów co HTTP: czujniki, liczniki, wejścia, EOL, ADC/PT1000, PWM, przekaźniki/IR,
timery i termostaty. Wyłączone porty są pomijane. Liczniki mają wyższe kafelki.

12 — ustawienia `/set`: wszystkie 33 porty, nazwa, aktualny tryb i przycisk
Ustawienia. Przycisk zmienia wygląd przy wciśnięciu i wraca po puszczeniu.
Nie uruchamia dalszego ekranu i nie zapisuje ustawień. Przeciągnięcie pionowe
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
