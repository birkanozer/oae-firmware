# Progress: network-settings

Branch feature/network-settings off dev.

## Tasks
- T1 persistent settings: done. LCD Settings block (marker 0xA1 + one byte per row) at EEPROM 74-81, loaded in lcdUiSetup, stored on change; stored supply applied at boot; lat/lon already persisted by upstream.
- T2 network page + :XN protocol: done. Settings > Network page (mode, name, IP, clients/signal, Own WiFi click bar, No bridge after 5 s), Own WiFi stored with the other settings, strict :XNM/:XNI/:XNN/:XNG parser in core/meade/MeadeParserNetwork.cpp; :XNN keeps its spaces.
- T3 docs: done. wiring.md USB-C serial cable (byte-safe, 0xB5 kept), README safety + Settings/Network, CLAUDE.md decisions, architecture WiFi-only + :XN table, CHANGELOG [Unreleased], BACKLOG (only own hunks committed; a foreign uncommitted ALT note left in the tree).
- Rulings: layout without "Name:"/"Signal:" accepted (21 columns); :XNN space-keeping parser change accepted; :XFR must reload UI settings after clearing (fix in review round).
