#ifndef GAME_H
#define GAME_H

#include "Player.h"
#include "Card.h"
#include <vector>
#include <deque>
#include <string>
#include <random>
#include <SFML/Graphics.hpp>

class Game {
public:
    Game(int numPlayers, int humanPlayerIndex);
    ~Game();

    void initializeDeck();
    void shuffleDeck();
    void dealCards();
    void update();
    bool isGameOver() const;
    Player* getCurrentPlayer() const;

    // UI related
    void draw(sf::RenderWindow& window);
    void handleInput(sf::Event& event);

private:
    std::vector<Player*> players;
    std::deque<Card*> deck;
    std::deque<Card*> discardPile;
    int currentPlayerIndex;
    int humanPlayerIndex;
    bool waitingForHumanDraw;
    bool gameOver;
    std::string statusMessage;
    std::mt19937 rng;
    sf::Font font;
    bool fontLoaded;

    // Game logic
    void removeMatchingPairs(Player* player);
    Player* getNextPlayerWithCards(Player* current) const;
    void drawCardFromPlayer(Player* current, Player* source);
    void advanceTurn();
    void completePlayerTurn(Player* current, Player* source);
    int activePlayerCount() const;
    bool isHumanTurn() const;
    bool isPointInBottomArea(int x, int y) const;
};

#endif // GAME_H