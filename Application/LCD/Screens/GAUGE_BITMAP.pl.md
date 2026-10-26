# Bitmapa tła Gauge

Ekran 9 (duży Gauge i dwa małe) przechowuje gotowe tło wszystkich łuków
w SDRAM: 504x297 + 2x240x147 pikseli ARGB8888, razem 880992 B (860,3 KiB).
Przy każdym ruchu, również między zakresami COLD/WARM/HOT, tło jest
odtwarzane przez memcpy. Napisy mają gotowe obrócone bitmapy pokrycia AA
w SDRAM (180000 B); zmiana wyróżnienia wymaga tylko innego koloru przy
nałożeniu tych samych pikseli, bez obrotu i interpolacji. Ponownie obliczana jest tylko wskazówka wraz z cieniem.
Nie ma wtedy przeliczania atan2/sqrt dla wszystkich pikseli tarczy.

Bitmapy tarcz i obrócone napisy są przygotowywane przy wejściu na ekran.
Podczas przeciągania żadna zmiana koloru nie przebudowuje tarczy. Zachowane są dotychczasowe kolory,
cieniowanie, wygładzanie i progi wyboru. Bufor nie przechowuje wskazówki,
więc ruch nie zostawia śladów poprzedniej pozycji.

Zmieniony wskaźnik jest wysyłany bezpośrednio z pLcd przez
LCD_DisplayBuffStride: jeden prostokąt DMA2D zamiast 297 osobnych wierszy
dużego wskaźnika (147 dla małych). Nie ma dodatkowej pełnej kopii do transmisji.
Pozycja dotyku jest pobierana przed dekodowaniem zdarzenia, które może
wyzerować indeks próbek i w przeciwnym razie oddać starszy punkt.

Łącznie bitmapy zajmują 1060992 B (około 1,012 MiB) SDRAM.

Testy porównują 147 obrazów z oryginalnymi piksel po pikselu, sprawdzają
przeciąganie trzech wskazówek oraz DMA ze stride, wyrównanie cache i granice.
Liczby operacji w teście komputerowym nie są pomiarem czasu na STM32.
