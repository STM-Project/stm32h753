# STM32H753 + ESP32-AT → Arkusz Google

Obsługa jest adaptacją protokołu z `STM-Project/Project_marafal`, commit
`d571681d3b86cda6de2ce5b2840cc745851301e5`. Zachowuje Apps Script, temperaturę
w dziesiątych stopnia, `relayAck`, losowy `nonce`, format odpowiedzi
`STM32CLOUD1:<nonce>:<0|1>` oraz demonstracyjny stan przekaźnika w RAM.
Tutaj wszystkie operacje sieciowe wykonuje istniejące `vtaskWifi()` przez
`CASE_Service()` i `SendToEsp32()`. Nie ma dodatkowego zadania ani LwIP/mbedTLS
na STM32; TLS obsługuje ESP32.

## Użycie istniejącego arkusza Project_marafal

1. Zachowaj dotychczasowe wdrożenie Apps Script i prywatny arkusz. Protokół
   jest zgodny; nie trzeba tworzyć nowego konta ani zmieniać skryptu.
2. Skopiuj `Application/GOOGLE/google_cloud_config.local.h.example` do
   `Application/GOOGLE/google_cloud_config.local.h`.
3. Wpisz tam `GOOGLE_CLOUD_ENABLED 1`, identyfikator `DEVICE_ID` z dotychczasowego
   skryptu, jego `DEVICE_TOKEN` i ścieżkę `/macros/s/.../exec` z adresu wdrożenia.
   Istniejący projekt może używać `stm32f769-demo`: wpisz dokładnie ten sam
   identyfikator, mimo że fizycznym urządzeniem jest teraz H753.
4. Ustawienia Wi-Fi, DNS i SNTP pozostają w dotychczasowej konfiguracji INIT.
   Potrzebny jest tryb STA lub AP+STA i dostęp do internetu.
5. Skompiluj i wgraj firmware. Co 60 sekund klient próbuje wysłać pomiar;
   po poprzedniej próbie nie nadrabia zaległych transmisji. Jeśli WWW właśnie
   wysyła stronę, klient poczeka na zwolnienie kolejki i krótką przerwę.
6. Na konsoli diagnostycznej wyślij tekst `cloud`: pokazuje konfigurację,
   gotowość TLS, ostatni kod błędu, liczniki, temperaturę i stan DEMO.

Tokenu nie wklejaj do czatu, adresu URL ani publicznego repozytorium.
Lokalny plik jest ignorowany przez Git. Domyślna konfiguracja repozytorium
ma klienta wyłączonego i nie zawiera danych dostępu z Project_marafal.
Nie używa hasła Gmail ani klucza płatnej usługi Google Cloud.

## Nowy arkusz, jeśli dotychczasowego nie ma

1. Utwórz Arkusz Google na Dysku Google.
2. Otwórz **Rozszerzenia → Apps Script**, wklej
   `cloud/google-apps-script/Code.gs` i uruchom funkcję `setup`.
3. Po autoryzacji powstaną zakładki `Sterowanie` i `Pomiary`.
   W ustawieniach projektu Apps Script odczytaj właściwości `DEVICE_ID`
   i `DEVICE_TOKEN`; domyślny identyfikator nowego skryptu to `stm32h753`.
4. Wdróż jako **Aplikacja internetowa**, **Uruchamiaj jako: Ja**,
   **Dostęp: Każdy**. Arkusz nadal może pozostać prywatny.
5. Ścieżkę z adresu wdrożenia kończącą się `/exec` oraz obie właściwości wpisz
   do lokalnego nagłówka. Nie używaj testowego adresu `/dev`.

## Co wykonuje INIT

Dotychczasowe kroki `INIT_CONNECTION` nadal ustawiają UART, tryb Wi-Fi,
połączenie STA, DHCP/adresację, DNS, SNTP i serwer WWW. Po poprawnym odczycie
czasu dochodzą kroki 22–27:

