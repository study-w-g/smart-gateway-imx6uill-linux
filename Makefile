.PHONY: help check tree clean

help:
	@echo "可用目标："
	@echo "  make check  - 检查项目目录和必需文档"
	@echo "  make tree   - 显示项目目录"
	@echo "  make clean  - 删除本地生成的输出文件"

check:
	@powershell -NoProfile -ExecutionPolicy Bypass -File scripts/check-project.ps1

tree:
	@powershell -NoProfile -Command "Get-ChildItem -Directory | Select-Object -ExpandProperty Name"

clean:
	@powershell -NoProfile -Command "if (Test-Path build) { Remove-Item -Recurse -Force build }; if (Test-Path out) { Remove-Item -Recurse -Force out }"
