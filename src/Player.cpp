#include "Player.h"
#include <algorithm>

Player::Player(std::string name, bool isHuman)
    : name(name), human(isHuman) {}

void Player::addCard(Card* card) {
    hand.push_back(card);
}

bool Player::removeCard(Card* card) {
    auto it = std::find(hand.begin(), hand.end(), card);
    if (it != hand.end()) {
        hand.erase(it);
        return true;
    }
    return false;
}

const std::vector<Card*>& Player::getHand() const {
    return hand;
}

std::vector<Card*>& Player::getHand() {
    return hand;
}

std::string Player::getName() const { return name; }
bool Player::isHuman() const { return human; }
int Player::getHandSize() const { return static_cast<int>(hand.size()); }