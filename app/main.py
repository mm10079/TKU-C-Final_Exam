from flask import Flask, jsonify, request, render_template
from datetime import datetime
import json
import os

from models.schema import Config
from database.score import db, Score
from game_ffi import get_ffi

def load_config():
    config_path = os.path.join(os.path.dirname(__file__) , 'config.json')
    if os.path.exists(config_path):
        with open(config_path, 'r') as f:
            return Config(**json.load(f))
    else:
        raise FileNotFoundError(f"Config file not found at {config_path}")
    
config = load_config()
database = db(config.database.db_url) 

app = Flask(__name__, template_folder='templates', static_folder='static')
_ffi = get_ffi()

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/api/new_game', methods=['POST'])
def new_game():
    data = request.json or {}
    rows = int(data.get('rows', 15))
    cols = int(data.get('cols', 15))
    bombs = int(data.get('bombs', 0))
    mode = str(data.get('difficulty', 'custom'))
    if bombs <= 0 or bombs >= rows * cols:
        bombs = 1
    _ffi.init(rows, cols, bombs, mode)
    return jsonify({'status':'ok', 'rows':rows, 'cols':cols, 'bombs':bombs, 'difficulty':mode})

@app.route('/api/get_settings')
def get_settings():
    return jsonify(config.get_difficulties)

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
    print("點擊事件：", kind, r, c)  # 調試用
    if kind == 'left':
        res = _ffi.left_click(r, c)
    else:
        res = _ffi.right_click(r, c)
    return jsonify({'result':res})

@app.route('/api/undo', methods=['POST'])
def undo():
    print("撤回事件")
    res = _ffi.undo()
    return jsonify({'result':res})

@app.route('/api/submit_score', methods=['POST'])
def submit_score():
    print("新增紀錄")
    data = request.json or {}
    name = data.get('name','anon')
    score = int(data.get('score',0))
    time = datetime.now()
    print(f"玩家 {name} 提交分數：{score}，難度：{_ffi.difficulty}，時間：{time}")
    new_score = Score(name=name, score=score, difficulty=_ffi.difficulty, time=time)
    try:
        database.add_data(new_score)
        print("紀錄新分數：", new_score)
        return jsonify({'status':'ok', 'difficulty':_ffi.difficulty, 'time': time.strftime('%Y-%m-%d %H:%M:%S')})
    except Exception as e:
        print("新增分數時發生錯誤：", e)
        return jsonify({'status':'error', 'message': str(e)})

@app.route('/api/get_leaderboard')
def leaderboard():
    print("載入排行榜")
    scores = database.get_data()
    if scores is None:
        print("沒有找到任何紀錄")
        return jsonify([])
    rows = [{'name':s.name,'score':s.score,'difficulty':s.difficulty,'time':s.time.strftime('%Y-%m-%d %H:%M:%S')} for s in scores]
    print("總紀錄數量：", len(rows))
    return jsonify(rows)

@app.route('/api/free_game', methods=['POST'])
def free_game():
    print("釋放遊戲資源")
    _ffi.free()
    return jsonify({'status':'ok'})

if __name__ == '__main__':
    print("啟動伺服器")
    app.run(debug=True, host=config.host, port=config.port)
