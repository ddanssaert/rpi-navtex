"""add filters column to push_subscriptions

Revision ID: 003
Revises: 002
Create Date: 2026-04-27

"""
from alembic import op
import sqlalchemy as sa

revision = '003'
down_revision = '002'
branch_labels = None
depends_on = None


def upgrade() -> None:
    op.add_column('push_subscriptions', sa.Column('filters', sa.JSON(), nullable=True))


def downgrade() -> None:
    op.drop_column('push_subscriptions', 'filters')
