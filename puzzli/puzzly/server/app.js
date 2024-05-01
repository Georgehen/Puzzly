const express = require('express');
const pool = require('./db/queries'); 
const app = express();
const cors = require('cors');

app.use(cors());
app.use(express.json());

// Endpoint to fetch tactics for a given username
app.get('/api/tactics/:username', async (req, res) => {
    const { username } = req.params;
    try {
        const result = await pool.query(
            'SELECT fen, description FROM tactics JOIN users ON users.user_id = tactics.user_id WHERE users.username = $1',
            [username]
        );
        if (result.rows.length > 0) {
            res.json(result.rows);
        } else {
            res.status(404).json({ message: "No tactics found for this user." });
        }
    } catch (err) {
        console.error(err);
        res.status(500).send("Server error");
    }
});

// Endpoint to post new tactics for a given username
app.post('/api/tactics/:username', async (req, res) => {
    const { username } = req.params;
    const { fen, description } = req.body;

    try {
        // Ensure user exists in the database or insert them
        let userResult = await pool.query(
            'SELECT user_id FROM users WHERE username = $1',
            [username]
        );

        let userId;

        if (userResult.rows.length === 0) {
            // Insert the user if not exists
            userResult = await pool.query(
                'INSERT INTO users (username) VALUES ($1) RETURNING user_id',
                [username]
            );
            userId = userResult.rows[0].user_id;
        } else {
            userId = userResult.rows[0].user_id;
        }

        // Insert the new tactic
        await pool.query(
            'INSERT INTO tactics (user_id, fen, description) VALUES ($1, $2, $3)',
            [userId, fen, description]
        );
        res.status(201).send('Tactic added successfully.');
    } catch (err) {
        console.error('Error adding tactic:', err);
        res.status(500).send('Failed to add tactic');
    }
});


module.exports = app;
