.PHONY: prepare-key

prepare-key:
ifeq ($(OS),Windows_NT)
	powershell.exe -NoProfile -NonInteractive -Command - < ../tools/h753_update_key.ps1
else
	python3 ../tools/h753_update_key.py h753_update_key.h
endif
