# Stany przekaźników i wyjść

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
logiczny Sandwitch, nie numer pinu STM32. Struktura nie zawiera wypełnienia PWM.

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
