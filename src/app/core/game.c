#include "game.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

// 磚塊狀態列舉：
// TILE_COVERED   = 未翻開
// TILE_FLAGGED   = 標記旗幟
// TILE_REVEALED  = 已揭開
typedef enum { TILE_COVERED = 0, TILE_FLAGGED = 1, TILE_REVEALED = 2 } TileState;
// 操作類型列舉：
// LEFT_REVEAL   = 左鍵揭開
// RIGHT_TOGGLE = 右鍵切換
typedef enum { LEFT_REVEAL = 1, RIGHT_TOGGLE = 2 } ActionType;

// 單一格子資料結構
typedef struct {
    int has_bomb;   // 是否含有地雷
    int adj;        // 鄰近八格地雷數
    TileState state; // 目前狀態
} Tile;

// 操作歷史節點，用於 undo 功能
typedef struct HistoryNode {
    ActionType action; /* 操作類別 */
    int r, c;
    int *revealed; /* left reveal 時實際揭開的格子列表，平面陣列 [r,c,r,c,...] */
    int revealed_count; // revealed 陣列中的格子數量
    int prev_flag; // 右鍵切換時，前一個狀態是否為旗子
    struct HistoryNode* prev; // 串接前一個歷史節點
} HistoryNode;

// 遊戲主控結構
struct GameHandle {
    int rows; // 欄數
    int cols; // 列數
    int bomb_count; // 地雷總數
    int flags_count; // 已插旗數
    int moves; // 歷史操作步數
    int game_over_type; /* 0=none, 1=win, 2=lose */
    Tile *tiles; // 所有格子的平面陣列
    HistoryNode *history; // undo 歷史鏈表
};

// 計算平面陣列索引：r、c 轉成 tiles 中的偏移值
static inline int idx(GameHandle* h, int r, int c) {
    return r * h->cols + c;
}

// 隨機放置地雷到盤面上
static void place_bombs(GameHandle* h, int bomb_count) {
    int total = h->rows * h->cols;
    if (bomb_count <= 0 || bomb_count > total) bomb_count = total / 10;
    srand((unsigned)time(NULL));
    int placed = 0;
    while (placed < bomb_count) {
        int i = rand() % total;
        if (!h->tiles[i].has_bomb) {
            h->tiles[i].has_bomb = 1;
            placed++;
        }
    }
}

// 計算每個格子的鄰近地雷數
static void compute_adjacency(GameHandle* h) {
    for (int r = 0; r < h->rows; ++r) {
        for (int c = 0; c < h->cols; ++c) {
            int count = 0;
            for (int dr = -1; dr <= 1; ++dr) {
                for (int dc = -1; dc <= 1; ++dc) {
                    if (dr == 0 && dc == 0) continue;
                    int nr = r + dr;
                    int nc = c + dc;
                    if (nr < 0 || nr >= h->rows || nc < 0 || nc >= h->cols) continue;
                    if (h->tiles[idx(h, nr, nc)].has_bomb) count++;
                }
            }
            h->tiles[idx(h, r, c)].adj = count;
        }
    }
}

// 將新歷史節點推入 undo 鏈表
static void push_history(GameHandle* h, HistoryNode* node) {
    node->prev = h->history;
    h->history = node;
}

// 從 undo 鏈表彈出最新歷史節點
static HistoryNode* pop_history(GameHandle* h) {
    HistoryNode* n = h->history;
    if (!n) return NULL;
    h->history = n->prev;
    return n;
}

