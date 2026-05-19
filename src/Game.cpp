#include "Game.h"
#include <algorithm>
#include <random>
#include <iostream>
#include <unordered_map>

namespace {
    static constexpr float WINDOW_WIDTH = 1024.f;
    static constexpr float WINDOW_HEIGHT = 768.f;
    static constexpr float CARD_WIDTH = 120.f;
    static constexpr float CARD_HEIGHT = 170.f;
    static constexpr float HORIZONTAL_MARGIN = 60.f;
    static constexpr float VERTICAL_MARGIN = 40.f;
}

Game::Game(int numPlayers, int humanPlayerIndex)
    : currentPlayerIndex(0), humanPlayerIndex(humanPlayerIndex), waitingForHumanDraw(false), gameOver(false), statusMessage("正在开始抽鬼牌游戏..."), rng(std::random_device{}()), fontLoaded(false) {
    if (font.openFromFile("C:/Windows/Fonts/simhei.ttf") || font.openFromFile("C:/Windows/Fonts/simsunb.ttf") || font.openFromFile("simhei.ttf") || font.openFromFile("simsunb.ttf")) {
        fontLoaded = true;
    } else {
        std::cerr << "警告：未能加载中文字体文件，游戏中文可能无法正确显示。" << std::endl;
    }

    for (int i = 0; i < numPlayers; ++i) {
        std::string name;
        if (i == humanPlayerIndex) {
            name = "你";
        } else if (i == 1) {
            name = "左家";
        } else if (i == 2) {
            name = "上家";
        } else {
            name = "右家";
        }
        players.push_back(new Player(name, i == humanPlayerIndex));
    }
    initializeDeck();
    shuffleDeck();
    dealCards();
    for (auto player : players) {
        removeMatchingPairs(player);
    }
    statusMessage = "你先手，点击底部区域从左家抽牌。";
}

Game::~Game() {
    for (auto player : players) {
        delete player;
    }
    for (auto card : deck) {
        delete card;
    }
    for (auto card : discardPile) {
        delete card;
    }
}

void Game::initializeDeck() {
    deck.clear();
    for (int rank = 1; rank <= 12; ++rank) {
        deck.push_back(new Card(static_cast<Rank>(rank)));
        deck.push_back(new Card(static_cast<Rank>(rank)));
    }
    deck.push_back(new Card(Rank::Ghost));
}

void Game::shuffleDeck() {
    std::shuffle(deck.begin(), deck.end(), rng);
}

void Game::dealCards() {
    int playerCount = static_cast<int>(players.size());
    for (size_t i = 0; i < deck.size(); ++i) {
        players[i % playerCount]->addCard(deck[i]);
    }
    deck.clear();
}

void Game::update() {
    if (gameOver) {
        return;
    }

    if (activePlayerCount() <= 1) {
        gameOver = true;
        Player* loser = nullptr;
        for (auto player : players) {
            if (player->getHandSize() > 0) {
                loser = player;
                break;
            }
        }
        statusMessage = loser ? loser->getName() + " 剩余鬼牌，游戏结束。" : "游戏结束。";
        return;
    }

    while (!gameOver && !waitingForHumanDraw && !getCurrentPlayer()->isHuman()) {
        Player* current = getCurrentPlayer();
        Player* source = getNextPlayerWithCards(current);
        if (source == current) {
            gameOver = true;
            statusMessage = current->getName() + " 拥有所有剩余牌，游戏结束。";
            return;
        }
        completePlayerTurn(current, source);
    }

    if (!gameOver && !waitingForHumanDraw && getCurrentPlayer()->isHuman()) {
        waitingForHumanDraw = true;
        statusMessage = "你的回合：点击底部区域从左家抽一张牌。";
    }
}

bool Game::isGameOver() const {
    return gameOver;
}

Player* Game::getCurrentPlayer() const {
    return players[currentPlayerIndex];
}

bool Game::isHumanTurn() const {
    return getCurrentPlayer() && getCurrentPlayer()->isHuman();
}

void Game::removeMatchingPairs(Player* player) {
    if (!player) {
        return;
    }
    auto& hand = player->getHand();
    std::unordered_map<int, std::vector<size_t>> rankIndices;
    for (size_t i = 0; i < hand.size(); ++i) {
        Card* card = hand[i];
        if (card->isGhost()) {
            continue;
        }
        rankIndices[static_cast<int>(card->getRank())].push_back(i);
    }

    std::vector<size_t> removeIndices;
    for (auto& [rank, indices] : rankIndices) {
        for (size_t i = 1; i < indices.size(); i += 2) {
            removeIndices.push_back(indices[i - 1]);
            removeIndices.push_back(indices[i]);
        }
    }

    if (removeIndices.empty()) {
        return;
    }

    std::sort(removeIndices.rbegin(), removeIndices.rend());
    for (size_t index : removeIndices) {
        discardPile.push_back(hand[index]);
        hand.erase(hand.begin() + index);
    }

    statusMessage = player->getName() + " 配对出牌。";
}

