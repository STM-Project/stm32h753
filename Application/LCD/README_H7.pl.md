# Pelny port LCD z Project_marafal (F7) do stm32h753 (H7)

Przeniesiono caly `Application/LCD` z Project_marafal, galaz NOWY,
commit `53766f9`, wraz z klawiatura, dotykiem STMPE811, grafikami,
czcionkami i wszystkimi ekranami, w tym 7, 8, 9 i 10.
Dodatkowo: Application/Lang, dwa zadania Application/Tasks,
examples.c/.h oraz cpu_utils.c/.h. Istniejace narzedzia H7 common/timer,
SD-card i debug pozostaly zachowane, bez zastapienia ich wersjami F7.

## Start i zadania

`Core/Src/freertos.c`, StartDefaultTask:

1. Istniejace InitAllMutex() tworzy mutexy SDRAM, fontow i SD (oraz H7 strBuff).
2. Po MX_FATFS_Init() oraz DEBUG_Init() wywolywane jest LCD_H7_Start().
3. Tworzone sa LcdUpdateSemaphoreId, osDma2dSemph i osDeviceMutex.
4. Inicjalizowane sa wylacznie bufory LCD, DMA2D i wykrywanie dotyku.
5. Create_TouchLcd_Task(): 2048 slow stosu, priorytet 1, cykl 20 ms.
6. Create_ScreensSelectLCD_Task(): 4024 slow stosu, priorytet 1, cykl 20 ms.

Zadania WiFi i logowania H7 nadal startuja swoim dotychczasowym kodem.
StartIdleMonitor/EndIdleMonitor w FreeRTOSConfig.h dostarczaja pomiar
obciazenia CPU uzywany przez ekrany. Kernel FreeRTOS nie jest modyfikowany.

Przelaczanie ekranu z zadania GUI:

```c
SCREEN_SetNr(10); // progi temperatury/wilgotnosci i lokalna edycja
SCREEN_SetNr(9);  // trzy wskazniki sterowane dotykiem
SCREEN_SetNr(8);  // ciemny panel temperatury
SCREEN_SetNr(7);  // niebieski panel temperatury
```

Poczatkowo ekran 0, tak jak w F7. Do fontow potrzebna jest karta SD z tymi
samymi plikami CFF/katalogami co w F7; fonty nie sa zaszyte w firmware.
BSP_TS_Init wykorzystuje STMPE811 na istniejacym I2C1. Gdy kontroler nie
odpowiada, wyswietlanie startuje, zadanie dotyku nie jest uruchamiane, jest log.
Mapowanie, zegary i piny I2C pozostaja konfiguracja H7; polaczenie fizyczne
panelu i kontrolera dotyku wymaga sprawdzenia na plytce.

## SDRAM — baza nadal 0x60000000

Nie zmieniono Core/Src/fmc.c, main.c (MPU/remap) ani konfiguracji LTDC.
LTDC pozostaje 800x480, RGB888, FBStartAdress=0x60000000.
Linker rezerwuje fizyczny bufor obrazu przed pozostalymi sekcjami:

| Obszar | Poczatek | Koniec (wylaczny) |
|---|---|---|
| .lcd_framebuffer, RGB888 | 0x60000000 | 0x60119400 |
| .lcd_work: pLcd, cache i grafika | 0x60119400 | 0x6058C000 |
| istniejaca .sdram H7 (HTTP itd.) | 0x6058C000 | 0x60F68C00 |

Mapa dotyczy aktualnej kompilacji FLASH. Koniec SDRAM: 0x61000000,
zapas 619520 bajtow. Linker sprawdza poczatek i rozmiar framebufferu.
Adresy buforow HTTP za sekcjami LCD sa ustalane przez linker — brak nakladania.
LCD_H7_Start zeruje tylko .lcd_work i .lcd_framebuffer, nigdy calego SDRAM,
ani cudzych danych/heapu. Zmiany sa w obu skryptach FLASH/RAM, pelny firmware
jest zweryfikowany w konfiguracji FLASH (kod nie miesci sie w 512 KB RAM_EXEC).

Pelne bufory F7 + bufory HTTP H7 przekraczaly 16 MiB o 4342784 bajty.
Dlatego w H7 zmniejszono POJEMNOSCI, zachowujac wszystkie funkcje i ekrany:

- cache czcionek/obrazow: 6 MiB -> 2 MiB;
- bufor ruchomych/obroconych napisow: 800x480 -> 800x240 pikseli ARGB8888;
- glowny pLcd pozostaje pelne 800x480 ARGB8888;
- nie zmniejszano buforow HTTP ani liczby wykresow.

Limity sa w LCD_fonts_images.c. Przy duzych zestawach fontow/obrazow lub
animacjach potrzebujacych calego bufora ruchomych napisow mozna osiagnac ten
limit szybciej niz w F7; nalezy kontrolowac wyniki funkcji ladowania fontow.

## Adapter DMA2D i synchronizacja

