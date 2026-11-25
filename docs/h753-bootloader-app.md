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
W pliku `firmware.hex` adres jest zapisany w rekordach. `SystemInit` ustawia
`SCB->VTOR` na adres wektorów wybrany przez linker.

Wariant bez `--bootloader-app` służy do samodzielnego uruchamiania aplikacji
od `0x08000000` i zapisuje wyniki w `Debug/cloud`. Flaga wariantu
bootloadera dotyczy powyższego skryptu. Konfiguracje STM32CubeIDE domyślnie
budują aplikację samodzielną.