Player* Game::getNextPlayerWithCards(Player* current) const {
    if (!current) {
        return nullptr;
    }
    int index = currentPlayerIndex;
    int playerCount = static_cast<int>(players.size());
    for (int i = 1; i < playerCount; ++i) {
        int candidate = (index + i) % playerCount;
        if (players[candidate]->getHandSize() > 0) {
            return players[candidate];
        }
    }
    return current;
}

void Game::drawCardFromPlayer(Player* current, Player* source) {
    if (!current || !source || source == current || source->getHandSize() == 0) {
        return;
    }
    auto& sourceHand = source->getHand();
    std::uniform_int_distribution<int> dist(0, static_cast<int>(sourceHand.size()) - 1);
    int index = dist(rng);
    Card* drawnCard = sourceHand[index];
    source->removeCard(drawnCard);
    current->addCard(drawnCard);
}

void Game::completePlayerTurn(Player* current, Player* source) {
    if (!current || !source) {
        return;
    }
    drawCardFromPlayer(current, source);
    statusMessage = current->getName() + " 从 " + source->getName() + " 抽牌。";
    removeMatchingPairs(current);
    advanceTurn();
}

int Game::activePlayerCount() const {
    int count = 0;
    for (auto player : players) {
        if (player->getHandSize() > 0) {
            count++;
        }
    }
    return count;
}

void Game::advanceTurn() {
    int playerCount = static_cast<int>(players.size());
    if (activePlayerCount() <= 1) {
        return;
    }
    do {
        currentPlayerIndex = (currentPlayerIndex + 1) % playerCount;
    } while (players[currentPlayerIndex]->getHandSize() == 0);
}

bool Game::isPointInBottomArea(int x, int y) const {
    float left = HORIZONTAL_MARGIN;
    float right = WINDOW_WIDTH - HORIZONTAL_MARGIN;
    float top = WINDOW_HEIGHT - CARD_HEIGHT - VERTICAL_MARGIN;
    float bottom = WINDOW_HEIGHT - VERTICAL_MARGIN;
    return x >= left && x <= right && y >= top && y <= bottom;
}

