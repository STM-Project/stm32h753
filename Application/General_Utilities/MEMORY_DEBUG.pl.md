# Raport pamięci pod znakiem |

Wyślij znak `|` na istniejącą konsolę debug UART. Każde wpisanie daje jeden
raport — nie ma timera ani automatycznego odświeżania. Komenda jest sprawdzana
przez globalny dispatcher debug w zadaniu WiFi, przed komendami Google/ESP.
Nie jest przypisana do konkretnego panelu LCD i działa również w Gauge,
edytorze progów i podczas połączenia Google. Nie zmienia konfiguracji USART.

Raport zawiera bieżące wolne miejsce sterty FreeRTOS, minimum wolnego miejsca
od startu, największy wolny blok oraz liczbę wolnych bloków. Różnica względem
configTOTAL_HEAP_SIZE obejmuje wszystkie alokacje i narzut sterty, nie tylko
stosy. Nie jest to sterta newlib/lwIP/mbedTLS ani wolne miejsce całego SDRAM.

Tabela obejmuje wszystkie aktywne wątki, w tym Idle, Timer i statyczny HTTPS:

- Limit B: zakres od pxStack do granicy pxEndOfStack zapisanej przez FreeRTOS.
  Kernel wyrównuje początkowy SP, więc limit może być o kilka bajtów mniejszy
  od zadeklarowanej alokacji stosu (na H7 zwykle o 4 B). Nie doliczamy bajtów
  spoza tej potwierdzonej granicy.
- Max zaj. B: limit minus minimalny wolny obszar stosu.
- Min wol. B: high-water mark przeliczony ze słów StackType_t na bajty.
- Max%: maksymalne historyczne zajęcie zakresu.
- Teraz%: bieżące zajęcie stosu według SP. Dla zadania wykonującego raport
  odczytywany jest PSP procesora; dla pozostałych używany jest SP zapisany
  przez FreeRTOS przy ostatnim przełączeniu (obejmuje zapisany kontekst CPU).
  Nie jest to procent użycia procesora. Przy każdym ręcznym wywołaniu może
  wzrosnąć lub zmaleć. Brak wiarygodnego zapisanego SP pokazuje n/d.
- Pamiec: fizyczne położenie stosu (SDRAM, RAM_D1, RAM_D2 albo DTCM).

Nie należy dodawać stosów statycznych do zajęcia sterty FreeRTOS. Są pokazane
w tabeli, ale przydzielono je poza ucHeap. Wartość stosu uzyskana z high-water
mark jest pomiarem obszaru nadpisanego od utworzenia wątku, nie gwarancją
bezpieczeństwa dowolnego przyszłego wywołania.

USER CODE w FreeRTOSConfig.h włącza zapis granicy i traceTASK_CREATE/DELETE.
Hooki utrzymują statyczny rejestr 64 wątków, bez alokacji i logowania w jądrze.
traceTASK_SWITCHED_OUT zachowuje również zapisany SP; istniejące hooki
pomiaru CPU pozostają aktywne. Nie ma odczytywania prywatnych struktur TCB
z kodu raportu ani skanowania stosów podczas przełączeń.
Biblioteki FreeRTOS są niezmienione. Jednorazowa migawka używa
uxTaskGetSystemState (skanowanie stosów chwilowo zawiesza scheduler).
Nazwy i pomiary są kopiowane przed wznowieniem zadań. Całe formatowanie
i wysyłanie odbywa się po wznowieniu, porcjami poniżej 960 bajtów przez
istniejącą kolejkę debug. Długie nazwy mają limit configMAX_TASK_NAME_LEN.
Przekroczenie limitu listy zgłaszane jest w raporcie; brak wpisu rozmiaru
nie jest zastępowany zgadywaną wartością.

Test tests/test_memory_debug.py sprawdza rzeczywiste makra trace i kod raportu,
konwersję słów na bajty, wartości historyczne, ponowne użycie uchwytów,
bezpieczeństwo nazw po wznowieniu schedulera i podział tabeli 64 wątków.