// 初始化遊戲盤面
//
// 運作方式：
// 1. 分配 GameHandle 結構並初始化行列、地雷數、狀態等
// 2. 使用 calloc 建立一個連續的 Tile 平面陣列
// 3. 初始化每格為未揭開、無地雷、adj=0
// 4. 呼叫 place_bombs 隨機放置地雷
// 5. 呼叫 compute_adjacency 計算每格周圍地雷數
// 6. 設定 undo 歷史為空
GAMEDLLAPI GameHandle* game_init(int rows, int cols, int bomb_count) {
    if (rows <= 0 || cols <= 0) return NULL;
    GameHandle* h = (GameHandle*)malloc(sizeof(GameHandle));
    if (!h) return NULL;
    h->rows = rows;
    h->cols = cols;
    h->bomb_count = bomb_count;
    h->flags_count = 0;
    h->moves = 0;
    h->game_over_type = 0;
    h->tiles = (Tile*)calloc(rows * cols, sizeof(Tile));
    if (!h->tiles) {
        free(h);
        return NULL;
    }
    for (int i = 0; i < rows * cols; i++) {
        h->tiles[i].has_bomb = 0;
        h->tiles[i].adj = 0;
        h->tiles[i].state = TILE_COVERED;
    }
    place_bombs(h, bomb_count);
    compute_adjacency(h);
    h->history = NULL;
    return h;
}

// 執行 flood-fill，收集左鍵點擊揭開的格子位置
//
// 運作方式：
// 1. 使用 stack 進行深度優先遍歷，從起始格 (r,c) 開始
// 2. 用 visited 記錄已加入 stack 的格子，避免重複處理
// 3. 當格子為未揭開且非旗幟時，標記為已揭開並加入結果 pairs
// 4. 如果該格子鄰近地雷數為 0，則把相鄰未揭開格子加入 stack
// 5. 最後回傳所有揭開格子的座標與數量
static int reveal_collect(GameHandle* h, int r, int c, int **out_pairs, int *out_count) {
    int rows = h->rows;
    int cols = h->cols;
    int max = rows * cols;
    
    int *pairs = (int*)malloc(sizeof(int) * 2 * max);
    if (!pairs) return -1;
    int pc = 0;
    int *stack = (int*)malloc(sizeof(int) * 2 * max);
    int *visited = (int*)calloc(max, sizeof(int));
    if (!stack || !visited) {
        free(pairs);
        free(stack);
        free(visited);
        return -1;
    }
    int sp = 0;
    
    int start_idx = idx(h, r, c);
    if (r >= 0 && r < rows && c >= 0 && c < cols) {
        visited[start_idx] = 1;
        stack[sp++] = r;
        stack[sp++] = c;
    }
    
    while (sp > 0) {
        int cc = stack[--sp];
        int rr = stack[--sp];
        
        if (rr < 0 || rr >= rows || cc < 0 || cc >= cols) continue;
        
        Tile *t = &h->tiles[idx(h, rr, cc)];
        if (t->state == TILE_REVEALED || t->state == TILE_FLAGGED) continue;
        
        t->state = TILE_REVEALED;
        
        pairs[pc * 2] = rr;
        pairs[pc * 2 + 1] = cc;
        pc++;
        
        if (t->has_bomb) continue;
        
        if (t->adj == 0) {
            for (int dr = -1; dr <= 1; ++dr) {
                for (int dc = -1; dc <= 1; ++dc) {
                    if (dr == 0 && dc == 0) continue;
                    
                    int nr = rr + dr;
                    int nc = cc + dc;
                    if (nr < 0 || nr >= rows || nc < 0 || nc >= cols) continue;
                    int nidx = nr * cols + nc;
                    if (visited[nidx]) continue;
                    Tile *nt = &h->tiles[nidx];
                    if (nt->state == TILE_COVERED) {
                        visited[nidx] = 1;
                        stack[sp++] = nr;
                        stack[sp++] = nc;
                    }
                }
            }
        }
    }
    
    free(stack);
    free(visited);
    *out_pairs = pairs;
    *out_count = pc;
    return 0;
}

// 判斷是否已達成勝利條件
//
// 運作方式：
// 1. 計算目前還未揭開或被插旗的格子數量
// 2. 如果這個數量等於地雷總數，代表所有安全格已被揭開
// 3. 將 game_over_type 設為 1 表示獲勝
static void check_win_condition(GameHandle* h) {
    // 如果已經爆炸失敗了，就不重複判定
    if (h->game_over_type == 2) return; 

    int total_cells = h->rows * h->cols;
    int covered_or_flagged_count = 0;

    for (int i = 0; i < total_cells; i++) {
        // 只要不是 REVEALED (也就是還是 COVERED 或 FLAGGED)，就計數
        if (h->tiles[i].state != TILE_REVEALED) {
            covered_or_flagged_count++;
        }
    }

    // 當「留在場上沒翻開的格子數」剛好等於「炸彈總數」，代表安全格子都翻完了 -> 獲勝！
    if (covered_or_flagged_count == h->bomb_count) {
        h->game_over_type = 1; // 標記為勝利
    }
}