void Game::draw(sf::RenderWindow& window) {
    window.clear(sf::Color(25, 90, 35));

    if (fontLoaded) {
        sf::Text title(font, statusMessage, 20);
        title.setFillColor(sf::Color::White);
        title.setPosition({30.f, 20.f});
        window.draw(title);
    }

    auto drawPlayerLabel = [&](const std::string& name, float x, float y, bool active) {
        if (!fontLoaded) {
            return;
        }
        sf::Text label(font, name, 18);
        label.setFillColor(active ? sf::Color::Yellow : sf::Color::White);
        label.setPosition({x, y});
        window.draw(label);
    };

    const float topY = VERTICAL_MARGIN;
    const float leftX = HORIZONTAL_MARGIN;
    const float rightX = WINDOW_WIDTH - HORIZONTAL_MARGIN - CARD_WIDTH;
    const float bottomY = WINDOW_HEIGHT - CARD_HEIGHT - VERTICAL_MARGIN;
    const float centerX = WINDOW_WIDTH / 2.f;

    Player* topPlayer = players[2];
    float topStartX = centerX - (topPlayer->getHandSize() * (CARD_WIDTH * 0.4f + 10.f)) / 2.f;
    for (int i = 0; i < topPlayer->getHandSize(); ++i) {
        sf::RectangleShape back({CARD_WIDTH * 0.4f, CARD_HEIGHT * 0.4f});
        back.setFillColor(sf::Color(80, 80, 180));
        back.setOutlineThickness(2.f);
        back.setOutlineColor(sf::Color::White);
        back.setPosition({topStartX + i * (CARD_WIDTH * 0.4f + 10.f), topY + 30.f});
        window.draw(back);
    }
    drawPlayerLabel(topPlayer->getName() + " (" + std::to_string(topPlayer->getHandSize()) + ")", topStartX, topY, currentPlayerIndex == 2);

    Player* leftPlayer = players[1];
    float leftStartY = WINDOW_HEIGHT / 2.f - (leftPlayer->getHandSize() * (CARD_HEIGHT * 0.25f + 8.f)) / 2.f;
    for (int i = 0; i < leftPlayer->getHandSize(); ++i) {
        sf::RectangleShape back({CARD_WIDTH * 0.25f, CARD_HEIGHT * 0.25f});
        back.setFillColor(sf::Color(180, 80, 80));
        back.setOutlineThickness(2.f);
        back.setOutlineColor(sf::Color::White);
        back.setPosition({leftX, leftStartY + i * (CARD_HEIGHT * 0.25f + 8.f)});
        window.draw(back);
    }
    drawPlayerLabel(leftPlayer->getName() + " (" + std::to_string(leftPlayer->getHandSize()) + ")", leftX, leftStartY - 30.f, currentPlayerIndex == 1);

    Player* rightPlayer = players[3];
    float rightStartY = WINDOW_HEIGHT / 2.f - (rightPlayer->getHandSize() * (CARD_HEIGHT * 0.25f + 8.f)) / 2.f;
    for (int i = 0; i < rightPlayer->getHandSize(); ++i) {
        sf::RectangleShape back({CARD_WIDTH * 0.25f, CARD_HEIGHT * 0.25f});
        back.setFillColor(sf::Color(80, 180, 80));
        back.setOutlineThickness(2.f);
        back.setOutlineColor(sf::Color::White);
        back.setPosition({rightX, rightStartY + i * (CARD_HEIGHT * 0.25f + 8.f)});
        window.draw(back);
    }
    drawPlayerLabel(rightPlayer->getName() + " (" + std::to_string(rightPlayer->getHandSize()) + ")", rightX, rightStartY - 30.f, currentPlayerIndex == 3);

    Player* human = players[humanPlayerIndex];
    float humanCount = static_cast<float>(human->getHandSize());
    float spacing = std::min(140.f, (WINDOW_WIDTH - HORIZONTAL_MARGIN * 2.f - CARD_WIDTH * 0.36f) / std::max(1.f, humanCount - 1.f));
    float humanStartX = std::clamp(centerX - (humanCount * spacing + CARD_WIDTH * 0.36f) / 2.f, HORIZONTAL_MARGIN, WINDOW_WIDTH - HORIZONTAL_MARGIN - CARD_WIDTH * 0.36f);
    for (int i = 0; i < human->getHandSize(); ++i) {
        human->getHand()[i]->draw(window, humanStartX + i * spacing, bottomY);
    }
    drawPlayerLabel(human->getName() + " (" + std::to_string(human->getHandSize()) + ")", humanStartX, bottomY - 36.f, currentPlayerIndex == humanPlayerIndex);

    sf::RectangleShape discardFrame({CARD_WIDTH * 0.7f, CARD_HEIGHT * 0.7f});
    discardFrame.setFillColor(sf::Color(40, 40, 40, 180));
    discardFrame.setOutlineThickness(2.f);
    discardFrame.setOutlineColor(sf::Color::White);
    discardFrame.setPosition({WINDOW_WIDTH - HORIZONTAL_MARGIN - CARD_WIDTH * 0.7f, WINDOW_HEIGHT / 2.f - CARD_HEIGHT * 0.35f});
    window.draw(discardFrame);

    if (fontLoaded) {
        sf::Text discardLabel(font, "已配对丢牌: " + std::to_string(discardPile.size()), 16);
        discardLabel.setFillColor(sf::Color::White);
        discardLabel.setPosition({WINDOW_WIDTH - HORIZONTAL_MARGIN - CARD_WIDTH * 0.7f, WINDOW_HEIGHT / 2.f + CARD_HEIGHT * 0.35f + 10.f});
        window.draw(discardLabel);
    }

    if (waitingForHumanDraw && currentPlayerIndex == humanPlayerIndex) {
        sf::RectangleShape guide({WINDOW_WIDTH - HORIZONTAL_MARGIN * 2.f, CARD_HEIGHT * 0.4f});
        guide.setFillColor(sf::Color(255, 255, 255, 50));
        guide.setPosition({HORIZONTAL_MARGIN, bottomY - 10.f});
        window.draw(guide);
    }
}

void Game::handleInput(sf::Event& event) {
    if (gameOver) {
        return;
    }
    if (event.is<sf::Event::MouseButtonPressed>()) {
        if (const sf::Event::MouseButtonPressed* mouseEvent = event.getIf<sf::Event::MouseButtonPressed>()) {
            if (mouseEvent->button == sf::Mouse::Button::Left && waitingForHumanDraw) {
                if (isPointInBottomArea(mouseEvent->position.x, mouseEvent->position.y) && getCurrentPlayer()->isHuman()) {
                    Player* human = getCurrentPlayer();
                    Player* source = getNextPlayerWithCards(human);
                    if (source != human) {
                        drawCardFromPlayer(human, source);
                        statusMessage = "你从 " + source->getName() + " 抽取了一张牌。";
                        removeMatchingPairs(human);
                        waitingForHumanDraw = false;
                        advanceTurn();
                    }
                }
            }
        }
    }
}
