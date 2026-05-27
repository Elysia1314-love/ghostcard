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
    : currentPlayerIndex(0), humanPlayerIndex(humanPlayerIndex), waitingForHumanDraw(false), gameOver(false), statusMessage(u8"正在开始抽鬼牌游戏..."), rng(std::random_device{}()), fontLoaded(false) {
    const std::vector<std::string> fontCandidates = {
        "C:/Windows/Fonts/simhei.ttf",
        "C:/Windows/Fonts/simsunb.ttf",
        "C:/Windows/Fonts/simsun.ttf",
        "C:/Windows/Fonts/msyh.ttf",
        "C:/Windows/Fonts/msyhbd.ttf",
        "assets/simhei.ttf",
        "assets/simsun.ttf"
    };
    for (const auto& fontPath : fontCandidates) {
        if (font.openFromFile(fontPath)) {
            fontLoaded = true;
            break;
        }
    }
    if (!fontLoaded) {
        std::cerr << u8"error with chinese language" << std::endl;
    }

    for (int i = 0; i < numPlayers; ++i) {
        std::string name;
        if (i == humanPlayerIndex) {
            name = u8"you";
        } else if (i == 1) {
            name = u8"player2";
        } else if (i == 2) {
            name = u8"player3";
        } else {
            name = u8"player4";
        }
        players.push_back(new Player(name, i == humanPlayerIndex));
    }
    initializeDeck();
    shuffleDeck();
    dealCards();
    for (auto player : players) {
        removeMatchingPairs(player);
    }
    statusMessage = u8"you first, click the bottom area to draw a card from the left player.";
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
    // 生成六个点数的配对牌，每个点数两张，共 12 张；鬼牌为单张
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

    float dt = clock.restart().asSeconds();

    // advance pending discard animations
    for (auto it = pendingDiscards.begin(); it != pendingDiscards.end();) {
        it->elapsed += dt;
        if (it->elapsed < 0.f) { ++it; continue; }
        float t = std::min(1.f, it->elapsed / it->duration);
        float ease = 1 - (1 - t) * (1 - t);
        // interpolate from original start to target by ease factor
        sf::Vector2f direction = it->target - it->pos;
        it->pos += direction * (ease * dt / std::max(0.0001f, it->duration));
        if (it->elapsed >= it->duration) {
            // finish animation: push to discard pile and erase
            discardPile.push_back(it->card);
            it = pendingDiscards.erase(it);
        } else {
            ++it;
        }
    }

    // Check end condition (wait for pending animations to finish)
    if (activePlayerCount() <= 1 && pendingDiscards.empty()) {
        gameOver = true;
        Player* loser = nullptr;
        for (auto player : players) {
            if (player->getHandSize() > 0) { loser = player; break; }
        }
        statusMessage = loser ? loser->getName() + u8" has all the remaining cards, game over." : u8"Game over.";
        return;
    }

    // AI turn handling with reaction delay
    if (!gameOver && !waitingForHumanDraw && !getCurrentPlayer()->isHuman()) {
        if (aiActionTimer <= 0.f) {
            std::uniform_real_distribution<float> dist(0.6f, 1.5f);
            aiActionTimer = dist(rng);
            statusMessage = getCurrentPlayer()->getName() + u8" is thinking...";
            return;
        } else {
            aiActionTimer -= dt;
            if (aiActionTimer > 0.f) return;
            // perform AI action
            Player* current = getCurrentPlayer();
            Player* source = getNextPlayerWithCards(current);
            if (source == current) {
                gameOver = true;
                statusMessage = current->getName() + u8" has all the remaining cards, game over.";
                return;
            }
            completePlayerTurn(current, source);
            aiActionTimer = 0.f;
            return;
        }
    }

    if (!gameOver && !waitingForHumanDraw && getCurrentPlayer()->isHuman()) {
        waitingForHumanDraw = true;
        statusMessage = u8"Your turn: Click the bottom area to draw a card from the left player.";
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
    if (!player) return;
    auto& hand = player->getHand();
    std::unordered_map<int, std::vector<size_t>> rankIndices;
    for (size_t i = 0; i < hand.size(); ++i) {
        Card* card = hand[i];
        if (card->isGhost()) continue;
        rankIndices[static_cast<int>(card->getRank())].push_back(i);
    }

    std::vector<size_t> removeIndices;
    for (auto& [rank, indices] : rankIndices) {
        for (size_t i = 1; i < indices.size(); i += 2) {
            removeIndices.push_back(indices[i - 1]);
            removeIndices.push_back(indices[i]);
        }
    }
    if (removeIndices.empty()) return;

    // compute owner index
    int ownerIndex = -1;
    for (size_t i = 0; i < players.size(); ++i) if (players[i] == player) { ownerIndex = static_cast<int>(i); break; }

    std::sort(removeIndices.rbegin(), removeIndices.rend());
    std::vector<Card*> handSnapshot = hand;

    auto computeCardPos = [&](int owner, int handIndex, int handSize)->sf::Vector2f {
        const float centerX = WINDOW_WIDTH / 2.f;
        const float bottomY = WINDOW_HEIGHT - CARD_HEIGHT - VERTICAL_MARGIN;
        if (owner == humanPlayerIndex) {
            float humanCount = static_cast<float>(handSize);
            float spacing = std::min(140.f, (WINDOW_WIDTH - HORIZONTAL_MARGIN * 2.f - CARD_WIDTH * 0.36f) / std::max(1.f, humanCount - 1.f));
            float humanStartX = std::clamp(centerX - (humanCount * spacing + CARD_WIDTH * 0.36f) / 2.f, HORIZONTAL_MARGIN, WINDOW_WIDTH - HORIZONTAL_MARGIN - CARD_WIDTH * 0.36f);
            return {humanStartX + handIndex * spacing, bottomY};
        } else if (owner == 2) {
            float topStartX = centerX - (handSize * (CARD_WIDTH * 0.4f + 10.f)) / 2.f;
            return {topStartX + handIndex * (CARD_WIDTH * 0.4f + 10.f), VERTICAL_MARGIN + 30.f};
        } else if (owner == 1) {
            float leftStartY = WINDOW_HEIGHT / 2.f - (handSize * (CARD_HEIGHT * 0.25f + 8.f)) / 2.f;
            return {HORIZONTAL_MARGIN, leftStartY + handIndex * (CARD_HEIGHT * 0.25f + 8.f)};
        } else {
            float rightStartY = WINDOW_HEIGHT / 2.f - (handSize * (CARD_HEIGHT * 0.25f + 8.f)) / 2.f;
            return {WINDOW_WIDTH - HORIZONTAL_MARGIN - CARD_WIDTH * 0.25f, rightStartY + handIndex * (CARD_HEIGHT * 0.25f + 8.f)};
        }
    };

    sf::Vector2f discardTarget(WINDOW_WIDTH - HORIZONTAL_MARGIN - CARD_WIDTH * 0.7f, WINDOW_HEIGHT / 2.f - CARD_HEIGHT * 0.35f);

    for (size_t idx = 0; idx < removeIndices.size(); idx += 2) {
        size_t i1 = removeIndices[idx + 1];
        size_t i2 = removeIndices[idx];
        if (i1 >= handSnapshot.size() || i2 >= handSnapshot.size()) continue;
        Card* c1 = handSnapshot[i1];
        Card* c2 = handSnapshot[i2];

        PendingDiscard p1; p1.card = c1; p1.ownerIndex = ownerIndex; p1.pos = computeCardPos(ownerIndex, static_cast<int>(i1), static_cast<int>(handSnapshot.size())); p1.target = discardTarget; p1.elapsed = -0.25f; p1.duration = 0.7f;
        PendingDiscard p2 = p1; p2.card = c2; p2.pos = computeCardPos(ownerIndex, static_cast<int>(i2), static_cast<int>(handSnapshot.size()));
        pendingDiscards.push_back(p1); pendingDiscards.push_back(p2);
    }

    // remove from real hand
    for (size_t index : removeIndices) {
        hand.erase(hand.begin() + index);
    }

    statusMessage = player->getName() + u8" discards a pair (animation).";
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
    statusMessage = current->getName() + u8" draws a card from " + source->getName() + u8".";
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

bool Game::isPointInDiscardArea(int x, int y) const {
    float left = WINDOW_WIDTH - HORIZONTAL_MARGIN - CARD_WIDTH * 0.7f;
    float top = WINDOW_HEIGHT / 2.f - CARD_HEIGHT * 0.35f;
    float width = CARD_WIDTH * 0.7f;
    float height = CARD_HEIGHT * 0.7f;
    return x >= left && x <= left + width && y >= top && y <= top + height;
}

void Game::drawDiscardPilePanel(sf::RenderWindow& window) {
    const float panelWidth = 420.f;
    const float panelHeight = 520.f;
    const float panelX = WINDOW_WIDTH - HORIZONTAL_MARGIN - panelWidth;
    const float panelY = VERTICAL_MARGIN;
    sf::RectangleShape panel({panelWidth, panelHeight});
    panel.setFillColor(sf::Color(10, 10, 10, 220));
    panel.setOutlineThickness(3.f);
    panel.setOutlineColor(sf::Color::White);
    panel.setPosition(sf::Vector2f(panelX, panelY));
    window.draw(panel);

    if (fontLoaded) {
        sf::Text title(font, u8"Discard Pile", 20);
        title.setFillColor(sf::Color::Yellow);
        title.setPosition(sf::Vector2f(panelX + 18.f, panelY + 14.f));
        window.draw(title);

        sf::Text countLabel(font, sf::String(u8"总弃牌数: " + std::to_string(discardPile.size())), 16);
        countLabel.setFillColor(sf::Color::White);
        countLabel.setPosition(sf::Vector2f(panelX + 18.f, panelY + 46.f));
        window.draw(countLabel);
    }

    const float cardWidth = CARD_WIDTH * 0.36f * 0.9f;
    const float cardHeight = CARD_HEIGHT * 0.36f * 0.9f;
    const float xStart = panelX + 18.f;
    const float yStart = panelY + 80.f;
    const float xSpacing = cardWidth + 10.f;
    const float ySpacing = cardHeight + 10.f;
    const int rows = 6;

    for (size_t i = 0; i < discardPile.size(); ++i) {
        int col = static_cast<int>(i / rows);
        int row = static_cast<int>(i % rows);
        float x = xStart + col * xSpacing;
        float y = yStart + row * ySpacing;
        if (discardPile[i]) {
            discardPile[i]->draw(window, x, y);
        }
    }

    if (discardPile.empty() && fontLoaded) {
        sf::Text emptyLabel(font, u8"当前没有弃牌。", 16);
        emptyLabel.setFillColor(sf::Color(200, 200, 200));
        emptyLabel.setPosition(sf::Vector2f(panelX + 18.f, panelY + 90.f));
        window.draw(emptyLabel);
    }
}

void Game::draw(sf::RenderWindow& window) {
    window.clear(sf::Color(25, 90, 35));

    // Header / status
    if (fontLoaded) {
        sf::Text title(font, sf::String(statusMessage), 20);
        title.setFillColor(sf::Color::White);
        title.setPosition(sf::Vector2f(30.f, 20.f));
        window.draw(title);
    }

    auto drawPlayerLabel = [&](const std::string& name, float x, float y, bool active) {
        if (!fontLoaded) return;
        sf::Text label(font, sf::String(name), 18);
        label.setFillColor(active ? sf::Color::Yellow : sf::Color::White);
        label.setPosition(sf::Vector2f(x, y));
        window.draw(label);
    };

    const float topY = VERTICAL_MARGIN;
    const float leftX = HORIZONTAL_MARGIN;
    const float rightX = WINDOW_WIDTH - HORIZONTAL_MARGIN - CARD_WIDTH;
    const float bottomY = WINDOW_HEIGHT - CARD_HEIGHT - VERTICAL_MARGIN;
    const float centerX = WINDOW_WIDTH / 2.f;

    // Top player area (cards shown as backs)
    Player* topPlayer = players[2];
    float topStartX = centerX - (topPlayer->getHandSize() * (CARD_WIDTH * 0.4f + 10.f)) / 2.f;
    for (int i = 0; i < topPlayer->getHandSize(); ++i) {
        sf::RectangleShape back({CARD_WIDTH * 0.4f, CARD_HEIGHT * 0.4f});
        back.setFillColor(sf::Color(80, 80, 180));
        back.setOutlineThickness(2.f);
        back.setOutlineColor(sf::Color::White);
        back.setPosition(sf::Vector2f(topStartX + i * (CARD_WIDTH * 0.4f + 10.f), topY + 30.f));
        window.draw(back);
    }
    drawPlayerLabel(topPlayer->getName() + " (" + std::to_string(topPlayer->getHandSize()) + ")", topStartX, topY, currentPlayerIndex == 2);

    // Left player area
    Player* leftPlayer = players[1];
    float leftStartY = WINDOW_HEIGHT / 2.f - (leftPlayer->getHandSize() * (CARD_HEIGHT * 0.25f + 8.f)) / 2.f;
    for (int i = 0; i < leftPlayer->getHandSize(); ++i) {
        sf::RectangleShape back({CARD_WIDTH * 0.25f, CARD_HEIGHT * 0.25f});
        back.setFillColor(sf::Color(180, 80, 80));
        back.setOutlineThickness(2.f);
        back.setOutlineColor(sf::Color::White);
        back.setPosition(sf::Vector2f(leftX, leftStartY + i * (CARD_HEIGHT * 0.25f + 8.f)));
        window.draw(back);
    }
    drawPlayerLabel(leftPlayer->getName() + " (" + std::to_string(leftPlayer->getHandSize()) + ")", leftX, leftStartY - 30.f, currentPlayerIndex == 1);

    // Right player area
    Player* rightPlayer = players[3];
    float rightStartY = WINDOW_HEIGHT / 2.f - (rightPlayer->getHandSize() * (CARD_HEIGHT * 0.25f + 8.f)) / 2.f;
    for (int i = 0; i < rightPlayer->getHandSize(); ++i) {
        sf::RectangleShape back({CARD_WIDTH * 0.25f, CARD_HEIGHT * 0.25f});
        back.setFillColor(sf::Color(80, 180, 80));
        back.setOutlineThickness(2.f);
        back.setOutlineColor(sf::Color::White);
        back.setPosition(sf::Vector2f(rightX, rightStartY + i * (CARD_HEIGHT * 0.25f + 8.f)));
        window.draw(back);
    }
    drawPlayerLabel(rightPlayer->getName() + " (" + std::to_string(rightPlayer->getHandSize()) + ")", rightX, rightStartY - 30.f, currentPlayerIndex == 3);

    // Bottom player area (human)
    Player* human = players[humanPlayerIndex];
    float humanCount = static_cast<float>(human->getHandSize());
    float spacing = std::min(140.f, (WINDOW_WIDTH - HORIZONTAL_MARGIN * 2.f - CARD_WIDTH * 0.36f) / std::max(1.f, humanCount - 1.f));
    float humanStartX = std::clamp(centerX - (humanCount * spacing + CARD_WIDTH * 0.36f) / 2.f, HORIZONTAL_MARGIN, WINDOW_WIDTH - HORIZONTAL_MARGIN - CARD_WIDTH * 0.36f);
    for (int i = 0; i < human->getHandSize(); ++i) {
        human->getHand()[i]->draw(window, humanStartX + i * spacing, bottomY);
    }
    drawPlayerLabel(human->getName() + " (" + std::to_string(human->getHandSize()) + ")", humanStartX, bottomY - 36.f, currentPlayerIndex == humanPlayerIndex);

    // Draw pending discard animations on top
    for (auto &p : pendingDiscards) {
        if (p.card) p.card->draw(window, p.pos.x, p.pos.y - 20.f * std::max(0.f, std::min(1.f, p.elapsed / p.duration)));
    }

    // Discard frame and label
    sf::RectangleShape discardFrame({CARD_WIDTH * 0.7f, CARD_HEIGHT * 0.7f});
    discardFrame.setFillColor(sf::Color(40, 40, 40, 180));
    discardFrame.setOutlineThickness(2.f);
    discardFrame.setOutlineColor(sf::Color::White);
    discardFrame.setPosition(sf::Vector2f(WINDOW_WIDTH - HORIZONTAL_MARGIN - CARD_WIDTH * 0.7f, WINDOW_HEIGHT / 2.f - CARD_HEIGHT * 0.35f));
    window.draw(discardFrame);

    if (fontLoaded) {
        std::string disc = std::string(u8"has Discarded: ") + std::to_string(discardPile.size());
        sf::Text discardLabel(font, sf::String(disc), 16);
        discardLabel.setFillColor(sf::Color::White);
        discardLabel.setPosition(sf::Vector2f(WINDOW_WIDTH - HORIZONTAL_MARGIN - CARD_WIDTH * 0.7f, WINDOW_HEIGHT / 2.f + CARD_HEIGHT * 0.35f + 10.f));
        window.draw(discardLabel);

        sf::Text hint(font, sf::String(u8"点击查看弃牌堆"), 14);
        hint.setFillColor(sf::Color(200, 200, 200));
        hint.setPosition(sf::Vector2f(WINDOW_WIDTH - HORIZONTAL_MARGIN - CARD_WIDTH * 0.7f, WINDOW_HEIGHT / 2.f - CARD_HEIGHT * 0.35f - 22.f));
        window.draw(hint);
    }

    if (showDiscardPanel) {
        drawDiscardPilePanel(window);
    }

    if (waitingForHumanDraw && currentPlayerIndex == humanPlayerIndex) {
        sf::RectangleShape guide({WINDOW_WIDTH - HORIZONTAL_MARGIN * 2.f, CARD_HEIGHT * 0.4f});
        guide.setFillColor(sf::Color(255, 255, 255, 50));
        guide.setPosition(sf::Vector2f(HORIZONTAL_MARGIN, bottomY - 10.f));
        window.draw(guide);
    }
}

void Game::handleInput(sf::Event& event) {
    if (gameOver) {
        return;
    }
    if (event.is<sf::Event::MouseButtonPressed>()) {
        if (const sf::Event::MouseButtonPressed* mouseEvent = event.getIf<sf::Event::MouseButtonPressed>()) {
            if (mouseEvent->button == sf::Mouse::Button::Left) {
                if (isPointInDiscardArea(mouseEvent->position.x, mouseEvent->position.y)) {
                    showDiscardPanel = !showDiscardPanel;
                    return;
                }

                if (waitingForHumanDraw) {
                    if (isPointInBottomArea(mouseEvent->position.x, mouseEvent->position.y) && getCurrentPlayer()->isHuman()) {
                        Player* human = getCurrentPlayer();
                        Player* source = getNextPlayerWithCards(human);
                        if (source != human) {
                            drawCardFromPlayer(human, source);
                            statusMessage = u8"You draw a card from " + source->getName() + u8".";
                            removeMatchingPairs(human);
                            waitingForHumanDraw = false;
                            advanceTurn();
                        }
                    }
                }
            }
        }
    }
}
