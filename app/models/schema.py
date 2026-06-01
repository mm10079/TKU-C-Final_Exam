from pydantic import BaseModel, Field
from pathlib import Path
from enum import Enum
import os

BASE_DIR = Path(__file__).resolve().parent.parent

class Difficulty(str, Enum):
    easy = 'easy'
    medium = 'medium'
    hard = 'hard'
    custom = 'custom'

class DifficultyConfig(BaseModel):
    rows: int = Field(..., description="遊戲的列數")
    cols: int = Field(..., description="遊戲的行數")
    bomb_ratio: int = Field(..., description="地雷比例，百分比表示")

class DatabaseConfig(BaseModel):
    db_uri: str = Field(default='game.sqlite', description="資料庫檔案名稱")
    db_type: str = Field(default='sqlite', description="資料庫類型，例如 sqlite、postgresql 等")

    @property
    def db_url(self) -> str:
        path = self.db_uri
        if not os.path.isabs(self.db_uri):
            path = os.path.join(BASE_DIR, 'database', self.db_uri)
        url = f"{self.db_type}:///{path}"
        return url

    @property
    def db_path(self) -> str:
        return os.path.join(BASE_DIR, 'database', self.db_uri)

class Config(BaseModel):
    appName: str = Field(default='踩地雷', description="應用程式名稱")
    version: str = Field(default='1.0.0', description="應用程式版本")
    listen_host: str = Field(default='localhost', description="監聽的主機名稱")
    listen_port: int = Field(default=8080, description="監聽的端口")
    database: DatabaseConfig = Field(default_factory=DatabaseConfig)
    difficulties: dict[Difficulty, DifficultyConfig] = Field(default_factory=dict)

    @property
    def host(self) -> str:
        if self.listen_host == 'localhost':
            return '0.0.0.0'
        return self.listen_host
    
    @property
    def port(self) -> int:
        if self.listen_port <= 0 or self.listen_port > 65535:
            return 8080
        return self.listen_port
    
    @property
    def get_difficulties(self) -> dict[str, dict]:
        return {diff.value: config.model_dump() for diff, config in self.difficulties.items()}