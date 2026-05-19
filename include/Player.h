#ifndef PLAYER_H
#define PLAYER_H

#include "Card.h"
#include <vector>
#include <string>

class Player {
public:
    Player(std::string name, bool isHuman = false);
    ~Player() = default;

    void addCard(Card* card);
    bool removeCard(Card* card);
    const std::vector<Card*>& getHand() const;
    std::vector<Card*>& getHand();
    std::string getName() const;
    bool isHuman() const;
    int getHandSize() const;

private:
    std::string name;
    bool human;
    std::vector<Card*> hand;
};

#endif // PLAYER_H