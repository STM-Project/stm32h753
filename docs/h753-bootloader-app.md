# Kompilacja aplikacji STM32H753 pod bootloader

W katalogu głównym projektu uruchom poniższe polecenie z
`arm-none-eabi-gcc` i `arm-none-eabi-objcopy` w `PATH`.

```sh
python3 tools/build_lcd.py --bootloader-app
```

| Obszar | Adres początkowy | Rozmiar |
| --- | --- | --- |
| Rezerwa dla bootloadera | `0x08000000` | 128 KiB |
| Aplikacja H7 | `0x08020000` | 1920 KiB |

Plik `Debug/bootloader-app/firmware.bin` zaczyna się od tablicy wektorów
aplikacji. Przy zapisie surowego BIN ustaw adres docelowy na `0x08020000`.
W pliku `firmware.hex` adres jest zapisany w rekordach. `main()` ustawia `SCB->VTOR` na adres wektorów wybrany
przez linker, przed włączeniem cache i inicjalizacją HAL.

Wariant bez `--bootloader-app` służy do samodzielnego uruchamiania aplikacji
od `0x08000000` i zapisuje wyniki w `Debug/cloud`.

W STM32CubeIDE wybierz **Debug** lub **Release** i wykonaj **Clean Project**,
a następnie **Build Project**. Obie konfiguracje linkują aplikację od
`0x08020000` i generują HEX oraz BIN. Krok wstępny korzysta z systemowego
Windows PowerShell na Windows lub `python3` na Linuksie. Oba używają tego
samego lokalnego klucza aktualizacji co skrypt. Do pierwszego wgrania użyj ST-LINK z kasowaniem wyłącznie
sektorów aplikacji, aby zachować bootloader w pierwszym sektorze.

## Regeneracja w CubeMX

Regeneruj istniejący projekt, z włączonym `Keep User Code`. Poprawki SD,
TLS i ustawienia tablicy wektorów są w sekcjach `USER CODE`. Projekt używa
linkera `Application/STM32H753_application.ld`, poza ścieżką linkera
generowanego przez CubeMX. Wersjonowany plik `STM32H753BITX_FLASH.ld` jest odsyłaczem dla starszych
plików Makefile. Debug i Release wskazują właściwy linker bezpośrednio.
Jeśli CubeMX odtworzy standardowy plik o tej nazwie, odśwież projekt i
przebuduj go z aktualną konfiguracją CubeIDE.

Przed kompilacją sprawdź zmiany w `.cproject`, szczególnie po migracji lub
utworzeniu projektu CubeIDE od zera. Wersjonowane konfiguracje Debug i Release
wskazują linker aplikacji i krok wstępny `h753_update_key.mk`. Błąd linkera
dotyczący wektorów lub klucza oznacza, że należy przywrócić te ustawienia
z wersjonowanego `.cproject`.

## Aktualizacja z działającej aplikacji

Po zwykłym resecie nowy bootloader od razu uruchamia poprawną aplikację.
W aplikacji ustaw aktywny profil **STA** w `/wifi` i po połączeniu otwórz
`/upload` (alias `/ulpad`) pod adresem i portem interfejsu STA.
Podaj klucz z `Debug/bootloader-app/update-key` i zatwierdź. Od tej chwili
aplikacja uruchomi bootloader, który ponownie połączy ESP32 z tą samą siecią
i włączy stronę przesyłania na tym samym porcie HTTP. Profil z ręcznym IP
zachowuje ten adres. Przy DHCP, jeśli router przydzieli inny adres,
sprawdź dzierżawę dla urządzenia o nazwie `Elektronika_STM`.

Klucz jest generowany lokalnie podczas pierwszej kompilacji wariantu
`--bootloader-app`. Zachowaj plik do kolejnych kompilacji. Klucz oraz
plik BIN przechowuj prywatnie. Używaj aktualizacji w zaufanej sieci lokalnej,
ponieważ HTTP nie szyfruje klucza. Aplikacja i bootloader muszą pochodzić
z wersji obsługującej przekazanie profilu przez pamięć podtrzymywaną.
