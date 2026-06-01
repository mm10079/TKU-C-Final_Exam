#include "game.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

typedef enum { TILE_COVERED = 0, TILE_FLAGGED = 1, TILE_REVEALED = 2 } TileState;

typedef struct {
    int has_bomb;
    int adj;
    TileState state;
} Tile;

typedef struct HistoryNode {
    int action; /* 1=left reveal, 2=right toggle */
    int r, c;
    int *revealed; /* flat array of r,c pairs for left reveals */
    int revealed_count;
    int prev_flag;
    struct HistoryNode* prev;
} HistoryNode;

struct GameHandle {
    int rows;
    int cols;
    int bomb_count;
    int flags_count;
    int moves;
    int game_over_type; /* 0=none, 1=win, 2=lose */
    Tile *tiles;
    HistoryNode *history;
};

static inline int idx(GameHandle* h, int r, int c) {
    return r * h->cols + c;
}

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

static void push_history(GameHandle* h, HistoryNode* node) {
    node->prev = h->history;
    h->history = node;
}

static HistoryNode* pop_history(GameHandle* h) {
    HistoryNode* n = h->history;
    if (!n) return NULL;
    h->history = n->prev;
    return n;
}

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

GAMEDLLAPI int game_left_click(GameHandle* h, int r, int c) {
    if (!h) return -1;
    if (r < 0 || r >= h->rows || c < 0 || c >= h->cols) return -1;
    if (h->game_over_type) return -1;
    Tile *t = &h->tiles[idx(h, r, c)];
    if (t->state == TILE_FLAGGED || t->state == TILE_REVEALED) return 0;
    HistoryNode *node = (HistoryNode*)malloc(sizeof(HistoryNode));
    if (!node) return -1;
    node->action = 1;
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

GAMEDLLAPI int game_right_click(GameHandle* h, int r, int c) {
    if (!h) return -1;
    if (r < 0 || r >= h->rows || c < 0 || c >= h->cols) return -1;
    if (h->game_over_type) return -1;
    Tile *t = &h->tiles[idx(h, r, c)];
    HistoryNode *node = (HistoryNode*)malloc(sizeof(HistoryNode));
    if (!node) return -1;
    node->action = 2;
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

GAMEDLLAPI int game_undo(GameHandle* h) {
    if (!h) return -1;
    HistoryNode* n = pop_history(h);
    if (!n) return -1;
    if (n->action == 1) {
        for (int i = 0; i < n->revealed_count; i++) {
            int rr = n->revealed[i * 2];
            int cc = n->revealed[i * 2 + 1];
            Tile *t = &h->tiles[idx(h, rr, cc)];
            t->state = TILE_COVERED;
        }
        free(n->revealed);
    } else if (n->action == 2) {
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
