### 專案介紹
以C語言為核心，以Python為前端，建立一個web進行踩地雷遊戲

### 專案規劃
遊戲內容為踩地雷，為二維版面分割成x*y份，並且有一定數量的炸彈，遊戲玩法為：
地板存在五種狀態：
1. 覆蓋地板
2. 覆蓋地板，被插上旗子
3. 翻開地板，顯示周圍有幾格地雷的數值地板
4. 翻開地板，周圍無地雷的空地板
5. 翻開地板，地雷地板

操作機制：
1. 左鍵：翻開地板，若為地雷，則遊戲失敗，若非地雷，則依據炸彈數量顯示數值或是空地板(對周圍進行判定，若為空地板則翻開所有連續的地板直到為數值地板)
2. 右鍵：點擊覆蓋地板可以插、拔旗子，點擊數值地板則判定周圍是否存在未插旗子的地雷，若地雷皆已插旗子，則自動翻開周圍的地板。
3. 若翻開全部地板，則遊戲成功，讓玩家輸入名稱，將分數記錄到資料庫
4. 顯示排行榜

遊戲分為五種難度，將版面與炸彈數量由Json獨立紀錄在Setting.json
1. 簡單難度 15*15版面 10%炸彈
2. 中等難度 50*50版面 30%炸彈
3. 困難難度 100*100版面 50%炸彈
4. 自訂難度 自訂版面 自訂炸彈數量

遊戲中：
至頂會顯示當前還有多少未確定位置的地雷，每次插旗子會減少數值，當數值歸零後會翻開所有未插旗子的覆蓋地板，如果存在地雷，則遊戲失敗
每次操作會記錄步驟到遊戲歷程，記錄位置、動作(翻開、插旗或是右鍵擴格子)
存在步驟，可以使用返回上一步，如果返回上一步，則將動作撤回

玩家流程：主頁面開始遊戲->選擇難度->開始遊戲->紀錄名稱(遊戲通關)->顯示排行榜->返回主頁面

### 專案設計
C語言為遊戲運行的主程序，提供接口
由Python Flask建置Web UI，使用ctypes調用C語言接口進行遊戲運算

### 專案規劃
使用資料結構建置地板的數值，使用linked list紀錄操作歷史以供返回上一步，直到第一步
遊戲開始時使用malloc配置版面與遊戲步驟的記憶體，遊戲結束進入排行榜前，釋放版面與遊戲步驟的記憶體

資料庫：sqlite
排行榜顯示、資料庫欄位：時間、名稱、分數、難度
分數計算方式，全部地板*地雷比例。

### UI設計
程序啟動後顯示主頁面：主頁面中上為標題踩地雷，中下為兩個直排按鈕，開始遊戲與顯示排行榜
主頁面：顯示直排的難度選擇，點擊難度進入遊戲或自訂難度輸入頁面
自訂難度輸入頁面：直排 []行版面、[]列版面、[]炸彈比例、進入遊戲
遊戲頁面：左上[返回上一步]、中上顯示當前炸彈數量、右上退出遊戲、中間為炸彈版面
輸入名稱：直排顯示 名稱：[] 確認按鈕
旁行榜：直排顯示資料，若此次分數在榜上，則高亮顯示分數

# Minesweeper Project (C core + Flask UI)

Prerequisites:
- C compiler (GCC or MSVC)
- Python 3.8+
- Matching architecture between Python and the compiled C DLL (64-bit Python needs a 64-bit DLL)

Build C library on Windows with MinGW (32-bit GCC):

```powershell
gcc -shared -o src/app/core/game.dll src/app/core/game.c
```

If you have a 64-bit compiler, build a 64-bit DLL for 64-bit Python:

```powershell
gcc -m64 -shared -o src/app/core/game.dll src/app/core/game.c
```

Install Python deps and run Flask:

```powershell
python -m venv .venv
.venv\Scripts\activate
pip install -r src/requirements.txt
python src/app/main.py
```

If the DLL cannot be loaded due to architecture mismatch, install a Python interpreter that matches the DLL bitness or use a matching compiler.
