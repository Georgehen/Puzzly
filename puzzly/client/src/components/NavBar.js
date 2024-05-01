import React from 'react';

function NavBar() {
    return (
        <nav className="navbar">
            <div className="container">
                <div>
                    <a href="/">
                        <p className="title">Puzzly</p>
                        <p className="subtitle">Generate chess tactics puzzles from your lichess games.</p>
                    </a>
                </div>
                <div id="navbarMenu" className="navbar-menu">
                    <div className="navbar-end"></div>
                </div>
            </div>
        </nav>
    );
}

export default NavBar;
