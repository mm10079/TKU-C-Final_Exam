from sqlmodel import create_engine, Session, SQLModel, Field, select
from sqlalchemy import case
from typing import Optional, Sequence
from datetime import datetime

class Score(SQLModel, table=True):
    id: int = Field(default=None, primary_key=True)
    name: str = Field(default='無名氏')
    score: int
    difficulty: str
    time: datetime = Field(default_factory=datetime.utcnow)

class db:
    def __init__(self, db_url: str = 'data.db', db_type: str = 'sqlite'):
        database_url = f"{db_type}:///{db_url}"
        self.engine = create_engine(database_url, connect_args={"check_same_thread": False, "timeout": 30})
        self.init_db()

    # 初始化資料庫
    def init_db(self):
        # 因為在這裡匯入了 schema，SQLModel 就能找到所有的 table=True
        SQLModel.metadata.create_all(self.engine)

    def get_session(self) -> Session:
        return Session(self.engine)

    def add_data(self, data: Score):
        with self.get_session() as session:
            session.add(data)
            session.commit()
            session.refresh(data)

    def get_data(self) -> Optional[Sequence[Score]]:
        difficulty_order = case(
            (Score.difficulty == "hard", 4),
            (Score.difficulty == "medium", 3),
            (Score.difficulty == "easy", 2),
            (Score.difficulty == "custom", 1),
            else_=0,  # 防呆：如果有其他預料之外的字串，排在最後面
        )
        with self.get_session() as session:
            return session.exec(select(Score).order_by(
                difficulty_order,
                Score.score,
                Score.time
                )).all()
        
    def update_value(self, data: Score):
        with self.get_session() as session:
            session.add(data)
            session.commit()
            session.refresh(data)