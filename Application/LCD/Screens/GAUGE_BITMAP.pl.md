# Bitmapa tła Gauge

Ekran 9 (duży Gauge i dwa małe) przechowuje gotowe tło łuków i napisów
w SDRAM: 504x297 + 2x240x147 pikseli ARGB8888, razem 880992 B (860,3 KiB).
Przy ruchu w ramach tego samego zakresu COLD/WARM/HOT tło jest odtwarzane
przez memcpy. Ponownie rysowana jest tylko wskazówka wraz z cieniem.
Nie ma wtedy przeliczania atan2/sqrt dla wszystkich pikseli tarczy.

Bitmapa danego wskaźnika jest przebudowywana przy wejściu na ekran i przy
zmianie wyróżnionego napisu COLD/WARM/HOT. Zachowane są dotychczasowe kolory,
cieniowanie, wygładzanie i progi wyboru. Bufor nie przechowuje wskazówki,
więc ruch nie zostawia śladów poprzedniej pozycji.

Zmieniony wskaźnik jest wysyłany bezpośrednio z pLcd przez
LCD_DisplayBuffStride: jeden prostokąt DMA2D zamiast 297 osobnych wierszy
dużego wskaźnika (147 dla małych). Nie ma dodatkowej pełnej kopii do transmisji.
Pozycja dotyku jest pobierana przed dekodowaniem zdarzenia, które może
wyzerować indeks próbek i w przeciwnym razie oddać starszy punkt.

Testy porównują 123 obrazy z oryginalnymi piksel po pikselu, sprawdzają
przeciąganie trzech wskazówek oraz DMA ze stride, wyrównanie cache i granice.
Liczby operacji w teście komputerowym nie są pomiarem czasu na STM32.
