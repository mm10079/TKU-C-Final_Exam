from flask import Flask, jsonify, request, render_template
import os
import sqlite3
from game_ffi import get_ffi
import json

app = Flask(__name__, template_folder='templates', static_folder='static')
_ffi = get_ffi()

DB_PATH = os.path.join(os.path.dirname(__file__), '..', 'db', 'game.db')

GAME_CONFIG = {
    'easy': {'rows': 9, 'cols': 9, 'bomb_ratio': 10},
    'medium': {'rows': 16, 'cols': 16, 'bomb_ratio': 15},
    'hard': {'rows': 16, 'cols': 30, 'bomb_ratio': 20},
}

def init_db():
    os.makedirs(os.path.dirname(DB_PATH), exist_ok=True)
    conn = sqlite3.connect(DB_PATH)
    c = conn.cursor()
    c.execute('''CREATE TABLE IF NOT EXISTS scoreboard (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT, score INTEGER, difficulty TEXT, time DATETIME DEFAULT CURRENT_TIMESTAMP)''')
    conn.commit()
    conn.close()

init_db()

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/api/new_game', methods=['POST'])
def new_game():
    data = request.json or {}
    rows = int(data.get('rows', 15))
    cols = int(data.get('cols', 15))
    bombs = int(data.get('bombs', 0))
    _ffi.init(rows, cols, bombs)
    return jsonify({'status':'ok', 'rows':rows, 'cols':cols, 'bombs':bombs})

@app.route('/api/get_settings')
def get_settings():
    return jsonify(GAME_CONFIG)

@app.route('/api/state')
def state():
    s = _ffi.get_state()
    return app.response_class(s, mimetype='application/json')


@app.route('/api/click', methods=['POST'])
def click():
    data = request.json or {}
    r = int(data.get('row',0))
    c = int(data.get('col',0))
    kind = data.get('kind','left')
    if kind == 'left':
        res = _ffi.left_click(r, c)
    else:
        res = _ffi.right_click(r, c)
    return jsonify({'result':res})

@app.route('/api/undo', methods=['POST'])
def undo():
    res = _ffi.undo()
    return jsonify({'result':res})

@app.route('/api/submit_score', methods=['POST'])
def submit_score():
    data = request.json or {}
    name = data.get('name','anon')
    score = int(data.get('score',0))
    difficulty = data.get('difficulty','custom')
    conn = sqlite3.connect(DB_PATH)
    c = conn.cursor()
    c.execute('INSERT INTO scoreboard (name, score, difficulty) VALUES (?, ?, ?)', (name, score, difficulty))
    conn.commit()
    conn.close()
    return jsonify({'status':'ok'})

@app.route('/api/get_leaderboard')
def leaderboard():
    conn = sqlite3.connect(DB_PATH)
    c = conn.cursor()
    c.execute('SELECT name, score, difficulty, time FROM scoreboard ORDER BY score DESC LIMIT 20')
    rows = [{'name':r[0],'score':r[1],'difficulty':r[2],'time':r[3]} for r in c.fetchall()]
    conn.close()
    return jsonify(rows)

if __name__ == '__main__':
    init_db()
    app.run(debug=True)