// 處理左鍵點擊：揭開格子、記錄到 undo、判斷是否爆炸或獲勝
//
// 運作方式：
// 1. 檢查邊界與遊戲是否結束
// 2. 如果點擊格子已被標記或揭開，直接返回
// 3. 建立一個 HistoryNode 保存本次操作資訊
// 4. 呼叫 reveal_collect 實際揭開格子，並將結果存入 history
// 5. 如果任何格子是地雷，設定 game_over_type 為 2 表示失敗
// 6. 否則呼叫 check_win_condition 判斷是否獲勝
GAMEDLLAPI int game_left_click(GameHandle* h, int r, int c) {
    if (!h) return -1;
    if (r < 0 || r >= h->rows || c < 0 || c >= h->cols) return -1;
    if (h->game_over_type) return -1;
    Tile *t = &h->tiles[idx(h, r, c)];
    if (t->state == TILE_FLAGGED || t->state == TILE_REVEALED) return 0;
    HistoryNode *node = (HistoryNode*)malloc(sizeof(HistoryNode));
    if (!node) return -1;
    node->action = LEFT_REVEAL;
    node->r = r;
    node->c = c;
    node->revealed = NULL;
    node->revealed_count = 0;
    node->prev_flag = 0;
    node->prev = NULL;
    int *pairs = NULL;
    int pc = 0;
    reveal_collect(h, r, c, &pairs, &pc);
    node->revealed = pairs;
    node->revealed_count = pc;
    push_history(h, node);
    h->moves += 1;
    for (int i = 0; i < pc; i++) {
        int rr = pairs[i * 2];
        int cc = pairs[i * 2 + 1];
        if (h->tiles[idx(h, rr, cc)].has_bomb) {
            h->game_over_type = 2;
            return 1;
        }
    }
    check_win_condition(h);
    return 0;
}

// 處理右鍵點擊：標旗或取消旗，並記錄到 undo
//
// 運作方式：
// 1. 檢查邊界與遊戲是否結束
// 2. 建立 HistoryNode 並記錄格子的前一個旗子狀態
// 3. 如果格子為 COVERED 則改成 FLAGGED，若為 FLAGGED 則改成 COVERED
// 4. 更新旗子數量並將操作存入 history
GAMEDLLAPI int game_right_click(GameHandle* h, int r, int c) {
    if (!h) return -1;
    if (r < 0 || r >= h->rows || c < 0 || c >= h->cols) return -1;
    if (h->game_over_type) return -1;
    Tile *t = &h->tiles[idx(h, r, c)];
    HistoryNode *node = (HistoryNode*)malloc(sizeof(HistoryNode));
    if (!node) return -1;
    node->action = RIGHT_TOGGLE;
    node->r = r;
    node->c = c;
    node->revealed = NULL;
    node->revealed_count = 0;
    node->prev = NULL;
    node->prev_flag = (t->state == TILE_FLAGGED) ? 1 : 0;
    if (t->state == TILE_COVERED) {
        t->state = TILE_FLAGGED;
        h->flags_count += 1;
    } else if (t->state == TILE_FLAGGED) {
        t->state = TILE_COVERED;
        if (h->flags_count > 0) h->flags_count -= 1;
    }
    push_history(h, node);
    h->moves += 1;
    return 0;
}

