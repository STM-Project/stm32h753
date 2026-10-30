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
10. Numer zarezerwowany dla progow; pomijany przez next/prev. Wejscie z konfiguracji portu w /set.
11. Panel odczytowy HTTP /main, przewijanie i wspólne PWM/przekaźniki.
12. Ustawienia portow HTTP /set, przewijanie, edycja nazw i formularze wartosci.

Po ostatnim ekranie `next` wraca do pierwszego, a `prev` działa odwrotnie.
Zmiana ekranu przechodzi przez istniejące funkcje EXIT, w tym zamykanie edytora
progów. Samo przewijanie nie zatwierdza niezapisanej edycji.

Nakładka nie korzysta z czcionek SD ani z roboczego pLcd. Ma oddzielne małe
bufory w SDRAM (łącznie 21600 B). Strzałki rysowane są raz przy wejściu na ekran.
Przez 5 sekund DMA2D pomija ich dwa prostokąty podczas aktualizacji panelu,
a nowe piksele spod nich trafiają do zapamiętanego tła. Nie ma cyklicznego
zdejmowania i ponownego rysowania nakładki. Po upływie czasu najnowsze tło
jest odtwarzane jeden raz. Źródłowy bufor panelu pozostaje niezmieniony.
Transfer z przesunięciem wierszy (stride) pozwala wysłać pozostałą część
obrazu maksymalnie trzema prostokątami, bez osobnego DMA dla każdego wiersza.
Nie zmienia konfiguracji CubeMX ani bibliotek HAL.

Test `tests/test_lcd_navigation.py` wykonuje rzeczywisty kontroler i renderer
na modelu framebuffera RGB888, sprawdzając przewijanie, granice pól, zwolnienie
palca, anulowanie gestu, przepełnienie zegara, ukryte pola i dokładne
odtworzenie zawartości ekranu przy wygaśnięciu nakładki.

13. Biały panel /nowa: trzy termometry z HTTP SVG i cztery przekaźniki 28..31.

W edytorach podrzednych strzalki sa schowane i nie przechwytuja klawiatury.
Powrot do /set odbywa sie przyciskiem Wroc. Kolejnosc pomija 10: 9 → 11.