LCD_Hardware.c zachowuje API F7 LCD_DisplayBuff(), ale obsluguje H753:
wejscie ARGB8888, wyjscie RGB888 zgodne z dotychczasowym LTDC.
DMA2D jest skonfigurowane przez rejestry w kodzie aplikacji; nie dodawano
ani nie poprawiano plikow bibliotek STM32. Zegar wlaczany przez RCC,
DMA2D_IRQHandler sygnalizuje koniec/blad przez osDma2dSemph.
Priorytet IRQ 6 jest zgodny z limitem wywolan RTOS (5).
Cache zrodla jest czyszczony na granicach 32-bajtowych przed transferem.

W tym CMSIS-RTOS sukces osSemaphoreWait() to osOK=0, blad to dodatni osErrorOS.
Adapter poprawnie rozroznia te wartosci. Timeout DMA to 1000 ms; blad nie
pozostawia zablokowanych mutexow. LTDC_IRQHandler i LineEventCallback
obsluguja sygnal LcdUpdateSemaphoreId w przerwie pionowej. Dotychczasowe
API nie wymaga czekania na ten sygnal przed kazdym transferem (jak w F7).

`lcdH7Status` pozwala podejrzec initialized, touch_available, last_error,
transfers, dma_errors i timeouts. Brak zadania ekranu sygnalizowany jako -3,
brak pamieci na zadanie dotyku jako -4. Obsluga IRQ jest w adapterze aplikacji;
przy pozniejszym generowaniu DMA2D/IRQ przez Cube unikaj zdublowanych handlerow.

## Elementy zalezne od F7 poza LCD

Nie przeniesiono starej konfiguracji zegarow, pinow, LTDC, FMC ani sterownikow HAL.
Pominieto nieuzywany include mbedtls.h oraz wylaczono w kopii SCREEN_FontsLCD.c
stare klawisze debugowe do testowania WiFi/SMTP F7. Mozna je warunkowo wlaczyc
przez LCD_ENABLE_F7_NETWORK_DEBUG dopiero po podlaczeniu odpowiednich API H7.
Nie zmienia to ekranow/klawiatury i nie zastapuje kodu sieciowego H7.
Ekran demonstracyjny radia zachowano; pomocnicze PCF8575_Write uzywa juz
skonfigurowanego I2C4, PCF8575_Init nie przestawia pinow ani zegara tego busa.

## Budowa i testy

W STM32CubeIDE wykonaj odswiezenie projektu i Clean/Build konfiguracji FLASH.
Sciezki include dla LCD/Screens/Touch/SensorThresholds/Lang/Tasks dodano
w .cproject dla obu konfiguracji. Application pozostaje katalogiem zrodlowym.
Alternatywnie:

```sh
ARM_GCC=/sciezka/do/arm-none-eabi-gcc python3 tools/build_lcd.py
python3 tests/test_lcd_h7_port.py
python3 tests/lcd_temperature_panel_test.py
python3 tests/lcd_temperature_dashboard_test.py
python3 tests/lcd_temperature_gauges_test.py
python3 tests/lcd_sensor_thresholds_test.py
```

Pelna kompilacja/linkowanie: 125/125 zrodel, firmware FLASH.
Test hosta adaptera sprawdza rzeczywisty LCD_Hardware.c z modelowanymi
rejestrami i RTOS: format/adres, cache, osOK, IRQ, timeout, blad i granice.
Pozostale testy sprawdzaja logike ekranow i klawiatury z atrapami sprzetu.
Nie wykonano testu na fizycznym STM32H753: piny LCD/dotyku, czas renderowania,
transfer DMA, dzialanie cache i pliki fontow SD wymagaja sprawdzenia na plytce.


Wyjscie HEX wlaczono w .cproject dla Debug i Release (Convert to Intel Hex).
CubeIDE tworzy Debug/STM32h753bit6.hex lub Release/STM32h753bit6.hex.
Skrypt tools/build_lcd.py tworzy dodatkowo Debug/cloud/firmware.hex przez
arm-none-eabi-objcopy -O ihex. Pliki wynikowe nie sa przechowywane w Git.

## Czesciowe odswiezanie pod obciazeniem HTTP

`LCD_DisplayPart` przekazuje prostokat bezposrednio z `pLcd` do DMA2D
przez `LCD_DisplayBuffStride`, ze skokiem wiersza 800 pikseli. Jeden
prostokat to jeden transfer (zamiast osobnego DMA i oczekiwania dla kazdej
linii). FGOR pomija kolumny poza prostokatem; formaty nadal ARGB8888 -> RGB888.
Czyszczenie cache obejmuje caly zakres zrodlowy ze skokiem wiersza.

Przy artefaktach pod obciazeniem sprawdz w debuggerze
`lcdH7Status.ltdc_error_flags` i `ltdc_error_events`. Bit 0 oznacza blad
transferu LTDC, bit 1 niedobor danych FIFO. Flagi sa zapamietywane od startu,
bez logowania w przerwaniu. Poprawka zmniejsza liczbe transferow i opoznienia
planisty; nie zapewnia podwojnego buforowania ani synchronizacji z VBlank.
Plyta wymaga proby rownoczesnego odswiezania LCD i obciazenia WiFi/HTTP.
