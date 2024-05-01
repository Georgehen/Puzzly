import React from 'react';

function UserInput({ username, setUsername, generate }) {
    return (
        <div>
            <div className="field has-addons has-addons-centered">
                <div className="control is-expanded">
                    <input className="input is-large" type="text" placeholder="Lichess username" value={username} onChange={e => setUsername(e.target.value)} />
                </div>
                <div className="control">
                    <button className="button is-large is-primary" onClick={generate}>Generate tactics</button>
                </div>
            </div>
        </div>
    );
}

export default UserInput;
