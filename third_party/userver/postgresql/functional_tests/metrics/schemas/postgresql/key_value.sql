CREATE TABLE IF NOT EXISTS key_value_table (
  key VARCHAR PRIMARY KEY,
  value VARCHAR,
  updated TIMESTAMPTZ NOT NULL DEFAULT NOW()
)
