import React, { useState } from 'react';
import axios from 'axios';
import { Chess } from 'chess.js';
import Navbar from './components/NavBar';
import UserInput from './components/UserInput';

function App() {
    const [username, setUsername] = useState('Mahlloch');
    const [blunderPositions, setBlunderPositions] = useState([]); // To store FENs

   const generate = async () => {
        try {
            const localResp = await axios.get(`http://localhost:5000/api/tactics/${username}`);
            console.log(localResp.data.length);
            if (localResp.data.length === 0) {
                const response = await axios.get(`https://lichess.org/api/games/user/${username}?max=200&analysed=true&evals=true&moves=true&pgnInJson=true&perfType=blitz,rapid,classical`, {
                    headers: { 'Accept': 'application/x-ndjson' }
                });
                const games = response.data.split('\n').filter(x => x).map(x => JSON.parse(x));
                const blunders = findBlunders(games, username);
                setBlunderPositions(blunders);
            } else {
                setBlunderPositions(localResp.data); // Use the locally stored tactics if available
            }
        } catch (error) {
            console.error('Error fetching tactics:', error);
        }
    };

    function findBlunders(games, username) {
      let blunderFENs = [];

      games.forEach((game) => {

        // Determine which color the user is playing
        const moves = game.moves;
        const players = game.players;
        const userColor = players.white.user.name === username ? 'white' : 'black';
        const analysis = game.analysis;
        const moveList = moves.split(' ');
        console.log(moveList);
        const chess = new Chess();

        let idx = 0;


        // Process each move
        moveList.forEach((move) => {
          // Check if the current move is a blunder by the user
          if (analysis[idx] && analysis[idx].judgment && analysis[idx].judgment.name === 'Blunder') {
            const currentPlayer = idx % 2 === 0 ? 'white' : 'black';

            // Check if the blunder was made by the user
            if (currentPlayer === userColor) {

              // Undo the blunder move to get the position before the move
              // Add the FEN of the position before the blunder
              blunderFENs.push(chess.fen());
            }
          }
          chess.move(move);
          idx++;
        });
      });

      return blunderFENs;
    }



    return (
        <div id="app">
            <section className="hero is-fullheight">
                <div className="hero-head">
                    <Navbar />
                </div>
                <div className="hero-body">
                    <div className="container">
                        <UserInput
                            username={username}
                            setUsername={setUsername}
                            generate={generate}
                        />
                        <div className="has-text-centered">
                            Enter your Lichess username to generate your tactics based on blunders.
                        </div>
                        <br />
                        {blunderPositions.map((fen, index) => (
                            <div key={index} className="button is-link is-outlined is-fullwidth mb-2">
                                <a href={`https://lichess.org/analysis/fromPosition/${encodeURIComponent(fen)}`} target="_blank" rel="noopener noreferrer">
                                    Analyze FEN before blunder {index + 1}
                                </a>
                            </div>
                        ))}
                    </div>
                </div>
            </section>
        </div>
    );
}

export default App;
