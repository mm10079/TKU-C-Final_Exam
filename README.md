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

## 使用技術

| 層級 | 技術 | 核心能力展示 |
|------|------|-----------|
| 前端 UI | HTML + CSS + JS | 動態介面渲染、事件處理、非同步 API 呼叫 |
| Web 後端 | Flask + Python | REST API 設計、路由與請求處理、資源生命週期管理 |
| 語言整合 | ctypes | C-Python 跨語言互操作、DLL 動態載入、內存指標操作 |
| 遊戲核心 | C 語言 | 遊戲邏輯實作、狀態管理、性能優化 |
| 演算法 | Flood-fill DFS、Fisher-Yates 洗牌 | 自動展開空白區域、高效隨機地雷配置 |
| 資料結構 | 平面陣列、Struct、Enum | 記憶體連續性、快取友善、類型安全 |
| 資料庫 | SQLite + SQLModel | 持久化儲存、ORM 資料模型、排行榜查詢 |

## 演算法優化

### Fisher-Yates 洗牌演算法
用於地雷隨機配置，時間複雜度 **O(bomb_count)**，相比原始碰撞法大幅提升性能。

**運作原理**：
1. 先將前 `bomb_count` 個位置標記為地雷
2. 在 `[i, total)` 範圍內隨機交換格子 `i` 與格子 `j`
3. 最終地雷均勻分佈，無重複碰撞

**效能對比**（10000 格盤面）：

| 地雷密度 | 原始碰撞法 | Fisher-Yates | 優化倍數 |
|---------|----------|-------------|--------|
| 10%（1000） | ~1000 次 | 1000 次 | 1× |
| 50%（5000） | ~15000 次 | 5000 次 | **3×** |
| 90%（9000） | ~90000 次 | 9000 次 | **10×** |

### Flood-fill DFS 自動展開
使用堆疊（stack）與訪問標記（visited），對於空白地板（`adj == 0`）進行深度優先搜索，自動展開相鄰格子直至遇到數值格為止。時間複雜度 O(rows × cols)。

## API說明

### C ↔ Python (ctypes 介面)

| 函式 | 參數 | 回傳 | 功能 |
|------|------|------|------|
| `game_init` | `rows, cols, bomb_count` | `GameHandle*` | 初始化遊戲盤面，分配記憶體並隨機放置地雷 |
| `game_left_click` | `handle, r, c` | `int` | 左鍵揭開格子；回傳 0（安全）、1（炸彈）、-1（錯誤） |
| `game_right_click` | `handle, r, c` | `int` | 右鍵切換旗幟狀態 |
| `game_undo` | `handle` | `int` | 復原上一次操作，恢復遊戲狀態 |
| `game_get_state` | `handle, buffer, bufsize` | `int` | 將遊戲狀態序列化成 JSON 字串，回傳位元組數 |
| `game_free` | `handle` | `void` | 釋放所有動態分配的記憶體（tiles、history） |

### HTTP REST API

| Method | Path | Request Body | Response | 功能 |
|--------|------|--------------|----------|------|
| `POST` | `/api/new_game` | `{"rows": 15, "cols": 15, "bombs": 40, "difficulty": "easy"}` | `{"status": "ok", "rows": 15, "cols": 15, "bombs": 40}` | 初始化新遊戲 |
| `GET` | `/api/get_settings` | 無 | `[{"name": "easy", "rows": 9, ...}, ...]` | 取得難度設定清單 |
| `GET` | `/api/state` | 無 | `{"rows": 15, "cols": 15, "bomb_count": 40, "tiles": [...]}` | 獲取當前遊戲狀態（JSON） |
| `POST` | `/api/click` | `{"row": 5, "col": 3, "kind": "left"}` | `{"result": 0}` | 左鍵/右鍵點擊（`kind`: left/right） |
| `POST` | `/api/undo` | 無 | `{"result": 0}` | 撤回上一步操作 |
| `POST` | `/api/submit_score` | `{"name": "玩家名稱", "score": 120}` | `{"status": "ok", "difficulty": "easy", "time": "2026-06-03 12:34:56"}` | 提交遊戲分數到排行榜 |
| `GET` | `/api/get_leaderboard` | 無 | `[{"name": "玩家", "score": 120, "difficulty": "easy", "time": "..."}, ...]` | 取得全部排行榜紀錄 |
| `POST` | `/api/free_game` | 無 | `{"status": "ok"}` | 釋放遊戲資源（通常在退出時呼叫） |

## 專案架構

