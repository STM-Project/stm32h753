# HTTP i HTTPS Ethernet: port F7 → H7

Źródło funkcji i bibliotek: `STM-Project/Project_marafal`, gałąź NOWY,
commit `53766f9e77145f2f3e69aba3cc382dfcea03077a` (ten sam projekt F7, z którego
pochodzi LCD). Przeniesiono lwIP 2.1.2, mbedTLS 2.16.2 i sterownik LAN8742.
Pamięć sieci odpowiada załączonemu `FP70/FP70.ioc`. PHY pozostaje **LAN8742**,
zgodnie z potwierdzeniem użytkownika; nie przeniesiono LAN8720A z FP70.

## Użycie

- Ethernet RMII, ten sam pinout co dotychczasowa konfiguracja ETH H7.
- IP `192.168.1.40`, maska `255.255.255.0`, brama `192.168.1.1`, DHCP wyłączony.
- DNS `8.8.8.8`, zapasowy `8.8.4.4`.
- `http://192.168.1.40/` i `https://192.168.1.40/`: plik `0:/aaa.htm` z SD.
- `/aaa.htm` jest aliasem `/`, `/test` pokazuje test połączenia (dla HTTPS
  także wynegocjowany szyfr). Brak pliku/karty daje odpowiedź HTTP 503.
- HTTPS używa **tego samego certyfikatu testowego EC mbedTLS co F7**. Przeglądarka
  nie uzna go za zaufany certyfikat urządzenia. Własny certyfikat i klucz można
  podłączyć w `MBEDTLS/App/mbedtls.c`.

Jest to port Ethernetowych endpointów F7. Obecne strony ustawień HTTP przez
ESP32 pozostają obsługiwane przez istniejący kod; nie przeniesiono ich pod nowe
endpointy Ethernet. SMTP/Google i sterownik ESP32 pozostają bez zmian.

## CubeMX i pamięć

`STM32h753bit6.ioc` zawiera LWIP, MBEDTLS, LAN8742, adresację F7, parametry
puli/buforów i przerwanie ETH (priorytet 5, zgodny z istniejącym FreeRTOS).
`.cproject` zawiera źródła i nagłówki dla Debug i Release, w tym
`MBEDTLS_CONFIG_FILE=<mbedtls_config.h>`.

Z FP70 przeniesiono **wyłącznie pamięć sieci**:

| Obszar | Adres | Właściwości |
| --- | --- | --- |
| MPU sieci (region 5 H7) | `0x30040000`, 32 KiB | normalna, bez cache |
| MPU deskryptorów (region 6) | `0x30040000`, 256 B | device, shared, bufferable |
| Deskryptory RX | `0x30040000` | 4 × 24 B w obecnym HAL |
| Deskryptory TX | `0x30040060` | jak linker FP70 |
| Sterta lwIP | `0x30044000`, 16 KiB | jak FP70, 16360 B + nagłówki alokatora |
| Pula RX zero-copy | `0x30010000` | 12 oddzielonych buforów, wyrównanie 32 B |

FP70 zawiera starą sekcję `.RxArraySection` przy `0x30040200`, ale jego aktualna
implementacja zero-copy korzysta z `RX_POOL`. Dwanaście takich buforów nie mieści
się między `0x30040200` a stertą `0x30044000`; dlatego pula otrzymała osobną
sekcję w SRAM1/2. DMA ma do niej dostęp. Przed odbiorem i po nim unieważniane są
wyłącznie linie cache należące do bufora. Przed TX czyszczony jest cache payloadu.
Pozostałe bufory DMA zachowują adresy od `0x30000000`.

Nie zmieniono USART6, DMA USART6, ESP32, priorytetów SDMMC/MDMA/DMA2D, ustawień
SDRAM/LCD ani progu przerwań FreeRTOS. Linker i asercje C wykrywają przekroczenie
obszarów i utratę wyrównania.

CubeMX nie generuje całej adaptacji H7 (linkera i kodu aplikacji). Po ponownym
wygenerowaniu należy uruchomić `python3 -B tests/test_network_configuration.py`
oraz kompilację. Test sprawdza też, czy nie powstały duplikaty uchwytów ETH
między `Core/Src/eth.c` a `LWIP/Target/ethernetif.c`.

## Różnice niezbędne dla H7

- Jedna definicja `heth`, deskryptorów i konfiguracji TX w `Core/Src/eth.c`.
- Bufory NOLOAD są inicjalizowane przed uruchomieniem DMA.
- HTTP/HTTPS mają wspólną obsługę żądań; pliki są wysyłane w porcjach 2048 B.
  Nie ma odwołań do `GETVAL_ptr()` ani buforów LCD. Blokada SD nie jest trzymana
  podczas komunikacji sieciowej. Krótkie odczyty/zapisy są obsługiwane.
- Init sieci ma osobny stos 8 KiB, HTTP 8 KiB, HTTPS 32 KiB. Nie zmieniono
  stosów istniejących zadań. Brak PHY lub zegara RMII nie zatrzymuje LCD/Wi-Fi.
- Konfiguracja netif i zmiany linku są chronione blokadą rdzenia TCP/IP.
- Adapter RNG kopiuje pełne słowa losowe i końcówki, zgłasza błędy HAL oraz
  chroni RNG mutexem. Adapter F7 używał `memset`, powielając jeden bajt.

## Weryfikacja

- `python3 -B tests/test_network_http.py`: fragmentacja żądań, częściowe zapisy,
  plik 200 kB, zwalnianie blokady SD, błędy i zamykanie plików.
- `python3 -B tests/test_network_rng.py`: bajty entropii, końcówki i błędy HAL.
- `python3 -B tests/test_network_configuration.py`: spójność CubeMX/MPU/linkera.
- Pełna kompilacja ARM obejmuje lwIP, TLS, adaptery i serwery.

Bez płytki nie można potwierdzić linku PHY, przebiegów RMII, zachowania DMA/cache
ani połączenia przeglądarki z fizycznym urządzeniem. Po wgraniu sprawdzić log ETH,
obie strony `/test`, plik z SD, odłączenie/podłączenie kabla oraz równoczesne LCD
 i Wi-Fi. Brak obsługi HTTPS w przeglądarce z powodu certyfikatu należy odróżnić
od błędu negocjacji TLS w logu.
