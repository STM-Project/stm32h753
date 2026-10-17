# Stany i polecenia przekaźników oraz PWM

`Application/relay_states.h` definiuje osobny globalny obiekt `RelayStatesData`.
Nie należy on do konfiguracji i nie jest zapisywany na kartę SD.

```c
#include "relay_states.h"

const RelayStates *stany = RELAY_GetStates();

if (stany->outputs[0].valid) {
    uint8_t pk1 = stany->outputs[0].on; /* 0=OFF, 1=ON */
}
```

Kolejność 13 wyjść odpowiada `HTTP_AllSettings.outputs[]`: porty 28–33
(PK1–PK6), 24–27 (OC), 21–23 (wyjścia obsługujące PWM). Pole `port` to numer
logiczny Sandwitch, nie numer pinu STM32.

## Polecenia z przycisków i suwaków HTTP

Żądania z obu stron (`/main` i `/set`) automatycznie aktualizują tę strukturę:

* `/mobile/pk001` przełącza żądany stan portu 28 (PK1). Kolejne numery 001–013
  odpowiadają kolejności `outputs[]` opisanej powyżej.
* `/lpc.cgi/n_pwm1=37&n_115_zapisz=` zapisuje 37% dla portu 21; `n_pwm2` oznacza
  port 22, a `n_pwm3` port 23. Akceptowany zakres wynosi 0–100%.

```c
const RelayState *pk1 = RELAY_GetState(28);
if (pk1->request_valid) {
    uint8_t zadanyStan = pk1->requested_on;
}

const RelayState *pwm1 = RELAY_GetState(21);
if (pwm1->pwm_valid) {
    uint8_t procent = pwm1->pwm_percent;
}
```

Pierwsze kliknięcie przy nieznanym stanie żąda ON. Gdy nie ma jeszcze polecenia,
ale sterownik zgłosił stan, pierwsze kliknięcie żąda jego przeciwieństwa.
PWM większy od zera ustawia też `requested_on=1`; PWM równy zero ustawia 0.
Przyciski działają tylko w trybie przekaźnika/IR, a suwak tylko w trybie PWM.
Błędne żądania zwracają HTTP 400 i nie zmieniają zmiennych. Kolejkowanie odpowiedzi
zapobiega wykonaniu polecenia, dla którego nie można przygotować odpowiedzi.
Odświeżanie `/TME.txt` i ponowne otwarcie strony używają zapamiętanych wartości.
Suwaki wysyłają polecenia kolejno dla danego kanału; podczas oczekiwania na
odpowiedź zachowują najnowszą wartość, żeby starsze żądanie nie nadpisało nowszego.

Polecenia są przechowywane w RAM do restartu/resetu struktury. Nie zapisują się
na SD. Pola `requested_on`, `request_valid`, `pwm_percent`, `pwm_valid` opisują
żądania, a `on` i `valid` pozostają stanem zgłoszonym przez sterownik. Sam GET
nie zmienia `on`/`valid` i nie steruje GPIO. Sterownik sprzętowy należy podłączyć
osobno; może odczytywać te polecenia i zgłaszać wykonany stan przez API poniżej.

## Stan zgłoszony przez sterownik

Na starcie wszystkie stany są nieznane (`valid=0`). W projekcie nie ma jeszcze
mapowania przekaźników na GPIO STM32H753. Po obsłużeniu wyjścia sterownik powinien
zgłosić jego stan:

```c
RELAY_UpdateState(28, 1);   /* zgłoszenie ON dla PK1, port logiczny 28 */
RELAY_UpdateState(28, 0);   /* zgłoszenie OFF dla PK1 */
RELAY_InvalidateState(28); /* stan nieznany */
```

Te funkcje rejestrują stan zgłoszony przez sterownik; nie sterują GPIO ani nie
potwierdzają fizycznego zwarcia styków przekaźnika. `valid` oznacza dostępność
zgłoszonego stanu. Pole `revision` zmienia się przy zmianach danych.
`RELAY_ResetStates()` oznacza wszystkie wyjścia jako nieznane, zachowując mapowanie.

Wskaźnik ma stały adres. Dane zmieniaj przez API, nie bezpośrednio przez globalny
obiekt. Używaj jednego zadania aktualizującego lub synchronizuj dostęp między
zadaniami. Błędny numer portu lub stan inny niż 0/1 zwraca 0 i nie zmienia danych.

`RELAY_InvalidateState()` unieważnia tylko zgłoszony stan, zachowując polecenie.
`RELAY_ResetStates()` czyści również żądania i zapamiętane wypełnienie PWM.
