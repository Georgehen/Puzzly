CREATE TABLE users (
    user_id SERIAL PRIMARY KEY,
    username VARCHAR(255) UNIQUE NOT NULL
);

CREATE TABLE tactics (
    tactic_id SERIAL PRIMARY KEY,
    user_id INT NOT NULL,
    fen TEXT NOT NULL,
    description TEXT,
    FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE CASCADE
);

