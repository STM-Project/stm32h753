# Nawigacja ekranów LCD

Na każdym ekranie, również na startowym panelu czcionek, przez 5 sekund od
wejścia widoczne są strzałki `prev` i `next`. Leżą przy lewej i prawej krawędzi,
na środku wysokości: x=0..29 lub x=770..799, y=210..269 na LCD 800x480.
Po zniknięciu strzałek pola dotykowe nadal działają w tych samych miejscach.

Przewijanie następuje po puszczeniu palca w tym samym polu, w którym rozpoczęto
dotyk. Przytrzymanie nie powtarza zmiany ekranu, a wysunięcie palca poza pole
anuluje gest. Gest jest przechwytywany przed panelem, aby nie naciskać
jednocześnie pola ustawień lub klawisza. Dotyk rozpoczęty w środku panelu
pozostaje obsługiwany przez panel.

Strzałki i debug `=` używają `SCREEN_Change()` oraz wspólnej kolejności:

0. Panel czcionek (ekran startowy).
1. Przykład grafiki i napisów NOWY_1.
2. Obracane napisy NOWY_2.
3. Przewijane napisy NOWY_3.
4. Test okręgów.
5. Kalibracja LCD.
6. Parametry wizualne LCD.
7. Jasny panel temperatury / PK.
8. Panel temperatury z wykresem.
9. Trzy wskaźniki Gauge.
10. Ustawienia progów temperatury / wilgotności i nazw, wspólne z HTTP.

Po ostatnim ekranie `next` wraca do pierwszego, a `prev` działa odwrotnie.
Zmiana ekranu przechodzi przez istniejące funkcje EXIT, w tym zamykanie edytora
progów. Samo przewijanie nie zatwierdza niezapisanej edycji.

Nakładka nie korzysta z czcionek SD ani z roboczego pLcd. Ma oddzielne małe
bufory w SDRAM (łącznie 21600 B). Przed aktualizacją panelu odtwarza tylko dwa
fragmenty 30x60, a po aktualizacji pobiera ich aktualną zawartość z fizycznego
framebuffera RGB888 i ponownie rysuje strzałki. Dzięki temu po zniknięciu
nakładki nie zostaje stary fragment wykresu lub tekstu. Używa istniejącego
LCD_DisplayBuff oraz tej samej kolejności blokad pamięci i DMA2D. Nie zmienia
konfiguracji CubeMX ani sterowników LCD/dotyku.

Test `tests/test_lcd_navigation.py` wykonuje rzeczywisty kontroler i renderer
na modelu framebuffera RGB888, sprawdzając przewijanie, granice pól, zwolnienie
palca, anulowanie gestu, przepełnienie zegara, ukryte pola i dokładne
odtworzenie zawartości ekranu przy wygaśnięciu nakładki.
