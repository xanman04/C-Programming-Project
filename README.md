# Multiplayer Blackjack in C

A networked, multiplayer Blackjack game implemented in C using TCP sockets.  
The project includes a full Blackjack engine, a complete deck/shuffling system, and a client–server architecture that allows multiple players to connect and play live rounds together.

---

## Overview

This project implements a simple but fully functional Blackjack game where a central server manages the deck, deals cards, coordinates turns, and determines results.  
Up to five clients can connect over TCP and play against the dealer in real time.

Everything is written in standard C with no external libraries.  
The project demonstrates systems programming concepts such as:

- Socket communication  
- `select()`-based connection handling  
- Modular game logic  
- Structured data design  

---

## Features

### ✔ Fully Networked Multiplayer
- Up to 5 players can connect simultaneously  
- New players can join between rounds  
- Server removes players cleanly if they disconnect  
- Clients receive live prompts and game updates  

### ✔ Complete Blackjack Engine
- Card values, Ace logic (11→1), blackjack detection  
- Bust and win/lose/push calculations  
- Dealer plays automatically (hits until 17)  

### ✔ Deck & Card System
- 52-card deck  
- Fisher–Yates shuffle  
- Automatic reshuffling when empty  
- Short readable card strings (e.g., `AH`, `10D`)  

### ✔ Clear Client UI
Clients receive simple text commands from the server and display them in a playable format:

- Dealer’s up card  
- Player’s hand  
- Hit/Stand prompts  
- Result messages  
- Round-end notifications  

---

## File Structure

```text
/include
    blackjack.h   – hand logic (values, blackjack, bust, formatting)
    deck.h        – card + deck definitions

/src
    blackjack.c   – implementation of hand operations
    deck.c        – deck creation, shuffling, dealing, formatting
    server.c      – multiplayer Blackjack server
    client.c      – interactive client program

README.md
Makefile      – build instructions (dependent on your environment)
```

---

## How the Game Works

### 1. Server
- Accepts incoming TCP connections  
- Creates and shuffles a deck each round  
- Deals two cards to each active player and the dealer  
- Sends each player their hand and the dealer’s up card  
- Handles each player's turn:  
  - Sends prompts  
  - Receives `HIT` / `STAND` decisions  
  - Deals new cards and reports busts or blackjack  
- Plays the dealer’s hand  
- Sends results (`WIN` / `LOSE` / `PUSH`)  
- Starts the next round automatically  

### 2. Client
- Connects to the server via IP + port  
- Prints all server messages in a user-friendly format  
- Asks the user for `HIT` or `STAND`  
- Continues playing rounds until disconnected  