```
期末報告/
├── app/
│   ├── main.py                    # Flask 主應用，提供遊戲 API 與路由
│   ├── game_ffi.py               # ctypes 介面，動態載入 game.dll 並包裝 C 函式
│   ├── config.json               # 難度設定（簡單、中等、困難、自訂）
│   ├── __init__.py
│   ├── core/
│   │   ├── game.c               # C 語言遊戲核心邏輯
│   │   ├── game.h               # C 函式宣告與結構定義
│   │   ├── game.dll             # 已編譯的 C 動態庫（Windows）
│   │   ├── libgame.so           # 已編譯的 C 動態庫（Linux）
│   │   └── Makefile             # C 編譯腳本（32-bit/64-bit）
│   ├── database/
│   │   ├── score.py             # SQLModel ORM 模型與資料庫操作
│   │   └── game.sqlite          # SQLite 資料庫（排行榜）
│   ├── models/
│   │   └── schema.py            # 資料結構定義（Config、難度參數）
│   └── templates/
│       └── index.html           # Web UI 前端頁面（98 風格）
├── README.md                      # 專案說明文件
├── SPEC.md                        # 詳細規格文件
├── develope.md                    # 開發日誌與設計筆記
├── requirement.txt                # Python 依賴列表
└── DOCKERFILE                     # Docker 容器配置（支援 Linux）
```

### 關鍵檔案說明

| 檔案 | 說明 |
|------|------|
| `app/main.py` | Flask 應用主程式，負責 API 路由與遊戲生命週期管理 |
| `app/game_ffi.py` | C-Python 介面層，隱藏底層指標操作細節 |
| `app/core/game.c` | 遊戲核心邏輯（盤面、flood-fill、undo、狀態輸出） |
| `app/core/Makefile` | C 編譯腳本（支援 32-bit 與 64-bit 編譯） |
| `app/database/score.py` | 排行榜資料庫操作（CRUD 與排序） |
| `app/templates/index.html` | 前端 UI（98 風格、動態渲染） |
| `DOCKERFILE` | Docker 容器配置（Linux 環境，自動編譯 C 核心） |

## 系統需求

### 本地執行（Windows）
- Python 3.8 以上
- `gcc` 或 MinGW 編譯器（編譯 C 核心）

### 本地執行（Linux）
- Python 3.8 以上
- `gcc` 與 build-essential

### Docker 執行
- Docker 19.03 以上
- Docker Compose（選用，方便多容器管理）

## 安裝與執行

### 方式 1：本地執行（Windows）

#### 1.1 安裝 Python 依賴
```sh
pip install -r requirement.txt
```

#### 1.2 編譯 C 遊戲核心（使用 Makefile）
進入 `app/core/` 目錄：
```sh
cd app/core
make              # 編譯 64-bit DLL（預設）
make 32bit        # 編譯 32-bit DLL（若需要）
make clean        # 清理編譯產物
```

或手動編譯：
```sh
gcc -m64 -shared -o game.dll game.c
```

#### 1.3 執行遊戲
```sh
python -m app.main
```

### 方式 2：本地執行（Linux）

#### 2.1 安裝系統依賴與 Python 套件
```sh
sudo apt-get install gcc build-essential python3 python3-pip
pip install -r requirement.txt
```

#### 2.2 編譯 C 遊戲核心
```sh
cd app/core
gcc -shared -fPIC -o libgame.so game.c
```

#### 2.3 執行遊戲
```sh
python -m app.main
```

### 方式 3：Docker 執行

#### 3.1 建構 Docker 映像
```sh
docker build -t minesweeper:latest .
```

#### 3.2 執行容器
```sh
docker run -p 5000:5000 minesweeper:latest
```

或背景執行：
```sh
docker run -d -p 5000:5000 --name minesweeper minesweeper:latest
```

#### 3.3 停止容器
```sh
docker stop minesweeper
docker rm minesweeper
```

## 使用說明

1. 開啟瀏覽器並前往 `http://127.0.0.1:5000/` 或 `http://localhost:5000/`
2. **主頁面**：選擇「開始遊戲」或「顯示排行榜」
3. **難度選擇**：選擇內建難度（簡單 / 中等 / 困難）或自訂參數
4. **遊戲操作**：
   - 左鍵：揭開格子（單格或 flood-fill 展開）
   - 右鍵：插旗 / 取消旗
   - 「返回上一步」：撤回最後一次操作
5. **遊戲結束**：
   - 成功：輸入名稱提交成績
   - 失敗：可選擇返回上一步或退出
6. **排行榜**：查看所有難度的成績排名

## 注意事項
- **Platform 相符性**：Python 與 C 動態庫必須位元相符（32-bit 或 64-bit），否則會載入失敗
- **Windows 執行**：若程式閃退，確認 `app/core/game.dll` 存在且架構相符
- **Linux 執行**：Makefile 會自動編譯 `libgame.so`；或使用 Dockerfile 以容器隔離環境
- **資料庫路徑**：排行榜資料庫預設位置為 `app/database/game.sqlite`
- **Docker 注意**：容器內使用 Linux，會編譯 `libgame.so` 而非 `game.dll`

## 擴充建議
- 加入遊戲計時器與最佳分數統計
- 將前端改為桌面應用程式（例如 PyQt）
- 加入難度選單與成績排序篩選
