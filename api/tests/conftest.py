import pytest
import os
from sqlalchemy.ext.asyncio import create_async_engine, AsyncSession
from sqlalchemy.orm import sessionmaker
from src.database import Base
import asyncio

@pytest.fixture(scope="session")
def setup_test_env(tmp_path_factory):
    tmp_path = tmp_path_factory.mktemp("data")
    db_file = tmp_path / "test.db"
    vapid_file = tmp_path / "vapid_keys.json"
    
    os.environ["DATABASE_URL"] = f"sqlite+aiosqlite:///{db_file}"
    os.environ["VAPID_KEYS_FILE"] = str(vapid_file)
    
    yield tmp_path

@pytest.fixture(autouse=True)
async def setup_db(setup_test_env):
    from src.database import engine
    async with engine.begin() as conn:
        await conn.run_sync(Base.metadata.create_all)
    yield
    async with engine.begin() as conn:
        await conn.run_sync(Base.metadata.drop_all)
