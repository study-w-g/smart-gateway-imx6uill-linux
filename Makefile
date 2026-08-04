.PHONY: help check tree clean

help:
	@echo "Targets:"
	@echo "  make check  - check project skeleton and required documents"
	@echo "  make tree   - print project directories"
	@echo "  make clean  - remove local generated output"

check:
	@powershell -NoProfile -ExecutionPolicy Bypass -File scripts/check-project.ps1

tree:
	@powershell -NoProfile -Command "Get-ChildItem -Directory | Select-Object -ExpandProperty Name"

clean:
	@powershell -NoProfile -Command "if (Test-Path build) { Remove-Item -Recurse -Force build }; if (Test-Path out) { Remove-Item -Recurse -Force out }"