- `AT+SYSMFG=2,"client_ca","client_ca.1",8,<rozmiar>` zapisuje publiczne
  certyfikaty Google Trust Services R1–R4. Dane są dzielone według rozmiaru
  `sendBuff` i wysyłane dopiero po zakończeniu poprzedniego UART DMA.
- `AT+CIPSSLCCONF=3,2,0,1` wymaga uwierzytelnienia serwera przez CA.
- `AT+CIPRECVMODE=0` i `AT+CIPDINFO=0` ustawiają format ramek zgodny z parserem.

Indeks CA można zmienić przez `GOOGLE_CLOUD_CA_INDEX`. Domyślny wpis
`client_ca.1` jest przeznaczony dla tej funkcji i będzie zastąpiony podczas INIT.
`GOOGLE_CLOUD_INSTALL_CA=0` pomija zapis, jeśli certyfikaty już wgrano do ESP.
Plik PEM znajduje się też w `cloud/google-apps-script/google_gts_roots.pem`.
ESP-AT musi obsługiwać SYSMFG, SSLCCONF, SSLCCN i SSLCSNI. Nieobsługiwane
polecenie kończy próbę błędem; kod nie przechodzi na połączenie bez weryfikacji.
Przy zablokowanym przesyłaniu surowych danych CA moduł jest resetowany, a klient
Google zostaje wyłączony do kolejnego startu zadania Wi-Fi/sterownika, żeby
umożliwić działanie WWW.

## Transmisja w vtaskWifi()

`GOOGLE_CLOUD_CONNECTION` jest kolejną gałęzią istniejącej maszyny stanów.
Używa wewnętrznego kanału ESP `3`; dotychczasowy SMTP zachowuje kanał `4`.
Przed próbą klient czeka na pustą kolejkę HTTP i 250 ms przerwy. Podobnie jak
istniejąca wysyłka e-maila, na czas wymiany HTTPS wyłącza serwer poleceniem
`CIPSERVER=0`. Po sukcesie lub błędzie zamyka własny kanał i wznawia serwer
na dotychczasowym porcie. W tym czasie lokalne WWW może na krótko czekać
lub wymagać ponowienia żądania. Jest tylko jeden właściciel UART/AT.

Dla każdego połączenia ustawiane są nazwa oczekiwanego serwera i SNI.
Pierwszy POST idzie do `script.google.com`. Przekierowanie 302/303 jest
akceptowane wyłącznie do HTTPS na `script.googleusercontent.com` i realizowane
jako GET **bez ponownego przesyłania tokena**. Długa ścieżka może być wysłana
w kilku CIPSEND. Wszystkie dane wychodzą przez `SendToEsp32()`.

Parser oddziela treść `+IPD` od odpowiedzi AT: tekst `OK`, `ERROR` lub `>`
wewnątrz HTTP nie zmienia stanu polecenia. Obsługuje fragmentację, odpowiedzi
Content-Length, chunked i zakończone zamknięciem połączenia. Limity buforów,
niezgodny nonce i niepoprawna odpowiedź powodują błąd zamiast zmiany DEMO.
Próba ma limit 25 s, a zamknięcie i przywrócenie serwera po 5 s; brak powrotu
serwera powoduje ponowne uruchomienie ESP i przejście przez dotychczasowy INIT.

## Pomiary i stan

- `GoogleCloud_ReadTemperature10()` domyślnie czyta temperaturę portu 1
  z `HttpRefr_sdram`, tak jak lokalny panel. Numer zmienia makro
  `GOOGLE_CLOUD_TEMPERATURE_PORT` (1–20). Funkcję można zastąpić własną.
- Brak danych lub wartość poza zakresem -80…150°C pomija transmisję.
  Jeśli program wpisuje do bufora dane testowe, to te dane trafią do arkusza.
- `googleCloudStatus.demo_relay` to stan checkboxa z arkusza, tylko w RAM.
  Tak jak w Project_marafal, nie przełącza fizycznego GPIO ani `RelayStatesData`.
  Kolejna transmisja raportuje go jako `relayAck`.
