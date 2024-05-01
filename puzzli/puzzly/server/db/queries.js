const { Pool } = require('pg');
const pool = new Pool({
  user: 'tor',
  host: 'localhost',
  database: 'chess_db',
  password: 'airongstring',
  port: 5432,
});

module.exports = pool;
