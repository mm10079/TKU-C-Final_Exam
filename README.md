# 期末專題報告：踩地雷 (Flask + C)

> 1142學期 計算機程式語言期末專題報告：以 Python Web 前端搭配 C 語言遊戲核心，實現一個踩地雷桌面遊戲。

## 專題簡介
本專案是一個踩地雷遊戲，採用 Flask Web 應用作為前端界面，遊戲核心由 C 語言實作，再透過 Python ctypes 進行呼叫。

## 主要功能
- 可自訂行列、地雷數的踩地雷遊戲
- 左鍵揭開格子，右鍵插旗 / 取消旗
- 自動 flood-fill 展開空白格子
- 勝利 / 失敗判斷
- Undo 功能
- 成績儲存與排行榜顯示（SQLite + SQLModel）

## 專案架構
- `src/app/main.py`：Flask 應用，提供遊戲 API 與 HTML 前端
- `src/app/game_ffi.py`：ctypes 介面，載入 `game.dll` 並呼叫 C 遊戲函式
- `src/app/core/game.c`：C 語言遊戲核心，實作盤面、點擊、flood-fill、undo 與狀態輸出
- `src/app/core/game.h`：C 函式庫介面定義
- `src/app/database/score.py`：SQLModel 資料庫模型與操作
- `src/app/templates/index.html`：遊戲前端頁面
- `db/schema.sql`：資料庫結構備份

## 系統需求
- Python 3.8 以上
- Windows 平台支援 `game.dll`
- `gcc` 或其他 C 編譯器（若要從原始碼編譯 DLL）

## 安裝與執行
### 安裝 Python 依賴
```sh
pip install -r requirement.txt
```

### 執行遊戲
```sh
python -m src.main
```

### 重新編譯 C 動態連結庫
若 `src/app/core/game.dll` 不存在或修改了 `src/app/core/game.c`，請進入 `src/app/core` 後執行：
```sh
gcc -shared -o game.dll game.c
```

## 使用說明
1. 開啟瀏覽器並前往 `http://127.0.0.1:5000/`
2. 選擇難度或自訂盤面大小與地雷數
3. 左鍵揭開格子，右鍵插旗
4. 遊戲結束後可提交成績並查看排行榜

## 注意事項
- 若 Python 程式出現閃退，請先確認 `src/app/core/game.dll` 是否正確存在且對應您的平台
- 成績資料庫預設路徑為 `src/app/database/game.sqlite`

## 擴充建議
- 加入遊戲計時器與最佳分數統計
- 將前端改為桌面應用程式（例如 PyQt）
- 加入難度選單與成績排序篩選
