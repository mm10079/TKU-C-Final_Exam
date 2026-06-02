# Minesweeper Game Core - Makefile
# 編譯 C 遊戲核心成動態庫 (DLL)

# ============================================
# 編譯器設定
# ============================================
CC := gcc
CFLAGS := -Wall -Wextra -O2
LDFLAGS := -shared

# 目標檔名與來源檔
TARGET := game.dll
SOURCE := game.c
HEADER := game.h

# ============================================
# 預設目標 (64-bit)
# ============================================
.PHONY: all
all: $(TARGET)

# ============================================
# 編譯目標 (64-bit)
# ============================================
$(TARGET): $(SOURCE) $(HEADER)
	$(CC) -m64 $(CFLAGS) $(LDFLAGS) -o $@ $(SOURCE)
	@echo "✓ Successfully built $(TARGET) (64-bit)"

# ============================================
# 32-bit 編譯目標
# ============================================
.PHONY: 32bit
32bit: $(SOURCE) $(HEADER)
	$(CC) -m32 $(CFLAGS) $(LDFLAGS) -o $(TARGET) $(SOURCE)
	@echo "✓ Successfully built $(TARGET) (32-bit)"

# ============================================
# 清理編譯產物
# ============================================
.PHONY: clean
clean:
	@if exist $(TARGET) del /Q $(TARGET)
	@echo "✓ Cleaned up"

# ============================================
# 幫助資訊
# ============================================
.PHONY: help
help:
	@echo "Minesweeper Game Core - Makefile"
	@echo "================================"
	@echo "Targets:"
	@echo "  make          - 編譯 64-bit DLL (預設)"
	@echo "  make 32bit    - 編譯 32-bit DLL"
	@echo "  make clean    - 清理編譯產物"
	@echo "  make help     - 顯示此訊息"
	@echo ""
	@echo "說明:"
	@echo "  確保 GCC 工具鏈已安裝 (MinGW 或其他)"
	@echo "  Python 版本與編譯 bitness 須一致"
	@echo "  例如: 64-bit Python 需要 64-bit DLL"
