# network-settings: implementation report (saved by the controller from the hand-back)

Commits: 78ca840 T1 persistent LCD settings (EEPROM 74-81, marker 0xA1, values then marker, write on
change, stored supply applied at boot); a3e33f7 T2 Settings > Network page + :XNM/:XNI/:XNN/:XNG
(MeadeParserNetwork.cpp, bounded, 5 s No bridge, Own WiFi Auto/On/Off persisted); d3646a9 T3 docs
(USB-C serial cable, README safety, CLAUDE.md decisions, architecture, CHANGELOG, BACKLOG).

Gate: 523/523 native, robin_e3 SUCCESS (RAM 16.8 %, flash 62.0 %).

Deviations: no "Name:"/"Signal:" labels (21 columns); MeadeParser keeps spaces after ":XNN".
Concerns: :XFR clears the store without reloading the LCD copy; STORE_SIZE 82 on every board; flash
fallback stalls a few ms per settings write; clang-format not machine-checked.
Needs hardware: power-cycle persistence, blank store, :XN over USART2, the cable.