// 復原上一次操作，支援左鍵揭開與右鍵標旗的 undo
//
// 運作方式：
// 1. 從 history 鏈表彈出最新操作節點
// 2. 若為 LEFT_REVEAL，將該次揭開的所有格子還原為 COVERED
// 3. 若為 RIGHT_TOGGLE，依 prev_flag 還原格子為 FLAGGED 或 COVERED，並調整 flag 計數
// 4. 釋放該歷史節點與其 revealed 陣列
// 5. 重置 game_over_type 與 moves
GAMEDLLAPI int game_undo(GameHandle* h) {
    if (!h) return -1;
    HistoryNode* n = pop_history(h);
    if (!n) return -1;
    if (n->action == LEFT_REVEAL) {
        for (int i = 0; i < n->revealed_count; i++) {
            int rr = n->revealed[i * 2];
            int cc = n->revealed[i * 2 + 1];
            Tile *t = &h->tiles[idx(h, rr, cc)];
            t->state = TILE_COVERED;
        }
        free(n->revealed);
    } else if (n->action == RIGHT_TOGGLE) {
        Tile *t = &h->tiles[idx(h, n->r, n->c)];
        if (n->prev_flag) {
            t->state = TILE_FLAGGED;
            h->flags_count += 1;
        } else {
            t->state = TILE_COVERED;
            if (h->flags_count > 0) h->flags_count -= 1;
        }
    }
    free(n);
    if (h->game_over_type) h->game_over_type = 0;
    if (h->moves > 0) h->moves -= 1;
    return 0;
}

// 將遊戲狀態序列化成 JSON 字串，供 Python 端讀取
//
// 運作方式：
// 1. 將基本遊戲狀態 (rows、cols、bomb_count、flags_count、moves、game_over_type) 寫入緩衝區
// 2. 依序遍歷每個格子，輸出它的 r/c/state/adj 以及是否顯示炸彈
// 3. show_bomb 只在該格已揭開且為地雷時回傳 1
// 4. 回傳實際寫入緩衝區的位元組數
GAMEDLLAPI int game_get_state(GameHandle* h, char* buffer, size_t bufsize) {
    if (!h || !buffer || bufsize == 0) return -1;
    size_t off = 0;
    int written = snprintf(buffer + off, bufsize - off,
                           "{\"rows\":%d,\"cols\":%d,\"bomb_count\":%d,\"flags_count\":%d,\"moves\":%d,\"game_over_type\":%d,\"tiles\":[",
                           h->rows, h->cols, h->bomb_count, h->flags_count, h->moves, h->game_over_type);
    if (written < 0) return -1;
    off += (size_t)written;
    if (off >= bufsize) return (int)bufsize - 1;
    for (int r = 0; r < h->rows; r++) {
        for (int c = 0; c < h->cols; c++) {
            Tile *t = &h->tiles[idx(h, r, c)];
            int show_bomb = (t->has_bomb && t->state == TILE_REVEALED) ? 1 : 0;
            written = snprintf(buffer + off, bufsize - off,
                               "{\"r\":%d,\"c\":%d,\"state\":%d,\"adj\":%d,\"bomb\":%d}",
                               r, c, (int)t->state, t->adj, show_bomb);
            if (written < 0) return -1;
            off += (size_t)written;
            if (off >= bufsize) return (int)bufsize - 1;
            if (!(r == h->rows - 1 && c == h->cols - 1)) {
                if (off + 2 < bufsize) {
                    buffer[off++] = ',';
                    buffer[off] = '\0';
                }
            }
        }
    }
    written = snprintf(buffer + off, bufsize - off, "]}\n");
    if (written < 0) return -1;
    off += (size_t)written;
    if (off >= bufsize) return (int)bufsize - 1;
    return (int)off;
}

// 釋放遊戲資源，包括 tiles 與 undo 歷史鏈表
//
// 運作方式：
// 1. 釋放 tiles 連續平面陣列
// 2. 逐一遍歷 history 鏈表，釋放每個 revealed 陣列與節點
// 3. 最後釋放 GameHandle 本體
GAMEDLLAPI void game_free(GameHandle* h) {
    if (!h) return;
    if (h->tiles) free(h->tiles);
    HistoryNode* n = h->history;
    while (n) {
        HistoryNode* p = n->prev;
        if (n->revealed) free(n->revealed);
        free(n);
        n = p;
    }
    free(h);
}