- Nonce powstaje ze sprzętowego RNG H753; zegar HSI48/RNG jest uruchamiany
  przy pierwszej próbie. Brak losowych danych kończy próbę błędem.
- `googleCloudStatus` zawiera sukcesy, błędy, ostatnią temperaturę i gotowość.
  Kod błędu: `-1` konfiguracja, `-2` czas, `-3` INIT/TLS, `-4` timeout,
  `-5` odpowiedź AT, `-6` HTTP, `-7` odpowiedź/nonce, `-8` RNG,
  `-9` powrót serwera WWW, `-18` pomiar.

Token i treść POST nie są wypisywane do logu ani zapisywane jako ostatnie
polecenie diagnostyczne. Inne komendy debug nie przerywają wymiany Google.
Duże bufory odpowiedzi/żądań są statyczne; nie trafiają na stos zadania.
Konfiguracja Google nie wymaga plików na SD i nie zmienia stron LPC.

## Sprawdzanie bez sprzętu

```sh
bash tests/run_google_cloud_tests.sh
```

Testy obejmują rzeczywiste gałęzie INIT/Google z `esp32wroom.c`, pakiety AT,
DMA-ready dla CA, wielopakietowy redirect, utratę odpowiedzi i powrót do WWW,
parser HTTP oraz logikę Apps Script. Wymagają GCC, Pythona 3 i Node.js.
Nie zastępują próby na fizycznym ESP32 z konkretnym firmware AT i własnym
wdrożeniem Google. Do testów nie są potrzebne prywatne tokeny ani konto.

## Graficzny panel temperatury i przekaźnika

Kod STM32 i jego konfiguracja pozostają bez zmian. Arkusz służy jako prywatny
magazyn danych; na adresie `/exec` zamiast tekstu otwiera się panel graficzny.

1. W istniejącym projekcie Apps Script zastąp zawartość `Code.gs` aktualnym
   plikiem `cloud/google-apps-script/Code.gs`.
2. Kliknij **+ → HTML**, nadaj nazwę **Panel** i wklej zawartość
   `cloud/google-apps-script/Panel.html`. Cały CSS i JavaScript są w tym pliku.
3. Uruchom ponownie `setup`. Zachowa token urządzenia, jego identyfikator,
   arkusze i dane. Doda osobny losowy `PANEL_KEY`.
4. W **Ustawienia projektu → Właściwości skryptu** skopiuj `PANEL_KEY`.
   To klucz panelu, inny niż `DEVICE_TOKEN`; traktuj go jak hasło.
5. **Wdróż → Zarządzaj wdrożeniami → edytuj istniejące wdrożenie → Nowa wersja
   → Wdróż**. Zachowaj wykonywanie jako właściciel i dostęp „Każdy”.
   Aktualizuj istniejące wdrożenie, aby adres używany przez STM32 się nie zmienił.
6. Otwórz dotychczasowy adres `/exec` w przeglądarce i wpisz `PANEL_KEY`.

Panel pokazuje termometr (niebieski przy niskiej temperaturze, czerwony przy
wysokiej), ostatni kontakt, ostatnie 120 pomiarów oraz stan przekaźnika DEMO.
Przyciski ON/OFF zapisują to samo pole B2 co dotychczasowy checkbox arkusza.
STM32 odbierze polecenie przy kolejnej wysyłce; potwierdzenie zobaczysz przy
następnym przesłanym `relayAck`. Nie jest to sterowanie fizycznym GPIO.
Panel odpytuje dane co 15 sekund; nowe pomiary nadal przychodzą w rytmie STM32.
Po 3 minutach bez kontaktu panel sygnalizuje brak świeżych danych.
Klucz jest przechowywany tylko w pamięci otwartej karty, bez adresu URL,
localStorage i bez ujawniania tokenu STM32. Historia i sterowanie wymagają klucza.
